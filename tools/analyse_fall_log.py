#!/usr/bin/env python3
"""Turn a raw FALLCSV serial capture into numbers the fall thresholds can be set from.

Why this exists
---------------
Every threshold in the fall block of app_config.h is currently a guess, and the
header says so. Guesses are fine as a starting point and useless as an ending
one: the only way to know whether 2.5 g is a fall or a handshake on this watch,
on this wrist, is to record both and look.

This reads the capture, splits it into events, and prints what each threshold
would have decided. It answers three questions in order of importance:

1. Would the current thresholds fire on the recorded falls?
2. Would they also fire on ordinary movement?
3. If not, what values would?

Question 2 is the one people skip. Sensitivity alone is trivially maximised by
lowering every threshold to zero; what makes a detector useful is clearing the
falls while staying under everything else, and that gap only exists if the
negative recording exists.

What it does NOT do
-------------------
It does not train the decision tree. That is a separate step and it needs the
event boundaries this script finds, which is why this comes first.

It does not label events for you. An event is "a stretch of large acceleration
with quiet either side"; whether that stretch was a fall or a bed is something
only you know, so --falls tells it how many of the events in the file are real
falls (the largest N by peak). Read the printed table before trusting that.

Run
---
    python tools/analyse_fall_log.py log_am.txt
    python tools/analyse_fall_log.py log_nga.txt --falls 20
    python tools/analyse_fall_log.py log_nga.txt --falls 20 --plot out/
"""

import argparse
import math
import os
import re
import sys

# Must match include/app_config.h. If you change a threshold there, change it
# here too, or this script will report on a firmware that no longer exists.
LSB_PER_G = 4096.0
FREEFALL_G = 0.4
IMPACT_G = 2.5
IMPACT_STANDALONE_G = 3.2
IMPACT_WINDOW_MS = 1500
CONFIRM_WINDOW_MS = 3000
SETTLE_MS = 400
STILLNESS_MAX_DEV_G = 0.35
STILLNESS_MIN_CALM_RATIO = 0.70
ORIENTATION_MIN_DEG = 30.0
NEARMISS_G = 1.8

# Gravity low-pass coefficient, matching fall_detector.cpp.
GRAV_ALPHA = 0.02

# Event splitting. An event starts when total acceleration leaves the quiet
# band and ends once it has been back inside for QUIET_MS.
EVENT_ENTER_G = 1.5
EVENT_QUIET_MS = 800
EVENT_PRE_MS = 1000      # context kept before the trigger, for the fall itself
EVENT_POST_MS = 4000     # context kept after, covering the confirmation window

CSV = re.compile(
    r"^FALLCSV,(\d+),(-?\d+),(-?\d+),(-?\d+),(-?\d+),(-?\d+),(-?\d+),"
    r"([\d.]+),(\d+)\s*$"
)

STATE_NAMES = {0: "NORMAL", 1: "SUSPECTED", 2: "ALERT", 3: "SENDING",
               4: "SENT", 5: "FAILED"}


class Sample:
    __slots__ = ("t", "ax", "ay", "az", "gx", "gy", "gz", "g", "state")

    def __init__(self, t, ax, ay, az, gx, gy, gz, g, state):
        self.t = t
        self.ax, self.ay, self.az = ax, ay, az
        self.gx, self.gy, self.gz = gx, gy, gz
        self.g = g
        self.state = state


def parse(path):
    """Read the capture. Returns (samples, firmware_lines, skipped)."""
    samples, firmware, skipped = [], [], 0
    with open(path, encoding="utf-8", errors="replace") as fh:
        for raw in fh:
            line = raw.strip()
            if not line:
                continue
            m = CSV.match(line)
            if m:
                t, ax, ay, az, gx, gy, gz = (int(m.group(i)) for i in range(1, 8))
                samples.append(Sample(t, ax, ay, az, gx, gy, gz,
                                      float(m.group(8)), int(m.group(9))))
            elif line.startswith("FALLCSV"):
                skipped += 1                       # truncated by serial overrun
            elif "[FALL]" in line or "FALL CONFIRMED" in line or "WARNING" in line:
                firmware.append(line)
    return samples, firmware, skipped


