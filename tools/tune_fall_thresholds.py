#!/usr/bin/env python3
"""Search the fall thresholds against every recorded capture at once, and train
a small decision tree on the same events.

Why this exists, separately from analyse_fall_log.py
----------------------------------------------------
That script answers "what did the current thresholds do to this one file". It
is a microscope. Setting thresholds needs the opposite: one number per candidate
setting, across every file, so that a change that rescues one capture and ruins
another is visible as a change rather than as two separate readings taken an
hour apart.

The specific failure that motivated it: the same firmware, same 30 deg tilt
threshold, caught 7 of 7 falls in a capture recorded with the arms out and 1 of
6 with the arms down. Tilt was measuring arm posture, not falling. A sweep makes
that kind of dependence visible immediately, because a feature that only works
in one file shows up as a split score.

It reuses analyse_fall_log's parser and replay so the physics cannot drift
between the two tools. The thresholds there are module globals, so this
overrides them in place around each replay -- ugly, but far safer than a second
copy of the phase logic that would need keeping in step by hand.

Run
---
    python tools/tune_fall_thresholds.py \\
        log_nga_2026-08-16.txt:6 log_nga2_2026-08-16.txt:7

    ... --sweep          # threshold grid search
    ... --tree           # decision tree with leave-one-out CV
    ... --dump feat.csv  # write the feature table

Each capture is given as PATH:N where N is how many of its events are real
falls. The N largest events by peak acceleration are labelled falls, exactly as
analyse_fall_log does -- read its printed table first and make sure that
labelling matches what actually happened.
"""

import argparse
import itertools
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import analyse_fall_log as A  # noqa: E402


# ---------------------------------------------------------------------------
# Loading
# ---------------------------------------------------------------------------

def load(spec):
    """Parse "path:n_falls" into (name, samples, events, labels).

    labels[i] is True when event i is one of the n_falls largest by peak, which
    is the same rule analyse_fall_log prints its table under.
    """
    if ":" not in spec:
        sys.exit(f"expected PATH:N_FALLS, got {spec!r}")
    path, _, n = spec.rpartition(":")
    n_falls = int(n)

    samples, firmware, skipped = A.parse(path)
    if not samples:
        sys.exit(f"{path}: no FALLCSV lines -- was the firmware built with "
                 f"FALL_LOG_RAW_SAMPLES=1?")

    events = A.find_events(samples)
    peaks = [(max(A.magnitude_g(samples[k]) for k in range(s, e + 1)), i)
             for i, (s, _, e) in enumerate(events)]
    fall_idx = {i for _, i in sorted(peaks, reverse=True)[:n_falls]}
    labels = [i in fall_idx for i in range(len(events))]

    return dict(name=os.path.basename(path), samples=samples, events=events,
                labels=labels, skipped=skipped, firmware=firmware)


# ---------------------------------------------------------------------------
# Replaying under arbitrary thresholds
# ---------------------------------------------------------------------------

# Only these are swept. Everything else in analyse_fall_log stays as the
# firmware has it; a sweep over every constant at once would overfit 13 falls
# into a lookup table.
TUNABLE = ("IMPACT_G", "IMPACT_STANDALONE_G", "STILLNESS_MIN_CALM_RATIO",
           "ORIENTATION_MIN_DEG", "FREEFALL_G")


def replay_all(cap, **overrides):
    """Replay one capture's events with the named thresholds overridden.

    The overrides are written into analyse_fall_log's globals and restored
    afterwards, so a caller that raises does not leave the module poisoned for
    the next call.
    """
    for k in overrides:
        if k not in TUNABLE:
            raise KeyError(f"{k} is not tunable")
    saved = {k: getattr(A, k) for k in overrides}
    try:
        for k, v in overrides.items():
            setattr(A, k, v)
        return [A.replay(cap["samples"], *e) for e in cap["events"]]
    finally:
        for k, v in saved.items():
            setattr(A, k, v)


