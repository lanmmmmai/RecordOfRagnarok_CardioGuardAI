#!/usr/bin/env python3
"""Build TFT_eSPI smooth-font (.vlw) headers with full Vietnamese coverage.

Why this exists
---------------
The board only had ASCII fonts loaded (LOAD_GLCD / LOAD_FONT2 / LOAD_FONT4), so
every accented Vietnamese character rendered as garbage. TFT_eSPI's "smooth
font" format stores an arbitrary, non-contiguous set of Unicode code points with
a lookup table, which is exactly what Vietnamese needs -- its precomposed
letters are scattered across U+00C0..U+1EF9, so an Adafruit GFX font (which
requires one contiguous range) would have to carry ~7,900 glyphs to reach them.

The source face is DejaVu Sans Bold, which ships with matplotlib and is under
the Bitstream Vera license -- free to embed in a product that ships. Arial and
Tahoma also cover Vietnamese but are licensed to Microsoft; embedding either one
in firmware that goes out the door is a licensing problem, not a technical one.

Run:  python tools/make_vlw.py
Output: include/fonts/vn_font_{small,medium,large}.h

The .vlw layout is documented in TFT_eSPI/Extensions/Smooth_font.cpp. All
integers are big-endian int32.
"""

import os
import sys

from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
OUT_DIR = os.path.join(HERE, "..", "include", "fonts")

# matplotlib is only a carrier for the .ttf here; nothing at build time needs it.
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

# ---------------------------------------------------------------------------
# Character set
#
# The full Vietnamese alphabet goes in, not just the letters today's strings
# happen to use. A missing glyph renders as nothing at all -- a silent hole in
# a sentence -- so the cost of a few extra kilobytes of flash buys the guarantee
# that any Vietnamese string added later just works.
# ---------------------------------------------------------------------------

ASCII = "".join(chr(c) for c in range(0x20, 0x7F))

# The 134 precomposed Vietnamese letters: the 12 base vowels with every tone
# mark, plus the six modified letters and d-with-stroke.
VIETNAMESE = (
    "ÀÁÂÃÈÉÊÌÍÒÓÔÕÙÚÝ"
    "àáâãèéêìíòóôõùúý"
    "ĂăĐđĨĩŨũƠơƯưƯ"
    "ẠạẢảẤấẦầẨẩẪẫẬậẮắẰằẲẳẴẵẶặ"
    "ẸẹẺẻẼẽẾếỀềỂểỄễỆệ"
    "ỈỉỊịỌọỎỏỐốỒồỔổỖỗỘộ"
    "ỚớỜờỞởỠỡỢợ"
    "ỤụỦủỨứỪừỬửỮữỰự"
    "ỲỳỴỵỶỷỸỹ"
)

# Arrows and the degree sign are used in on-screen hints and vitals labels.
SYMBOLS = "→←°"

CHARSET = sorted(set(ASCII + VIETNAMESE + SYMBOLS))

# Pixel sizes chosen to sit close to the built-in fonts they replace:
# font 1 (8 px) -> 12 px, because Vietnamese tone marks are simply unreadable
# below about 11 px; font 2 (16 px) -> 16 px; font 4 (26 px) -> 26 px.
SIZES = [
    ("small",  12, "VN_FONT_SMALL"),
    ("medium", 17, "VN_FONT_MEDIUM"),
    ("large",  27, "VN_FONT_LARGE"),
]


def be32(value):
    """Big-endian int32, the byte order TFT_eSPI's readInt32() expects."""
    return int(value & 0xFFFFFFFF).to_bytes(4, "big")