def magnitude_g(s):
    """Recompute from the raw axes rather than trusting the logged totalG.

    The firmware prints totalG to three decimals; recomputing keeps this script
    honest if that column is ever dropped, and catches a scale mismatch between
    LSB_PER_G here and the CTRL2 setting on the device.
    """
    x, y, z = s.ax / LSB_PER_G, s.ay / LSB_PER_G, s.az / LSB_PER_G
    return math.sqrt(x * x + y * y + z * z)


def check_health(samples, firmware, skipped):
    """Sanity-check the capture before any conclusion is drawn from it."""
    print("=" * 70)
    print("CAPTURE HEALTH")
    print("=" * 70)

    n = len(samples)
    if n == 0:
        print("  No FALLCSV lines found.")
        # Which of the two failures this is decides what to do next, and the
        # answer is already in the file: firmware chatter means the board was
        # alive and talking, so only the CSV switch is off. Silence means the
        # capture itself failed and reflashing would not help.
        if firmware:
            print(f"  The board was running -- {len(firmware)} fall-detector "
                  "line(s) came through.")
            print("  -> FALL_LOG_RAW_SAMPLES is 0 in include/app_config.h.")
            print("     Set it to 1, rebuild, reflash, and capture again.")
        else:
            print("  Nothing from the firmware either: no boot banner, no fall lines.")
            print("  -> Check the COM port, the baud rate (115200), and that")
            print("     the capture was running while the watch was powered.")
        return False

    span_ms = samples[-1].t - samples[0].t
    print(f"  samples          {n}")
    print(f"  duration         {span_ms / 1000.0:.1f} s")

    if skipped:
        print(f"  malformed lines  {skipped}  (serial overrun; harmless if few)")

    # Sample period. The loop targets 20 ms; a wider spread means the loop was
    # blocked, and every millisecond-based threshold below is measured in those
    # same ticks.
    gaps = [samples[i].t - samples[i - 1].t for i in range(1, n)]
    gaps_sorted = sorted(gaps)
    median_gap = gaps_sorted[len(gaps_sorted) // 2]
    p95_gap = gaps_sorted[int(len(gaps_sorted) * 0.95)]
    worst = max(gaps)
    print(f"  tick median      {median_gap} ms   (firmware targets 20 ms)")
    print(f"  tick p95 / max   {p95_gap} / {worst} ms")
    if median_gap > 25:
        print("  !! The loop is running slower than 20 ms per tick. Serial")
        print("     printing at this rate may itself be the cause; the fall")
        print("     windows in ms are unaffected but the sample counts are.")

    # Rest magnitude. With the watch still, |a| must sit at 1 g. A systematic
    # offset here shifts every g-threshold by the same amount, so it is worth
    # more than it looks: it is the one number that validates LSB_PER_G.
    quiet = [magnitude_g(s) for s in samples if abs(magnitude_g(s) - 1.0) < 0.15]
    if quiet:
        mean_rest = sum(quiet) / len(quiet)
        var = sum((q - mean_rest) ** 2 for q in quiet) / len(quiet)
        print(f"  rest magnitude   {mean_rest:.3f} g  "
              f"(noise sd {math.sqrt(var):.4f} g, n={len(quiet)})")
        if abs(mean_rest - 1.0) > 0.05:
            print(f"  !! Rest should be 1.000 g. Off by {mean_rest - 1.0:+.3f} g.")
            print("     Either LSB_PER_G here disagrees with CTRL2 on the device,")
            print("     or the accelerometer needs an offset calibration.")
    else:
        print("  rest magnitude   never quiet enough to measure")

    # Saturation. At +/-8g the raw axes clip at 32767, and a clipped impact
    # reads lower than it was -- which matters for the AI features later even
    # though the 2.5-3.2 g thresholds are cleared either way.
    clipped = sum(1 for s in samples
                  if max(abs(s.ax), abs(s.ay), abs(s.az)) >= 32000)
    if clipped:
        print(f"  clipped samples  {clipped}  ({100.0 * clipped / n:.2f}%)"
              "  -- impacts hit the +/-8g ceiling")

    print()
    return True


def find_events(samples):
    """Split the capture into stretches of movement separated by quiet.

    Threshold-crossing rather than a fixed window: a fall is one continuous
    disturbance of unknown length, and cutting it at an arbitrary boundary
    would split one impact across two events or merge a fall with the getting-up
    afterwards.
    """
    events = []
    i, n = 0, len(samples)
    while i < n:
        if magnitude_g(samples[i]) <= EVENT_ENTER_G:
            i += 1
            continue

        trigger = i
        # Walk forward to the last sample before EVENT_QUIET_MS of calm.
        j = i
        last_loud = i
        while j < n and samples[j].t - samples[last_loud].t < EVENT_QUIET_MS:
            if magnitude_g(samples[j]) > EVENT_ENTER_G:
                last_loud = j
            j += 1

        start = trigger
        while start > 0 and samples[trigger].t - samples[start - 1].t < EVENT_PRE_MS:
            start -= 1
        end = last_loud
        while end + 1 < n and samples[end + 1].t - samples[last_loud].t < EVENT_POST_MS:
            end += 1

        events.append((start, trigger, end))

        # Resume past the whole event, context included, not just past the
        # noisy part. The tail deliberately holds the confirmation window, and
        # someone who has fallen moves inside it -- rolling over, pushing up on
        # an elbow. Resuming at last_loud would let that movement open a second
        # event overlapping the first, so one fall would be counted twice and
        # its own struggling would be filed as an ordinary-movement negative.
        i = end + 1
    return events


def replay(samples, start, trigger, end):
    """Run the firmware's 4-phase logic over one event and report every number.

    This is a re-implementation, not the firmware itself, so it can disagree.
    It exists to answer "what would a different threshold have done", which the
    device cannot answer without being reflashed and the fall repeated.
    """
    gx = gy = gz = None
    freefall_t = None
    pre = None
    entry, entry_t = None, None
    peak = 0.0
    min_g = 999.0

    # Pass 1 stops at the entry sample. The gravity filter must not run past it:
    # the firmware's copy has only ever seen the pre-impact history at that
    # point, and letting this one read ahead would compare a "before" vector
    # that already knew how the event ended.
    entry_k = end
    for k in range(start, end + 1):
        s = samples[k]
        ax, ay, az = s.ax / LSB_PER_G, s.ay / LSB_PER_G, s.az / LSB_PER_G
        g = magnitude_g(s)
        peak = max(peak, g)
        min_g = min(min_g, g)

        if gx is None:
            gx, gy, gz = ax, ay, az        # seed, so the filter starts settled
        else:
            gx = gx * (1 - GRAV_ALPHA) + ax * GRAV_ALPHA
            gy = gy * (1 - GRAV_ALPHA) + ay * GRAV_ALPHA
            gz = gz * (1 - GRAV_ALPHA) + az * GRAV_ALPHA

        if g < FREEFALL_G and freefall_t is None:
            freefall_t = s.t
            pre = (gx, gy, gz)
        if freefall_t is not None:
            if s.t - freefall_t > IMPACT_WINDOW_MS:
                freefall_t = None
            elif g > IMPACT_G:
                entry, entry_t, entry_k = "A", s.t, k
        if entry is None and g > IMPACT_STANDALONE_G:
            pre = (gx, gy, gz)
            entry, entry_t, entry_k = "B", s.t, k
        if entry is not None:
            break

    # The peak and the free-fall minimum describe the whole event, so finish
    # scanning for them even though the phase logic has stopped.
    for k in range(entry_k + 1, end + 1):
        g = magnitude_g(samples[k])
        peak = max(peak, g)
        min_g = min(min_g, g)

    result = {
        "peak_g": peak,
        "min_g": min_g,
        "entry": entry,
        "t": samples[trigger].t,
        "duration_ms": samples[end].t - samples[start].t,
        "logged_state": max(s.state for s in samples[start:end + 1]),
    }

    if entry is None:
        result.update(calm_ratio=None, max_dev=None, tilt_deg=None,
                      still=False, reoriented=False, verdict="no entry")
        return result

    # Phases 3 and 4, over the confirmation window that follows entry.
    #
    # This walks the full sample array rather than stopping at `end`. The event
    # boundary was drawn around the noisy part plus context; the firmware's
    # confirmation window is a fixed 3 s from the impact and pays no attention
    # to that boundary. Clipping here would end the window early and read the
    # missing samples as the wearer refusing to lie still.
    still_n = calm_n = 0
    max_dev = 0.0
    post_gx, post_gy, post_gz = gx, gy, gz
    last_k = entry_k
    for k in range(entry_k, len(samples)):
        s = samples[k]
        elapsed = s.t - entry_t
        if elapsed >= CONFIRM_WINDOW_MS:
            break
        last_k = k
        ax, ay, az = s.ax / LSB_PER_G, s.ay / LSB_PER_G, s.az / LSB_PER_G
        post_gx = post_gx * (1 - GRAV_ALPHA) + ax * GRAV_ALPHA
        post_gy = post_gy * (1 - GRAV_ALPHA) + ay * GRAV_ALPHA
        post_gz = post_gz * (1 - GRAV_ALPHA) + az * GRAV_ALPHA
        if elapsed >= SETTLE_MS:
            dev = abs(magnitude_g(s) - 1.0)
            max_dev = max(max_dev, dev)
            still_n += 1
            if dev < STILLNESS_MAX_DEV_G:
                calm_n += 1

    # An event whose tail was cut short by the end of the capture has no
    # stillness evidence either way. Saying "not still" would file it as a
    # miss caused by the wearer moving, when the real cause is that the
    # recording stopped -- so mark it and keep it out of the statistics.
    covered_ms = samples[last_k].t - entry_t
    truncated = covered_ms < CONFIRM_WINDOW_MS - 200

    ratio = (calm_n / still_n) if still_n else 0.0
    still = ratio >= STILLNESS_MIN_CALM_RATIO

    tilt = 0.0
    if pre:
        pmag = math.sqrt(sum(c * c for c in pre))
        nmag = math.sqrt(post_gx ** 2 + post_gy ** 2 + post_gz ** 2)
        if pmag > 0.1 and nmag > 0.1:
            dot = (pre[0] * post_gx + pre[1] * post_gy + pre[2] * post_gz)
            tilt = math.degrees(math.acos(max(-1.0, min(1.0, dot / (pmag * nmag)))))
    reoriented = tilt > ORIENTATION_MIN_DEG

    result.update(
        calm_ratio=ratio, calm_n=calm_n, still_n=still_n,
        max_dev=max_dev, tilt_deg=tilt,
        still=still, reoriented=reoriented, truncated=truncated,
        verdict=("truncated" if truncated
                 else "ALERT" if (still and reoriented) else "dismissed"),
        fail=("" if truncated or (still and reoriented)
              else ("stillness " if not still else "") +
                   ("tilt" if not reoriented else "")).strip(),
    )
    return result


def report_events(samples, events, n_falls):
    """Print one row per event, and the threshold-by-threshold breakdown."""
    rows = [replay(samples, *e) for e in events]

    # Labelling: the n_falls largest peaks are assumed to be the falls. Crude,
    # and printed alongside every peak so it can be checked by eye.
    order = sorted(range(len(rows)), key=lambda i: rows[i]["peak_g"], reverse=True)
    is_fall = [False] * len(rows)
    for i in order[:n_falls]:
        is_fall[i] = True

    print("=" * 70)
    print(f"EVENTS  ({len(rows)} found"
          + (f", top {n_falls} by peak assumed to be falls)" if n_falls else ")"))
    print("=" * 70)
    if n_falls:
        print("  Check the label column against what you actually did. If it is")
        print("  wrong the calibration below is wrong with it.\n")

    print(f"  {'#':>3} {'t (s)':>8} {'label':>6} {'peak':>6} {'min':>5} "
          f"{'entry':>5} {'calm':>9} {'tilt':>6}  verdict")
    print("  " + "-" * 66)
    for i, r in enumerate(rows):
        label = "FALL" if is_fall[i] else "-"
        calm = (f"{r['calm_n']}/{r['still_n']}"
                if r.get("still_n") else "-")
        tilt = f"{r['tilt_deg']:.0f}" if r["tilt_deg"] is not None else "-"
        entry = r["entry"] or "-"
        note = r["verdict"] + (f" ({r['fail']})" if r.get("fail") else "")
        print(f"  {i + 1:>3} {r['t'] / 1000.0:>8.1f} {label:>6} "
              f"{r['peak_g']:>6.2f} {r['min_g']:>5.2f} {entry:>5} "
              f"{calm:>9} {tilt:>6}  {note}")

    print()
    return rows, is_fall


def report_thresholds(rows, is_fall):
    """For each threshold, print what falls needed and what movement produced.

    A threshold is only usable if there is daylight between those two, and the
    point of printing both is that the gap -- or its absence -- is the finding.
    """
    falls = [r for r, f in zip(rows, is_fall) if f and not r.get("truncated")]
    others = [r for r, f in zip(rows, is_fall) if not f and not r.get("truncated")]
    if not falls:
        print("No events labelled as falls, so no threshold comparison. Pass")
        print("--falls N to label the N largest events.\n")
        return

    print("=" * 70)
    print("THRESHOLDS: what falls needed vs what ordinary movement produced")
    print("=" * 70)

    def spread(vals):
        if not vals:
            return None
        v = sorted(vals)
        return v[0], v[len(v) // 2], v[-1]

    def line(name, current, fall_vals, other_vals, higher_is_fall=True):
        f = spread(fall_vals)
        o = spread(other_vals)
        print(f"\n  {name}   (currently {current})")
        if f:
            print(f"    falls     min {f[0]:.2f}   median {f[1]:.2f}   max {f[2]:.2f}")
        if o:
            print(f"    other     min {o[0]:.2f}   median {o[1]:.2f}   max {o[2]:.2f}")
        if f and o:
            if higher_is_fall:
                gap_lo, gap_hi = o[2], f[0]
                if gap_hi > gap_lo:
                    print(f"    -> clear gap {gap_lo:.2f} .. {gap_hi:.2f}; "
                          f"a threshold at {(gap_lo + gap_hi) / 2:.2f} separates them")
                else:
                    n_over = sum(1 for v in other_vals if v >= f[0])
                    print(f"    -> NO clear gap: {n_over} non-fall event(s) reach "
                          f"the weakest fall. Peak alone cannot separate these.")
            else:
                gap_lo, gap_hi = f[2], o[0]
                if gap_hi > gap_lo:
                    print(f"    -> clear gap {gap_lo:.2f} .. {gap_hi:.2f}; "
                          f"a threshold at {(gap_lo + gap_hi) / 2:.2f} separates them")
                else:
                    print("    -> NO clear gap on this feature.")

    line("Impact peak (g)", f"{IMPACT_G} / {IMPACT_STANDALONE_G} standalone",
         [r["peak_g"] for r in falls], [r["peak_g"] for r in others])
    line("Free-fall minimum (g)", FREEFALL_G,
         [r["min_g"] for r in falls], [r["min_g"] for r in others],
         higher_is_fall=False)

    fall_ratios = [r["calm_ratio"] for r in falls if r.get("calm_ratio") is not None]
    other_ratios = [r["calm_ratio"] for r in others if r.get("calm_ratio") is not None]
    if fall_ratios:
        line("Calm ratio", STILLNESS_MIN_CALM_RATIO, fall_ratios, other_ratios)

    fall_tilt = [r["tilt_deg"] for r in falls if r.get("tilt_deg") is not None]
    other_tilt = [r["tilt_deg"] for r in others if r.get("tilt_deg") is not None]
    if fall_tilt:
        line("Tilt (deg)", ORIENTATION_MIN_DEG, fall_tilt, other_tilt)

    print()


def report_outcome(rows, is_fall):
    """Sensitivity and false alarms as counted fractions, not percentages alone."""
    cut = sum(1 for r in rows if r.get("truncated"))
    falls = [r for r, f in zip(rows, is_fall) if f and not r.get("truncated")]
    others = [r for r, f in zip(rows, is_fall) if not f and not r.get("truncated")]
    if not falls and not others:
        return

    print("=" * 70)
    print("OUTCOME with the thresholds currently in app_config.h")
    print("=" * 70)
    if cut:
        print(f"  {cut} event(s) excluded: the capture ended before their "
              f"{CONFIRM_WINDOW_MS} ms confirmation window closed.")
        print("  Leave the log running a few seconds after the last movement.\n")

    caught = sum(1 for r in falls if r["verdict"] == "ALERT")
    false_alarms = sum(1 for r in others if r["verdict"] == "ALERT")

    if falls:
        pct = 100.0 * caught / len(falls)
        print(f"  falls detected      {caught}/{len(falls)}  ({pct:.0f}%)")
        missed = [(i + 1, r) for i, (r, f) in enumerate(zip(rows, is_fall))
                  if f and not r.get("truncated") and r["verdict"] != "ALERT"]
        for idx, r in missed:
            # .get, because an event that never entered confirmation has no
            # failed check to name -- the reason it was missed is the verdict
            # itself ("no entry"), and indexing "fail" directly raised KeyError
            # on exactly those events.
            why = r.get("fail") or r["verdict"]
            print(f"    event {idx} missed: {why}")
    if others:
        print(f"  false alarms        {false_alarms}/{len(others)} non-fall events")
        for i, (r, f) in enumerate(zip(rows, is_fall)):
            if not f and r["verdict"] == "ALERT":
                print(f"    event {i + 1} fired at {r['peak_g']:.2f} g "
                      f"-- what were you doing at t={r['t'] / 1000.0:.0f} s?")

    # Sample size honesty. At n=20 a "90%" is one event away from "85%", and a
    # report that prints only the percentage invites a confidence nobody has.
    if falls and len(falls) < 30:
        n = len(falls)
        p = caught / n
        se = math.sqrt(max(p * (1 - p), 1e-9) / n)
        lo, hi = max(0.0, p - 1.96 * se), min(1.0, p + 1.96 * se)
        print(f"\n  n={n}. The 95% interval around {100 * p:.0f}% runs "
              f"{100 * lo:.0f}%..{100 * hi:.0f}%.")
        print("  Report this as a fraction with n stated, not as a bare percentage.")

    print()


def report_firmware(firmware):
    """What the device itself decided, as a cross-check on the replay above."""
    if not firmware:
        return
    print("=" * 70)
    print("FIRMWARE DECISIONS (from the device, not replayed)")
    print("=" * 70)
    for line in firmware:
        print("  " + line.strip())
    print()


def write_events_csv(samples, events, is_fall, outdir):
    """One CSV per event, for training and for plotting elsewhere."""
    os.makedirs(outdir, exist_ok=True)
    for i, (start, trigger, end) in enumerate(events):
        label = "fall" if is_fall[i] else "other"
        path = os.path.join(outdir, f"event_{i + 1:03d}_{label}.csv")
        with open(path, "w", encoding="utf-8") as fh:
            fh.write("t_ms,rel_ms,ax,ay,az,gx,gy,gz,total_g,state\n")
            t0 = samples[trigger].t
            for k in range(start, end + 1):
                s = samples[k]
                fh.write(f"{s.t},{s.t - t0},{s.ax},{s.ay},{s.az},"
                         f"{s.gx},{s.gy},{s.gz},{magnitude_g(s):.4f},{s.state}\n")
    print(f"  wrote {len(events)} event file(s) to {outdir}/\n")


def plot(samples, events, is_fall, rows, outdir):
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        print("  matplotlib not available; skipping plots.\n")
        return

    os.makedirs(outdir, exist_ok=True)

    # Overview: the whole capture with events marked, so the splitting itself
    # can be checked before anything derived from it is believed.
    t = [s.t / 1000.0 for s in samples]
    g = [magnitude_g(s) for s in samples]
    fig, ax = plt.subplots(figsize=(16, 4))
    ax.plot(t, g, linewidth=0.4, color="#333")
    for i, (start, trigger, end) in enumerate(events):
        ax.axvspan(samples[start].t / 1000.0, samples[end].t / 1000.0,
                   color="#d33" if is_fall[i] else "#39c", alpha=0.18)
        ax.text(samples[trigger].t / 1000.0, max(g) * 0.95, str(i + 1),
                fontsize=7, ha="center")
    for y, label, c in ((IMPACT_G, "impact 2.5g", "#d33"),
                        (IMPACT_STANDALONE_G, "standalone 3.2g", "#a11"),
                        (FREEFALL_G, "free-fall 0.4g", "#39c"),
                        (1.0, "rest 1g", "#999")):
        ax.axhline(y, color=c, linestyle="--", linewidth=0.7)
        ax.text(t[0], y, " " + label, fontsize=6, color=c, va="bottom")
    ax.set_xlabel("time (s)")
    ax.set_ylabel("|a| (g)")
    ax.set_title("Whole capture -- red = labelled fall, blue = other event")
    fig.tight_layout()
    fig.savefig(os.path.join(outdir, "overview.png"), dpi=110)
    plt.close(fig)

    # Per-event detail, aligned on the trigger so events can be overlaid by eye.
    for i, (start, trigger, end) in enumerate(events):
        seg = samples[start:end + 1]
        rel = [(s.t - samples[trigger].t) / 1000.0 for s in seg]
        fig, (a1, a2) = plt.subplots(2, 1, figsize=(9, 5), sharex=True)
        a1.plot(rel, [magnitude_g(s) for s in seg], color="#111", linewidth=0.9)
        a1.axhline(IMPACT_G, color="#d33", linestyle="--", linewidth=0.7)
        a1.axhline(FREEFALL_G, color="#39c", linestyle="--", linewidth=0.7)
        a1.axhline(1.0, color="#999", linestyle=":", linewidth=0.7)
        a1.set_ylabel("|a| (g)")
        r = rows[i]
        a1.set_title(f"event {i + 1} -- {'FALL' if is_fall[i] else 'other'} -- "
                     f"peak {r['peak_g']:.2f} g, entry {r['entry'] or '-'}, "
                     f"{r['verdict']}")
        for axis, colour in (("ax", "#d33"), ("ay", "#2a2"), ("az", "#39c")):
            a2.plot(rel, [getattr(s, axis) / LSB_PER_G for s in seg],
                    linewidth=0.7, color=colour, label=axis)
        a2.legend(fontsize=7, ncol=3)
        a2.set_xlabel("time from trigger (s)")
        a2.set_ylabel("axis (g)")
        fig.tight_layout()
        label = "fall" if is_fall[i] else "other"
        fig.savefig(os.path.join(outdir, f"event_{i + 1:03d}_{label}.png"), dpi=110)
        plt.close(fig)

    print(f"  wrote {len(events) + 1} plot(s) to {outdir}/\n")


def main():
    ap = argparse.ArgumentParser(
        description="Analyse a FALLCSV capture from the CardioGuardAI watch.")
    ap.add_argument("logfile", help="serial capture containing FALLCSV lines")
    ap.add_argument("--falls", type=int, default=0, metavar="N",
                    help="label the N largest events as falls (default 0: "
                         "treat everything as ordinary movement)")
    ap.add_argument("--plot", metavar="DIR",
                    help="write PNG plots and per-event CSVs to DIR")
    args = ap.parse_args()

    if not os.path.isfile(args.logfile):
        sys.exit(f"No such file: {args.logfile}")

    samples, firmware, skipped = parse(args.logfile)
    if not check_health(samples, firmware, skipped):
        return 1

    events = find_events(samples)
    if not events:
        print("No events found: nothing in the capture passed "
              f"{EVENT_ENTER_G} g.")
        print("If you were moving, the IMU may not be reading correctly.\n")
        report_firmware(firmware)
        return 0

    rows, is_fall = report_events(samples, events, args.falls)
    report_thresholds(rows, is_fall)
    report_outcome(rows, is_fall)
    report_firmware(firmware)

    if args.plot:
        write_events_csv(samples, events, is_fall, args.plot)
        plot(samples, events, is_fall, rows, args.plot)

    return 0


if __name__ == "__main__":
    sys.exit(main())
