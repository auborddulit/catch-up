# Logo « Catch-UP » en pixels (128×64 OLED) : source unique, dessinée à la main.
#   python3 tools/logo.py
# Écrit :
#   firmware/catch_up/logo.h   bitmaps XBM pour U8g2 (logo complet + bouteille seule)
#   firmware/catch_up/web_ui.h logo et icône de la page web (entre les marqueurs LOGO / ICONE)
#   docs/logo/catch-up.svg, catch-up-bouteille.svg, catch-up.png, catch-up-ecran.png
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# --- lettres « Catch- » (hauteur des capitales 19 px, traits 4 px vertical / 3 px horizontal)
C = """
..##########
.###########
############
#####.......
####........
####........
####........
####........
####........
####........
####........
####........
####........
####........
####........
#####.......
############
.###########
..##########"""
a = """
.########..
##########.
###....####
#......####
..#########
.##########
###########
####...####
####...####
####...####
####..#####
###########
.##########
..#####.###"""
t = """
........
..####..
..####..
..####..
..####..
########
########
########
..####..
..####..
..####..
..####..
..####..
..####..
..####..
..####..
..######
...#####
....####"""
c = """
..########
.#########
##########
####......
####......
####......
####......
####......
####......
####......
####......
##########
.#########
..########"""
h = """
####.......
####.......
####.......
####.......
####.......
####.####..
##########.
###########
#####..####
####...####
####...####
####...####
####...####
####...####
####...####
####...####
####...####
####...####
####...####"""
tiret = """
######
######
######"""
U = """
####...####
####...####
####...####
####...####
####...####
####...####
####...####
####...####
####...####
####...####
####...####
####...####
####...####
####...####
####...####
#####.#####
###########
.#########.
..#######.."""
P = """
########...
##########.
###########
####...####
####...####
####...####
####...####
###########
##########.
########...
####.......
####.......
####.......
####.......
####.......
####.......
####.......
####.......
####......."""

def g(s): return [r for r in s.strip("\n").split("\n")]

CAP_TOP = 28          # ligne du haut des capitales dans le logo (60 lignes)
H = 60
BW = 30               # largeur du corps de la bouteille


def bottle():
    """Bouteille souple 30 × 60, « UP » en creux (lignes 28 à 46)."""
    px = [[0] * BW for _ in range(H)]
    def row(y, x0, x1):
        for x in range(x0, x1 + 1): px[y][x] = 1
    # bouchon 12 px, coins arrondis
    for y in range(0, 6):
        ins = 1 if y in (0, 5) else 0
        row(y, 9 + ins, 20 - ins)
    # épaule (trapèze)
    for i, y in enumerate(range(8, 15)):
        ins = 7 - i
        row(y, ins, BW - 1 - ins)
    # corps, coins du bas arrondis
    bottom = {59: 3, 58: 2, 57: 1, 56: 1}
    for y in range(15, H):
        ins = bottom.get(y, 0)
        row(y, ins, BW - 1 - ins)
    # lettres en creux
    for gl, x0 in ((g(U), 3), (g(P), 16)):
        for dy, r in enumerate(gl):
            for dx, ch in enumerate(r):
                if ch == "#": px[CAP_TOP + dy][x0 + dx] = 0
    return px


def logo():
    glyphs = [(g(C), 0), (g(a), 5), (g(t), 0), (g(c), 5), (g(h), 0), (g(tiret), 10)]
    width = sum(len(gl[0]) for gl, _ in glyphs) + 2 * len(glyphs) + BW
    px = [[0] * width for _ in range(H)]
    x = 0
    for gl, dy0 in glyphs:
        for dy, r in enumerate(gl):
            for dx, ch in enumerate(r):
                if ch == "#": px[CAP_TOP + dy0 + dy][x + dx] = 1
        x += len(gl[0]) + 2
    b = bottle()
    for y in range(H):
        for dx in range(BW): px[y][x + dx] = b[y][dx]
    return px