def build_vlw(font_path, px):
    font = ImageFont.truetype(font_path, px)

    glyphs = []
    for ch in CHARSET:
        advance = round(font.getlength(ch))
        bbox = font.getbbox(ch)  # relative to the 'la' origin: y grows downward

        if ch == " " or bbox is None or bbox[2] <= bbox[0] or bbox[3] <= bbox[1]:
            # Blank glyph. Width and height stay zero; only the advance matters.
            glyphs.append({"code": ord(ch), "w": 0, "h": 0,
                           "adv": advance, "dx": 0, "dy": 0, "bmp": b""})
            continue

        x0, y0, x1, y1 = bbox
        w, h = x1 - x0, y1 - y0

        img = Image.new("L", (w, h), 0)
        ImageDraw.Draw(img).text((-x0, -y0), ch, font=font, fill=255)

        ascent, _ = font.getmetrics()
        glyphs.append({
            "code": ord(ch),
            "w": w,
            "h": h,
            "adv": advance,
            "dx": x0,
            # dY is measured upward from the baseline to the top of the bitmap.
            "dy": ascent - y0,
            "bmp": img.tobytes(),
        })

    # TFT_eSPI derives the line box from the header ascent plus the largest
    # descent it finds while parsing. Taking the ascent from the tallest glyph
    # rather than the face metrics keeps the box tight, so MC_DATUM centring
    # lands where the old ASCII fonts did, while still leaving room for the
    # stacked tone marks on characters like "Ế".
    max_ascent = max(g["dy"] for g in glyphs)
    max_descent = max(g["h"] - g["dy"] for g in glyphs)

    out = bytearray()
    out += be32(len(glyphs))    # glyph count
    out += be32(11)             # format version
    out += be32(px)             # nominal size
    out += be32(0)              # deprecated mboxY
    out += be32(max_ascent)
    out += be32(max_descent)

    for g in glyphs:
        out += be32(g["code"])
        out += be32(g["h"])
        out += be32(g["w"])
        out += be32(g["adv"])
        out += be32(g["dy"])
        out += be32(g["dx"])
        out += be32(0)          # padding, ignored by the library

    for g in glyphs:
        out += g["bmp"]

    # Trailer: font name, PostScript name, anti-aliased flag. The library never
    # reads past the bitmaps, but a well-formed file stays inspectable by the
    # other tools that understand .vlw.
    name = b"DejaVuSansBold"
    out += bytes([len(name)]) + name
    out += bytes([len(name)]) + name
    out += bytes([1])

    return bytes(out), max_ascent, max_descent


def write_header(path, symbol, blob, px, max_ascent, max_descent):
    guard = symbol + "_H"
    lines = [
        "// Generated by tools/make_vlw.py -- do not edit by hand.",
        "//",
        "// DejaVu Sans Bold at %d px, TFT_eSPI smooth-font (.vlw) format." % px,
        "// %d glyphs, %d bytes, line box %d px (ascent %d + descent %d)."
        % (len(CHARSET), len(blob), max_ascent + max_descent, max_ascent, max_descent),
        "// Bitstream Vera license -- redistributable in a shipped product.",
        "",
        "#ifndef %s" % guard,
        "#define %s" % guard,
        "",
        "#include <pgmspace.h>",
        "",
        "const uint8_t %s[] PROGMEM = {" % symbol,
    ]

    for i in range(0, len(blob), 16):
        chunk = blob[i:i + 16]
        lines.append("    " + "".join("0x%02X," % b for b in chunk))

    lines += ["};", "", "#endif  // %s" % guard, ""]

    with open(path, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines))


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    print("Source face: %s" % FONT_PATH)
    print("%d glyphs per size\n" % len(CHARSET))

    total = 0
    for name, px, symbol in SIZES:
        blob, asc, desc = build_vlw(FONT_PATH, px)
        path = os.path.abspath(os.path.join(OUT_DIR, "vn_font_%s.h" % name))
        write_header(path, symbol, blob, px, asc, desc)
        total += len(blob)
        print("%-7s %2d px  %7d bytes  line box %2d px  -> %s"
              % (name, px, len(blob), asc + desc, os.path.relpath(path, HERE + "/..")))

    print("\nTotal flash cost: %.1f KB" % (total / 1024.0))


if __name__ == "__main__":
    main()