def score(caps, **overrides):
    """Return (tp, fn, fp, tn, per_capture_detail) over every capture.

    An event counts as detected when replay's verdict is ALERT. Truncated
    events -- ones whose confirmation window ran off the end of the recording --
    are excluded from both counts rather than guessed at, which is why the four
    numbers may not sum to the event total.
    """
    tp = fn = fp = tn = 0
    detail = []
    for cap in caps:
        rows = replay_all(cap, **overrides)
        c_tp = c_fn = c_fp = c_tn = 0
        for r, is_fall in zip(rows, cap["labels"]):
            if r.get("truncated"):
                continue
            fired = r["verdict"] == "ALERT"
            if is_fall:
                c_tp += fired
                c_fn += not fired
            else:
                c_fp += fired
                c_tn += not fired
        tp, fn, fp, tn = tp + c_tp, fn + c_fn, fp + c_fp, tn + c_tn
        detail.append((cap["name"], c_tp, c_fn, c_fp, c_tn))
    return tp, fn, fp, tn, detail


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def report_current(caps):
    print("=" * 78)
    print(" CURRENT THRESHOLDS, every capture")
    print("=" * 78)
    for k in TUNABLE:
        print(f"  {k:<28} {getattr(A, k)}")
    print()

    tp, fn, fp, tn, detail = score(caps)
    print(f"  {'capture':<28} {'falls caught':>14} {'false alarms':>14}")
    for name, c_tp, c_fn, c_fp, c_tn in detail:
        print(f"  {name:<28} {f'{c_tp}/{c_tp + c_fn}':>14} "
              f"{f'{c_fp}/{c_fp + c_tn}':>14}")
    print(f"  {'TOTAL':<28} {f'{tp}/{tp + fn}':>14} {f'{fp}/{fp + tn}':>14}")
    print()


def report_features(caps):
    """Print every event's features, falls and non-falls side by side.

    This is the table that decides whether any threshold can work at all: if a
    feature's fall range overlaps its non-fall range completely, no cut point
    on that feature separates them and the sweep below is wasted effort.
    """
    print("=" * 78)
    print(" FEATURES PER EVENT")
    print("=" * 78)
    print(f"  {'capture':<22} {'#':>3} {'label':>6} {'peak':>7} {'min':>6} "
          f"{'entry':>5} {'calm':>6} {'tilt':>6}  verdict")
    for cap in caps:
        rows = replay_all(cap)
        for i, (r, is_fall) in enumerate(zip(rows, cap["labels"])):
            calm = (f"{r['calm_ratio']:.2f}"
                    if r.get("calm_ratio") is not None else "-")
            tilt = (f"{r['tilt_deg']:.0f}"
                    if r.get("tilt_deg") is not None else "-")
            print(f"  {cap['name'][:22]:<22} {i:>3} "
                  f"{'FALL' if is_fall else 'other':>6} "
                  f"{r['peak_g']:>7.2f} {r['min_g']:>6.2f} "
                  f"{str(r['entry'] or '-'):>5} {calm:>6} {tilt:>6}  "
                  f"{r['verdict']}")
    print()

    # Range overlap, stated explicitly rather than left to the reader.
    print("  separation (fall range vs other range):")
    for key, label, fmt in (("peak_g", "impact peak (g)", "{:.2f}"),
                            ("calm_ratio", "calm ratio", "{:.2f}"),
                            ("tilt_deg", "tilt (deg)", "{:.0f}")):
        f_vals, o_vals = [], []
        for cap in caps:
            for r, is_fall in zip(replay_all(cap), cap["labels"]):
                v = r.get(key)
                if v is None:
                    continue
                (f_vals if is_fall else o_vals).append(v)
        if not f_vals or not o_vals:
            print(f"    {label:<20} not enough labelled events")
            continue
        f_lo, f_hi = min(f_vals), max(f_vals)
        o_lo, o_hi = min(o_vals), max(o_vals)
        gap = "SEPARABLE" if f_lo > o_hi else "overlaps"
        print(f"    {label:<20} fall [{fmt.format(f_lo)}, {fmt.format(f_hi)}]"
              f"   other [{fmt.format(o_lo)}, {fmt.format(o_hi)}]   {gap}")
    print()


