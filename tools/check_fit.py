#!/usr/bin/env python3
"""Check that every on-screen string fits inside the round 240 px display.

Why this exists
---------------
Switching from the ASCII fonts to the Vietnamese smooth fonts made every line
box taller (8->17, 16->24, 26->37 px) and every accented string wider, on a
display that is not a rectangle: at height y the usable half-chord is only
sqrt(120^2 - (y-120)^2), so a footer at y=224 has just ~60 px of width to work
with, not 240. Eyeballing that on the device is slow and unreliable, so this
script measures it the same way the firmware will render it.

It parses each src/ui/screen_*.cpp for drawString() calls, tracks which vnFont()
size and which text datum are in effect at that point, measures every string
literal in the text argument with the same DejaVu Sans Bold face used to build
the .vlw headers, and flags anything that runs past the circle or collides with
another line box.

Two deliberate limits:

* Only string literals are measured. A runtime-formatted string (clock, BPM,
  the notification text pushed in from the dispatcher) is listed as UNCHECKED
  with a worst-case estimate where one is obvious, but still needs a human.
* Overlap detection pairs two boxes only when they overlap horizontally as
  well as vertically, and only when they can actually be drawn in the same
  frame -- strings sitting in opposite branches of the same if/else chain are
  never on screen together, and reporting those buries the real collisions.

Run:  python tools/check_fit.py
"""

import math
import os
import re
import sys

from PIL import ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
UI_DIR = os.path.join(HERE, "..", "src", "ui")

try:
    import matplotlib
    FONT_PATH = os.path.join(
        os.path.dirname(matplotlib.__file__),
        "mpl-data", "fonts", "ttf", "DejaVuSans-Bold.ttf",
    )
except ImportError:
    FONT_PATH = None

if not FONT_PATH or not os.path.isfile(FONT_PATH):
    sys.exit("DejaVuSans-Bold.ttf not found. Install matplotlib or edit FONT_PATH.")

# Must match SIZES in make_vlw.py and the line boxes it reported.
PX = {"VN_SMALL": 12, "VN_MEDIUM": 17, "VN_LARGE": 27}
LINE_BOX = {"VN_SMALL": 17, "VN_MEDIUM": 24, "VN_LARGE": 37}
FONTS = {name: ImageFont.truetype(FONT_PATH, size) for name, size in PX.items()}

# Coordinate macros used by the screens.
CONSTS = {"SCREEN_CENTER_X": 120, "SCREEN_CENTER_Y": 120}

R = 120       # display radius
MARGIN = 3    # keep glyphs off the bezel ring itself

CALL = re.compile(r'\b(vnFont|drawString|setTextDatum)\s*\(', re.S)
STRING = re.compile(r'"((?:[^"\\]|\\.)*)"')


def split_args(src, open_paren):
    """Return (arg_list, index_past_close) for a call whose '(' is at open_paren."""
    depth, args, cur, i = 0, [], "", open_paren
    in_str = False
    while i < len(src):
        c = src[i]
        if in_str:
            cur += c
            if c == "\\":
                cur += src[i + 1]
                i += 2
                continue
            if c == '"':
                in_str = False
            i += 1
            continue
        if c == '"':
            in_str = True
            cur += c
        elif c == "(":
            depth += 1
            if depth > 1:
                cur += c
        elif c == ")":
            depth -= 1
            if depth == 0:
                args.append(cur.strip())
                return args, i + 1
            cur += c
        elif c == "," and depth == 1:
            args.append(cur.strip())
            cur = ""
        else:
            cur += c
        i += 1
    return args, i


def match_brace(src, i):
    """Index just past the '}' closing the '{' at index i."""
    depth = 0
    while i < len(src):
        if src[i] == "{":
            depth += 1
        elif src[i] == "}":
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    return len(src)


def skip_ws(src, i):
    while i < len(src) and src[i].isspace():
        i += 1
    return i


def find_branches(src):
    """List of (start, end, chain_id, branch_id) for every if/else-if/else body.

    Two draw calls whose branch signatures share a chain but differ in branch
    are mutually exclusive: only one of them is ever on screen.
    """
    branches = []
    chain = 0
    for m in re.finditer(r'\bif\s*\(', src):
        # Skip the "if" of an "else if" -- it is picked up with its chain below.
        before = src[max(0, m.start() - 6):m.start()].rstrip()
        if before.endswith("else"):
            continue

        chain += 1
        i = match_paren(src, m.end() - 1)
        branch = 0
        while True:
            i = skip_ws(src, i)
            if i >= len(src) or src[i] != "{":
                break                                   # single-statement body
            end = match_brace(src, i)
            branches.append((i, end, chain, branch))

            j = skip_ws(src, end)
            if not src.startswith("else", j):
                break
            j = skip_ws(src, j + 4)
            if src.startswith("if", j):
                j = match_paren(src, src.index("(", j))
            i = j
            branch += 1
    return branches


