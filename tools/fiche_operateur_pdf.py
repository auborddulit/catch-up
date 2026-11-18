# Génère docs/fiche-operateur.pdf (A4, à plastifier) :
#   python3 tools/fiche_operateur_pdf.py docs/fiche-operateur.pdf
# Nécessite WeasyPrint et les polices Inter. Les écrans viennent de docs/ecrans/
# (captures du vrai firmware : make -C tests ecrans).
import os, sys, weasyprint

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def img(n):
    return f'<img class="oimg" src="file://{ROOT}/docs/ecrans/{n}.png">'

S = [(img("06-nouveau-lot-verifier"), "<b>Démarrer</b> : n° de lot, T°, cible, puis OK."),
     (img("09-en-cours"), "<b>En cours</b> : UP de la caisse en sortie / cible."),
     (img("10-decaler"), "<b>DÉCALER</b> : décaler, puis appuyer."),
     (img("11-temoin-debranchable"), "<b>Témoin débranchable</b> : capuchon sur la prise.")]

steps = [("OK", "Démarrer le lot", "<b>OK</b> › Démarrer un lot › <b>OK</b> à chaque écran (n° de lot, T° des bouteilles, cible) › <b>OK</b> démarrer."),
         ("DÉCALAGE", "Caisse 1 avec le témoin", "Témoin dans la caisse 1, entrer la caisse, <b>appuyer</b>."),
         ("DÉCALAGE", "À chaque DÉCALER", "Sortir à gauche, avancer, entrer à droite, <b>appuyer</b>."),
         ("Prise", "« Témoin débranchable »", "Débrancher le témoin, <b>capuchon sur la prise</b>, témoin en chambre froide."),
         ("OK", "Dernière caisse entrée", "<b>OK</b> › Fin des entrées › <b>OK</b>. Continuer au bip."),
         ("OK", "« Lot terminé »", "<b>OK</b> › Terminer le lot › <b>OK</b>. Relevé à imprimer et signer (page web).")]

probs = [("Appui DÉCALAGE en trop", "<b>OK</b> › Annuler décalage › <b>OK</b> (dans la minute)."),
         ("Une caisse de trop affichée", "<b>OK</b> › Retirer dern. caisse › <b>OK</b>."),
         ("« Sortie avant la cible »", "<b>Isoler la caisse</b> et prévenir le responsable."),
         ("« Bain trop froid »", "Vérifier cuve, niveau, pompe. Le décalage s'adapte seul."),
         ("Coupure de courant", "Rien à faire : reprise automatique. Le signaler."),
         ("Le témoin ne marche plus", "Finir le lot, prévenir le responsable (lot à vérifier)."),
         ("Un menu ouvert par erreur", "<b>OK long</b> (1 s) = retour.")]

TAG = {"OK": "tel", "DÉCALAGE": "btn", "Prise": "pri"}
st = "".join(f'''<tr><td class="num"><div class="dot">{i+1}</div></td><td class="stp"><span class="st">{t}</span> <span class="tag {TAG[w]}">{w}</span><br>{x}</td></tr>''' for i, (w, t, x) in enumerate(steps))
sc = f'''<table class="scr"><tr><td>{S[0][0]}<div class="cap">{S[0][1]}</div></td><td>{S[1][0]}<div class="cap">{S[1][1]}</div></td></tr>
<tr><td>{S[2][0]}<div class="cap">{S[2][1]}</div></td><td>{S[3][0]}<div class="cap">{S[3][1]}</div></td></tr></table>'''
pr = "".join(f'<tr><td class="k">{a}</td><td>{b}</td></tr>' for a, b in probs)