def xbm(name, px):
    w, h = len(px[0]), len(px)
    out, line = [], []
    for y in range(h):
        for bx in range(0, w, 8):
            v = 0
            for i in range(8):
                if bx + i < w and px[y][bx + i]: v |= 1 << i
            line.append("0x%02x" % v)
    for i in range(0, len(line), 16): out.append("  " + ",".join(line[i:i + 16]) + ",")
    return (f"#define {name}_W {w}\n#define {name}_H {h}\n"
            f"static const unsigned char {name}_BITS[] PROGMEM = {{\n" + "\n".join(out) + "\n};\n")


def svg_path(px):
    d = []
    for y, r in enumerate(px):
        x = 0
        while x < len(r):
            if r[x]:
                x0 = x
                while x < len(r) and r[x]: x += 1
                d.append(f"M{x0} {y}h{x - x0}v1h-{x - x0}z")
            else:
                x += 1
    return "".join(d)


def svg(px, color="#000", pad=0, title="Catch-UP"):
    w, h = len(px[0]), len(px)
    W = max(w, h) if pad else w
    ox = (W - w) // 2
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="{-ox} 0 {W} {h}" shape-rendering="crispEdges">'
            f'<title>{title}</title><path fill="{color}" d="{svg_path(px)}"/></svg>')


def png(px, path, scale, fg=(0, 0, 0), bg=(255, 255, 255), frame=None):
    try:
        from PIL import Image
    except ImportError:
        print("Pillow absent : PNG non écrit"); return
    w, h = len(px[0]), len(px)
    W, H_ = frame or (w, h)
    im = Image.new("RGB", (W, H_), bg)
    ox, oy = (W - w) // 2, (H_ - h) // 2
    for y in range(h):
        for x in range(w):
            if px[y][x]: im.putpixel((ox + x, oy + y), fg)
    im.resize((W * scale, H_ * scale), Image.NEAREST).save(path)


def main():
    L, B = logo(), bottle()
    fw = os.path.join(ROOT, "firmware", "catch_up")
    with open(os.path.join(fw, "logo.h"), "w") as f:
        f.write("// logo.h — Logo « Catch-UP » en pixels (généré par tools/logo.py, ne pas modifier à la main).\n"
                "#pragma once\n#include <Arduino.h>\n\n" + xbm("LOGO", L) + "\n" + xbm("BOUTEILLE", B))
    # page web : logo en blanc (bandeau sombre) et icône d'onglet
    ui = os.path.join(fw, "web_ui.h")
    s = open(ui, encoding="utf-8").read()
    logo_tag = (f'<svg class="logo" viewBox="0 0 {len(L[0])} {len(L)}" shape-rendering="crispEdges" role="img" '
                f'aria-label="Catch-UP"><path fill="currentColor" d="{svg_path(L)}"/></svg>')
    s = re.sub(r"<!--LOGO-->.*?<!--/LOGO-->", lambda m: "<!--LOGO-->" + logo_tag + "<!--/LOGO-->", s, flags=re.S)
    icon = svg(B, "%231d2733", pad=1).replace('"', "'").replace("<", "%3C").replace(">", "%3E")
    s = re.sub(r"<!--ICONE-->.*?<!--/ICONE-->",
               lambda m: f'<!--ICONE--><link rel="icon" href="data:image/svg+xml,{icon}"><!--/ICONE-->', s, flags=re.S)
    open(ui, "w", encoding="utf-8").write(s)
    d = os.path.join(ROOT, "docs", "logo")
    os.makedirs(d, exist_ok=True)
    open(os.path.join(d, "catch-up.svg"), "w").write(svg(L))
    open(os.path.join(d, "catch-up-bouteille.svg"), "w").write(svg(B, pad=1))
    png(L, os.path.join(d, "catch-up.png"), 6)
    png(L, os.path.join(d, "catch-up-ecran.png"), 4, fg=(255, 255, 255), bg=(0, 0, 0), frame=(128, 64))
    print(f"logo {len(L[0])}×{len(L)}, bouteille {len(B[0])}×{len(B)} : fichiers écrits")


if __name__ == "__main__":
    main()