def report_sweep(caps, top=15):
    """Grid search. Ranked by falls caught first, false alarms second.

    Not by accuracy: the two errors are not equal here. A missed fall is an
    elderly person on the floor with nobody called; a false alarm is a message
    the family ignores. Ordering by recall and breaking ties on precision says
    that plainly, where a single accuracy number would hide it.
    """
    grid = {
        "IMPACT_G": [2.0, 2.5, 3.0],
        "IMPACT_STANDALONE_G": [2.5, 3.0, 3.2, 3.5, 4.0],
        "STILLNESS_MIN_CALM_RATIO": [0.50, 0.60, 0.70, 0.80],
        "ORIENTATION_MIN_DEG": [0.0, 5.0, 10.0, 20.0, 30.0],
    }
    keys = list(grid)
    results = []
    for combo in itertools.product(*(grid[k] for k in keys)):
        ov = dict(zip(keys, combo))
        if ov["IMPACT_STANDALONE_G"] < ov["IMPACT_G"]:
            # Path B is the fallback for impacts with no free-fall before them,
            # so it must be the stricter of the two or path A can never fire.
            continue
        tp, fn, fp, tn, _ = score(caps, **ov)
        results.append((tp, -fp, ov, (tp, fn, fp, tn)))

    results.sort(key=lambda r: (r[0], r[1]), reverse=True)

    print("=" * 78)
    print(f" SWEEP  ({len(results)} valid combinations)")
    print("=" * 78)
    print(f"  {'caught':>8} {'missed':>7} {'false':>6} {'impact':>7} "
          f"{'stand':>6} {'calm':>5} {'tilt':>5}")
    seen = set()
    shown = 0
    for tp, _, ov, (tp2, fn, fp, tn) in results:
        sig = (tp, fp)
        if sig in seen:
            continue          # one representative per outcome, not 40 ties
        seen.add(sig)
        print(f"  {tp:>8} {fn:>7} {fp:>6} "
              f"{ov['IMPACT_G']:>7.1f} {ov['IMPACT_STANDALONE_G']:>6.1f} "
              f"{ov['STILLNESS_MIN_CALM_RATIO']:>5.2f} "
              f"{ov['ORIENTATION_MIN_DEG']:>5.0f}")
        shown += 1
        if shown >= top:
            break
    print()
    return results


# ---------------------------------------------------------------------------
# Decision tree
# ---------------------------------------------------------------------------

def feature_rows(caps):
    """One row per event: (capture, index, features dict, label).

    Features are taken from an unmodified replay, so they describe the event
    rather than any particular threshold's opinion of it. Events that never
    reached the confirmation stage still contribute their peak and minimum,
    with the confirmation features filled in as neutral -- dropping them would
    delete exactly the ordinary movement the tree must learn to reject.
    """
    out = []
    for cap in caps:
        for i, (r, is_fall) in enumerate(zip(replay_all(cap), cap["labels"])):
            out.append((cap["name"], i, {
                "peak_g": r["peak_g"],
                "min_g": r["min_g"],
                "calm_ratio": r.get("calm_ratio") if r.get("calm_ratio") is not None else 0.0,
                "tilt_deg": r.get("tilt_deg") if r.get("tilt_deg") is not None else 0.0,
                "duration_ms": float(r["duration_ms"]),
            }, is_fall))
    return out


FEATURE_NAMES = ("peak_g", "min_g", "calm_ratio", "tilt_deg", "duration_ms")


def gini(labels):
    n = len(labels)
    if n == 0:
        return 0.0
    p = sum(labels) / n
    return 2 * p * (1 - p)


def best_split(rows, features):
    """Lowest weighted Gini over midpoints between adjacent observed values.

    Midpoints rather than the observed values themselves: a threshold sitting
    exactly on a training sample is decided by a floating-point comparison
    against a number that came from one particular fall, and the next fall's
    value lands on the other side for no physical reason.
    """
    best = None
    labels = [r[3] for r in rows]
    base = gini(labels)
    n = len(rows)
    for f in features:
        vals = sorted({r[2][f] for r in rows})
        for a, b in zip(vals, vals[1:]):
            thr = (a + b) / 2.0
            left = [r[3] for r in rows if r[2][f] <= thr]
            right = [r[3] for r in rows if r[2][f] > thr]
            if not left or not right:
                continue
            g = (len(left) * gini(left) + len(right) * gini(right)) / n
            if best is None or g < best[0]:
                best = (g, f, thr)
    if best is None or best[0] >= base - 1e-12:
        return None
    return best[1], best[2]


class Node:
    __slots__ = ("feature", "thr", "left", "right", "label", "n", "n_fall")

    def __init__(self):
        self.feature = self.thr = self.left = self.right = None
        self.label = False
        self.n = self.n_fall = 0


def build_tree(rows, features, depth, min_leaf=2):
    node = Node()
    node.n = len(rows)
    node.n_fall = sum(r[3] for r in rows)
    node.label = node.n_fall * 2 >= node.n
    if depth <= 0 or node.n_fall in (0, node.n) or node.n < 2 * min_leaf:
        return node
    sp = best_split(rows, features)
    if sp is None:
        return node
    f, thr = sp
    left = [r for r in rows if r[2][f] <= thr]
    right = [r for r in rows if r[2][f] > thr]
    if len(left) < min_leaf or len(right) < min_leaf:
        return node
    node.feature, node.thr = f, thr
    node.left = build_tree(left, features, depth - 1, min_leaf)
    node.right = build_tree(right, features, depth - 1, min_leaf)
    return node


def predict(node, feats):
    while node.feature is not None:
        node = node.left if feats[node.feature] <= node.thr else node.right
    return node.label