def match_paren(src, i):
    depth = 0
    while i < len(src):
        if src[i] == "(":
            depth += 1
        elif src[i] == ")":
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    return len(src)


def signature(branches, offset):
    return {(c, b) for (s, e, c, b) in branches if s <= offset < e}


def exclusive(sig_a, sig_b):
    chains_a = {c: b for (c, b) in sig_a}
    return any(c in chains_a and chains_a[c] != b for (c, b) in sig_b)


def as_int(expr):
    expr = expr.strip()
    if expr in CONSTS:
        return CONSTS[expr]
    try:
        return int(expr)
    except ValueError:
        return None


def unescape(text):
    """Decode C escapes without mangling the UTF-8 already in the literal."""
    return (text.replace('\\"', '"')
                .replace("\\n", "\n")
                .replace("\\\\", "\\"))


def text_width(text, size_name):
    x0, _, x1, _ = FONTS[size_name].getbbox(text)
    return x1 - x0


def half_chord(y):
    dy = abs(y - R)
    return 0.0 if dy >= R else math.sqrt(R * R - dy * dy)


def check_file(path):
    with open(path, encoding="utf-8") as fh:
        src = fh.read()

    size = "VN_MEDIUM"   # initVnFonts() leaves the medium font selected
    datum = "MC_DATUM"
    problems, unchecked, boxes = [], [], []
    branches = find_branches(src)

    pos = 0
    while True:
        m = CALL.search(src, pos)
        if not m:
            break
        fn = m.group(1)
        args, pos = split_args(src, m.end() - 1)
        line = src.count("\n", 0, m.start()) + 1

        if fn == "vnFont":
            token = args[-1].strip() if args else ""
            if token in PX:
                size = token
            continue

        if fn == "setTextDatum":
            datum = args[-1].strip() if args else datum
            continue

        # drawString(spr-less: text, x, y[, font]) -- the sprite is the receiver.
        if len(args) < 3:
            continue
        expr = args[0]
        x, y = as_int(args[1]), as_int(args[2])
        if x is None or y is None:
            unchecked.append((line, expr, "coords not literal"))
            continue

        literals = [unescape(s) for s in STRING.findall(expr)]
        if not literals:
            unchecked.append((line, expr, "runtime-formatted"))
            continue

        for text in literals:
            w = text_width(text, size)
            h = LINE_BOX[size]

            if datum.endswith("ML_DATUM") or datum == "ML_DATUM":
                left, right = x, x + w
            elif datum == "MR_DATUM":
                left, right = x - w, x
            else:                                    # MC_DATUM
                left, right = x - w / 2, x + w / 2
            top, bottom = y - h / 2, y + h / 2

            # The circle bites hardest at whichever corner of the box sits
            # nearer the top or bottom edge, so test the box against both.
            worst = min(half_chord(top), half_chord(bottom)) - MARGIN
            if left < R - worst or right > R + worst:
                over = max((R - worst) - left, right - (R + worst))
                problems.append(
                    f"  line {line:>3}  OVERFLOW {over:4.0f}px  {size:<9} "
                    f'y={y:<4} "{text}"'
                )
            boxes.append((line, left, right, top, bottom, text,
                          signature(branches, m.start())))

    # Vertical collisions, but only between boxes that overlap horizontally too
    # (several screens put two or three labels side by side on one row) and can
    # be on screen at the same time.
    for i, a in enumerate(boxes):
        for b in boxes[i + 1:]:
            if a[0] == b[0] or exclusive(a[6], b[6]):
                continue
            h_overlap = a[1] < b[2] - 1 and b[1] < a[2] - 1
            v_overlap = a[3] < b[4] - 1 and b[3] < a[4] - 1
            if h_overlap and v_overlap:
                problems.append(
                    f'  lines {a[0]}/{b[0]}  OVERLAP   "{a[5]}" and "{b[5]}"'
                )

    return problems, unchecked


def main():
    files = sorted(
        os.path.join(UI_DIR, f)
        for f in os.listdir(UI_DIR)
        if f.startswith("screen_") and f.endswith(".cpp")
    )

    total = 0
    for path in files:
        problems, unchecked = check_file(path)
        total += len(problems)
        name = os.path.basename(path)
        if problems:
            print(f"\n{name}")
            for p in problems:
                print(p)
        if unchecked:
            print(f"\n{name}  -- unchecked (needs a human eye):")
            for line, expr, why in unchecked:
                print(f"  line {line:>3}  {expr[:52]:<52} [{why}]")

    print("\n" + "-" * 62)
    print("OK: every literal fits." if total == 0 else f"{total} problem(s) found.")
    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main())
