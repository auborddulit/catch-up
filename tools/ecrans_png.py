# Assemble les captures out/ecrans/*.pbm (tests/test_ecrans) en une planche PNG.
#   python3 tools/ecrans_png.py tests/out/ecrans docs/ecrans.png
import sys, os
from PIL import Image, ImageDraw, ImageFont
src, dst = sys.argv[1], sys.argv[2]
names = [l.strip() for l in open(os.path.join(src, "liste.txt")) if l.strip()]
S, PAD, COLS = 3, 14, 3
W, H = 128 * S + 12, 64 * S + 12
try:
    fnt = ImageFont.truetype("/usr/share/fonts/opentype/inter/Inter-Medium.otf", 15)
except Exception:
    fnt = ImageFont.load_default()
rows = (len(names) + COLS - 1) // COLS
sheet = Image.new("RGB", (COLS * (W + PAD) + PAD, rows * (H + 34 + PAD) + PAD), "white")
d = ImageDraw.Draw(sheet)
for i, n in enumerate(names):
    lines = open(os.path.join(src, n + ".pbm")).read().split("\n")[2:66]
    im = Image.new("RGB", (W, H), (44, 44, 42))
    scr = Image.new("RGB", (128, 64), (8, 8, 8))
    for y, row in enumerate(lines):
        for x, c in enumerate(row[:128]):
            if c == "1":
                scr.putpixel((x, y), (181, 212, 244))
    im.paste(scr.resize((128 * S, 64 * S), Image.NEAREST), (6, 6))
    cx, cy = PAD + (i % COLS) * (W + PAD), PAD + (i // COLS) * (H + 34 + PAD)
    sheet.paste(im, (cx, cy))
    label = n.split("-", 1)[1].replace("-", " ")
    d.text((cx, cy + H + 8), f"{n.split('-')[0]}. {label}", fill=(20, 20, 19), font=fnt)
sheet.save(dst)
# chaque écran seul (pour la fiche opérateur) : docs/ecrans/<nom>.png
each = os.path.join(os.path.dirname(dst), "ecrans")
os.makedirs(each, exist_ok=True)
for n in names:
    lines = open(os.path.join(src, n + ".pbm")).read().split("\n")[2:66]
    scr = Image.new("RGB", (128, 64), (8, 8, 8))
    for y, row in enumerate(lines):
        for x, c in enumerate(row[:128]):
            if c == "1":
                scr.putpixel((x, y), (181, 212, 244))
    scr.resize((128 * 4, 64 * 4), Image.NEAREST).save(os.path.join(each, n + ".png"))
print(dst, sheet.size)
