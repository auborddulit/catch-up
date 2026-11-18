# Génère docs/cablage.svg et docs/cablage.png (plan de câblage, firmware v1.2)
#   python3 tools/cablage_svg.py
import cairosvg
W, H = 1180, 860
o = []
a = o.append
a(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" font-family="Arial, Helvetica, sans-serif" font-size="13">')
a('<rect width="100%" height="100%" fill="#fff"/>')
a('<style>.box{fill:#f7f9fc;stroke:#334;stroke-width:1.5}.esp{fill:#e8f0fe;stroke:#1a4fa0;stroke-width:2}'
  '.r{stroke:#d32f2f;stroke-width:2;fill:none}.k{stroke:#222;stroke-width:2;fill:none}.b{stroke:#1565c0;stroke-width:2;fill:none}'
  '.p{stroke:#6a1b9a;stroke-width:2;fill:none}.o{stroke:#e08600;stroke-width:2;fill:none}.g{stroke:#2e7d32;stroke-width:2;fill:none}'
  '.t{fill:#111}.s{font-size:11px;fill:#444}.h{font-size:15px;font-weight:bold}.res{fill:#fff;stroke:#a15c00;stroke-width:1.5}</style>')
a('<text x="20" y="28" class="h">compteur-up — plan de câblage (ESP32 DevKit, firmware v1.2)</text>')
a('<text x="20" y="46" class="s">Rouge : 3,3 V · Noir : GND · Bleu : 1-Wire et boutons · Violet : I2C · Orange : SPI (carte SD + écran) · Vert : 5 V. Toutes les masses sont communes.</text>')

# ESP32
ex, ey, ew, eh = 430, 80, 230, 700
a(f'<rect x="{ex}" y="{ey}" width="{ew}" height="{eh}" rx="10" class="esp"/>')
a(f'<text x="{ex+ew/2}" y="{ey+26}" text-anchor="middle" class="h">ESP32 DevKit</text>')
L = {"GPIO 4": 140, "GPIO 16": 210, "GPIO 17": 280, "GPIO 25": 350, "3V3": 430, "GND": 740}
R = {"GPIO 21 (SDA)": 120, "GPIO 22 (SCL)": 145, "GPIO 18 (SCK)": 220, "GPIO 23 (MOSI)": 245, "GPIO 19 (MISO)": 270,
     "GPIO 5 (CS carte SD)": 295, "GPIO 33 (CS écran)": 360, "GPIO 14 (DC écran)": 385, "GPIO 13 (RST écran)": 410,
     "GPIO 32": 480, "GPIO 34": 520, "GPIO 35": 545, "GPIO 39": 570, "GPIO 27": 640, "VIN (5 V)": 700}
for n, y in L.items():
    a(f'<circle cx="{ex}" cy="{y}" r="4" fill="#1a4fa0"/><text x="{ex+10}" y="{y+4}" class="t">{n}</text>')
for n, y in R.items():
    a(f'<circle cx="{ex+ew}" cy="{y}" r="4" fill="#1a4fa0"/><text x="{ex+ew-10}" y="{y+4}" text-anchor="end" class="t">{n}</text>')

# Sondes
a('<text x="30" y="96" class="t" font-weight="bold">Sondes DS18B20</text>')
a('<text x="30" y="112" class="s">témoin sur prise M12 · bain par presse-étoupes</text>')
a('<line x1="300" y1="118" x2="300" y2="430" class="r"/><line x1="300" y1="430" x2="430" y2="430" class="r"/>')
a('<text x="306" y="114" class="s">3,3 V</text>')
a('<line x1="250" y1="118" x2="250" y2="740" class="k"/><line x1="40" y1="740" x2="430" y2="740" class="k"/>')
names = ["Témoin (bouteille)", "Bain place 1 (entrée)", "Bain place 2", "Bain place 3 (sortie)"]
for nm, y in zip(names, [140, 210, 280, 350]):
    a(f'<rect x="30" y="{y-26}" width="190" height="52" rx="6" class="box"/>')
    a(f'<text x="40" y="{y-6}" class="t" font-weight="bold">{nm}</text>')
    a(f'<text x="40" y="{y+12}" class="s">VDD rouge · DATA jaune · GND noir</text>')
    a(f'<line x1="220" y1="{y-16}" x2="300" y2="{y-16}" class="r"/><circle cx="300" cy="{y-16}" r="3" fill="#d32f2f"/>')
    a(f'<line x1="220" y1="{y}" x2="430" y2="{y}" class="b"/>')
    a(f'<line x1="220" y1="{y+16}" x2="250" y2="{y+16}" class="k"/><circle cx="250" cy="{y+16}" r="3" fill="#222"/>')
    a(f'<line x1="345" y1="{y}" x2="345" y2="{y-6}" class="b"/><circle cx="345" cy="{y}" r="3" fill="#1565c0"/>')
    a(f'<rect x="340" y="{y-26}" width="10" height="20" class="res"/>')
    a(f'<polyline points="345,{y-26} 345,{y-30} 300,{y-30}" class="r"/><circle cx="300" cy="{y-30}" r="3" fill="#d32f2f"/>')
    a(f'<text x="355" y="{y-12}" class="s">4,7 kΩ</text>')

bx = 820
def box(y, h, title, lines):
    a(f'<rect x="{bx}" y="{y}" width="340" height="{h}" rx="6" class="box"/><text x="{bx+10}" y="{y+19}" class="t" font-weight="bold">{title}</text>')
    for i, l in enumerate(lines):
        a(f'<text x="{bx+10}" y="{y+37+16*i}" class="s">{l}</text>')

# Horloge I2C
box(95, 62, "Horloge DS3231 (I2C)", ["SDA → 21 · SCL → 22 · VCC → 3V3 · GND"])
a(f'<line x1="{ex+ew}" y1="120" x2="{bx}" y2="120" class="p"/><line x1="{ex+ew}" y1="145" x2="{bx}" y2="145" class="p"/>')
# Bus SPI partagé
box(195, 120, "Carte microSD (SPI)", ["CS → 5 · SCK → 18 · MOSI → 23 · MISO → 19", "VCC → VIN (module avec régulateur) · GND", "(sans régulateur : VCC → 3V3)"])
box(335, 105, "Écran OLED 2,42\" SSD1309 (SPI)", ["SCK (D0) → 18 · MOSI (D1) → 23 · CS → 33", "DC → 14 · RES → 13 · VCC → 3V3 · GND", "SCK et MOSI partagés avec la carte SD"])
xs = 760
for y, ty in [(220, 220), (245, 245)]:
    a(f'<line x1="{ex+ew}" y1="{y}" x2="{bx}" y2="{y}" class="o"/>')
a(f'<line x1="{ex+ew}" y1="270" x2="{bx}" y2="270" class="o"/><line x1="{ex+ew}" y1="295" x2="{bx}" y2="295" class="o"/>')
# branches SCK / MOSI vers l'écran
a(f'<polyline points="{xs},220 {xs},350 {bx},350" class="o"/><circle cx="{xs}" cy="220" r="3.5" fill="#e08600"/>')
a(f'<polyline points="{xs+20},245 {xs+20},368 {bx},368" class="o"/><circle cx="{xs+20}" cy="245" r="3.5" fill="#e08600"/>')
a(f'<line x1="{ex+ew}" y1="360" x2="{xs-10}" y2="360" class="o"/><polyline points="{xs-10},360 {xs-10},386 {bx},386" class="o"/>')
a(f'<line x1="{ex+ew}" y1="385" x2="{xs-25}" y2="385" class="o"/><polyline points="{xs-25},385 {xs-25},404 {bx},404" class="o"/>')
a(f'<line x1="{ex+ew}" y1="410" x2="{xs-40}" y2="410" class="o"/><polyline points="{xs-40},410 {xs-40},422 {bx},422" class="o"/>')
# Boutons
box(462, 135, "Boutons (contact NO vers GND)", ["DÉCALAGE (Ø 22 mm) → 32 · rappel interne",
    "▲ → 34 · ▼ → 35 · OK → 39", "+ une 10 kΩ entre chaque broche et 3V3", "(34, 35, 39 n'ont pas de rappel interne)",
    "OK long = retour · DÉCALAGE long = silence"])
for y in [480, 520, 545, 570]:
    a(f'<line x1="{ex+ew}" y1="{y}" x2="{bx}" y2="{y}" class="b"/>')
for i, y in enumerate([520, 545, 570]):
    x = 700 + 22 * i
    a(f'<circle cx="{x}" cy="{y}" r="3" fill="#1565c0"/><line x1="{x}" y1="{y}" x2="{x}" y2="{y-10}" class="b"/>'
      f'<rect x="{x-5}" y="{y-28}" width="10" height="18" class="res"/><line x1="{x}" y1="{y-28}" x2="{x}" y2="500" class="r"/>')
a('<polyline points="700,500 744,500" class="r"/><text x="748" y="504" class="s">3V3</text>')
a('<text x="684" y="598" class="s">3 × 10 kΩ</text>')
# Buzzer + alim
box(615, 72, "Buzzer actif (module KY-012)", ["S → 27 · + → 3V3 ou 5 V selon le module · − → GND", "(buzzer nu : via NPN 2N2222 + 1 kΩ)"])
a(f'<line x1="{ex+ew}" y1="640" x2="{bx}" y2="640" class="b"/>')
box(705, 72, "Alimentation 5 V", ["→ VIN et GND (ou prise USB de la carte)", "prise sous différentiel 30 mA"])
a(f'<line x1="{ex+ew}" y1="700" x2="{bx}" y2="728" class="g"/>')
a(f'<polyline points="{ex+ew},700 790,700 790,262 {bx},262" class="g"/><circle cx="790" cy="700" r="3" fill="#2e7d32"/>')
a('<text x="20" y="840" class="s">Brochage défini dans firmware/compteur_up/config.h. Variante écran 1,3" SH1106 I2C : SDA 21 / SCL 22, et commenter DISPLAY_SSD1309_SPI.</text>')
a('</svg>')
svg = "\n".join(o)
open("docs/cablage.svg", "w").write(svg)
cairosvg.svg2png(bytestring=svg.encode(), write_to="docs/cablage.png", output_width=1400)
print("ok")