def print_tree(node, indent="    "):
    if node.feature is None:
        print(f"{indent}-> {'FALL' if node.label else 'not fall'} "
              f"({node.n_fall}/{node.n})")
        return
    print(f"{indent}if {node.feature} <= {node.thr:.4g}:")
    print_tree(node.left, indent + "    ")
    print(f"{indent}else:")
    print_tree(node.right, indent + "    ")


def report_tree(caps, depth, features):
    rows = feature_rows(caps)
    n_fall = sum(r[3] for r in rows)
    print("=" * 78)
    print(f" DECISION TREE  depth<={depth}  "
          f"{len(rows)} events ({n_fall} falls, {len(rows) - n_fall} other)")
    print(f" features: {', '.join(features)}")
    print("=" * 78)

    if n_fall < 2 or len(rows) - n_fall < 2:
        print("  not enough of one class to train or validate. Collect more.")
        print()
        return None

    # Leave-one-out, because 26-ish events cannot spare a held-out split. It is
    # optimistic on correlated samples -- seven falls from one recording session
    # are not seven independent falls -- so read it as an upper bound.
    correct = tp = fn = fp = tn = 0
    for i in range(len(rows)):
        train = rows[:i] + rows[i + 1:]
        t = build_tree(train, features, depth)
        got = predict(t, rows[i][2])
        want = rows[i][3]
        correct += got == want
        if want:
            tp, fn = tp + got, fn + (not got)
        else:
            fp, tn = fp + got, tn + (not got)
    print(f"  leave-one-out: {correct}/{len(rows)} correct   "
          f"falls caught {tp}/{tp + fn}   false alarms {fp}/{fp + tn}")

    # A second, harsher check: hold out an entire capture. Falls recorded in one
    # session share a wrist, a posture and a floor, so leave-one-out can score
    # well on a tree that only learned that session. This is the number that
    # says whether it generalises to a day it has not seen.
    if len(caps) > 1:
        print("  leave-one-capture-out:")
        for held in caps:
            train = [r for r in rows if r[0] != held["name"]]
            test = [r for r in rows if r[0] == held["name"]]
            if not train or not test:
                continue
            t = build_tree(train, features, depth)
            h_tp = sum(1 for r in test if r[3] and predict(t, r[2]))
            h_fn = sum(1 for r in test if r[3] and not predict(t, r[2]))
            h_fp = sum(1 for r in test if not r[3] and predict(t, r[2]))
            h_tn = sum(1 for r in test if not r[3] and not predict(t, r[2]))
            print(f"    hold out {held['name'][:28]:<30} "
                  f"caught {h_tp}/{h_tp + h_fn}   false {h_fp}/{h_fp + h_tn}")

    full = build_tree(rows, features, depth)
    print("\n  tree trained on everything:")
    print_tree(full)
    print()
    return full


def dump_features(caps, path):
    rows = feature_rows(caps)
    with open(path, "w", encoding="utf-8") as fh:
        fh.write("capture,event,label," + ",".join(FEATURE_NAMES) + "\n")
        for name, i, feats, label in rows:
            fh.write(f"{name},{i},{int(label)}," +
                     ",".join(f"{feats[f]:.4f}" for f in FEATURE_NAMES) + "\n")
    print(f"  wrote {len(rows)} rows to {path}\n")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("captures", nargs="+", metavar="PATH:N_FALLS")
    ap.add_argument("--sweep", action="store_true", help="threshold grid search")
    ap.add_argument("--tree", action="store_true", help="train a decision tree")
    ap.add_argument("--depth", type=int, default=3)
    ap.add_argument("--features", default=",".join(FEATURE_NAMES),
                    help="comma-separated subset for the tree")
    ap.add_argument("--dump", metavar="CSV", help="write the feature table")
    args = ap.parse_args()

    caps = [load(c) for c in args.captures]
    for cap in caps:
        print(f"  {cap['name']}: {len(cap['samples'])} samples, "
              f"{len(cap['events'])} events, {sum(cap['labels'])} labelled falls"
              + (f", {cap['skipped']} truncated lines" if cap["skipped"] else ""))
    print()

    report_current(caps)
    report_features(caps)
    if args.sweep:
        report_sweep(caps)
    if args.tree:
        feats = [f.strip() for f in args.features.split(",") if f.strip()]
        bad = [f for f in feats if f not in FEATURE_NAMES]
        if bad:
            sys.exit(f"unknown feature(s): {', '.join(bad)}")
        report_tree(caps, args.depth, feats)
    if args.dump:
        dump_features(caps, args.dump)


if __name__ == "__main__":
    main()