html = f'''<!doctype html><html lang="fr"><head><meta charset="utf-8"><style>
@page{{size:A4;margin:0}}
body{{margin:0;font-family:Inter,sans-serif;color:#141413}}
.page{{width:794px;height:1123px;box-sizing:border-box;padding:40px 44px 36px;position:relative}}
table{{border-collapse:collapse;width:100%}}
.hd td{{vertical-align:bottom;padding:0 0 10px}} .hd{{border-bottom:3px solid #141413}}
.kick{{font-size:13px;font-weight:600;letter-spacing:1px;color:#5f5e5a}}
h1{{margin:2px 0 0;font-size:34px;line-height:40px}}
h2{{margin:0 0 8px;font-size:19px}}
.addr{{font-size:13px;line-height:18px;text-align:right;color:#3d3d3a}}
.sig{{border-collapse:separate;border-spacing:12px 0;margin:16px -12px 0;width:calc(100% + 24px)}}
.sig td{{width:33.3%;border:2px solid;border-radius:10px;padding:10px 12px;vertical-align:top;font-size:14px;line-height:19px}}
.sig .l{{font-size:11.5px;font-weight:700}} .sig .b{{font-size:22px;font-weight:700;line-height:28px}}
.g{{background:#e3f1e7;border-color:#0a6b2d}} .g .l{{color:#0a6b2d}}
.r{{background:#fbe9e7;border-color:#b3261e}} .r .l{{color:#b3261e}}
.n{{background:#f1efe8;border-color:#5f5e5a}} .n .l{{color:#444441}}
.mid{{margin-top:20px}} .mid>tbody>tr>td{{vertical-align:top}}
.num{{width:36px;vertical-align:top;padding:0 0 10px}}
.dot{{width:28px;height:28px;border-radius:14px;background:#141413;color:#fff;font-weight:700;font-size:15px;text-align:center;line-height:28px}}
.stp{{font-size:14px;line-height:19px;padding:2px 0 10px;color:#2b2b29}} .st{{font-size:16px;font-weight:700;color:#141413}}
.tag{{font-size:11.5px;font-weight:600;padding:1px 7px;border-radius:9px}}
.tel{{color:#1b4f8a;background:#e6eef7}} .btn{{color:#0a6b2d;background:#e3f1e7}} .pri{{color:#7a4a00;background:#fbefd9}}
.note{{font-size:12.5px;line-height:16px;color:#3d3d3a;margin-bottom:10px}}
.scr td{{width:50%;vertical-align:top;padding:0 5px 12px}}
.oimg{{width:100%;display:block;border:5px solid #3a3a38;border-radius:5px;box-sizing:border-box}}
.cap{{font-size:12.5px;line-height:16px;color:#3d3d3a;margin-top:4px}}
.pb{{margin-top:6px;font-size:14px;line-height:19px;border-top:1px solid #c9c7bf}} .pb td{{padding:5px 10px 5px 0;border-bottom:1px solid #c9c7bf;vertical-align:top}} .pb .k{{font-weight:600;width:36%}}
.ft{{position:absolute;left:44px;width:706px;bottom:36px;border-top:1px solid #c9c7bf;padding-top:8px;font-size:13px;line-height:18px;color:#3d3d3a}}
</style></head><body><div class="page">
<table class="hd"><tr><td><div class="kick">PASTEURISATION · FICHE OPÉRATEUR</div><h1>Le compteur UP en 1 minute</h1></td>
<td class="addr">Boutons : <b style="color:#141413">▲ ▼ OK</b> + <b style="color:#141413">DÉCALAGE</b><br>Relevés : <b style="color:#141413">http://compteur-up.local</b></td></tr></table>
<table class="sig"><tr>
<td class="g"><div class="l">1 BIP / SECONDE · ÉCRAN CLIGNOTANT</div><div class="b">DÉCALER</div>Décaler, puis <b>appuyer sur DÉCALAGE</b>.</td>
<td class="r"><div class="l">3 BIPS RAPIDES · ÉCRAN CLIGNOTANT</div><div class="b">ALARME</div>Lire l'écran. <b>OK</b> = acquitter.</td>
<td class="n"><div class="l">DÉCALAGE MAINTENU 2 S</div><div class="b">SILENCE</div>Coupe le buzzer 5 minutes.</td></tr></table>
<table class="mid"><tr><td style="width:53%;padding-right:24px"><h2>Un lot, étape par étape</h2><table>{st}</table></td>
<td><h2>Ce que montre l'écran</h2><div class="note"><b>OK</b> ouvre le menu · <b>▲ ▼</b> choisir ou modifier · <b>OK long</b> = retour.</div>{sc}</td></tr></table>
<h2 style="margin-top:12px">En cas de problème</h2><table class="pb">{pr}</table>
<table class="ft"><tr><td>Commentaires : page web, après le lot · Ne pas débrancher l'appareil pendant un lot</td>
<td style="text-align:right">Responsable : <b style="color:#141413">[NOM] · [TÉLÉPHONE]</b></td></tr></table>
</div></body></html>'''
open('/tmp/fiche_print.html', 'w').write(html)
weasyprint.HTML('/tmp/fiche_print.html').write_pdf(sys.argv[1])
