# Génère docs/fiche-operateur.pdf : python3 tools/fiche_operateur_pdf.py docs/fiche-operateur.pdf (WeasyPrint + polices Inter et JetBrains Mono)
# Version imprimable de la fiche opérateur (mise en page en tableaux, fiable pour WeasyPrint)
import weasyprint, sys
def screen(lines, band=None, foot=None):
    body="<br>".join(l.replace(" ","&nbsp;") for l in lines)
    extra=""
    if band: extra=f'<div class="band">{band}</div>'
    if foot: extra=f'<div class="foot">{foot.replace(" ","&nbsp;")}</div>'
    return f'<div class="oled">{body}{extra}</div>'
S=[(screen(["L2611-A      09:02","Eau 68.0 67.1 66.2","Témoin -&gt; caisse 1","puis appuyer"]),"<b>Lot démarré</b> : témoin en caisse 1, puis appuyer."),
   (screen(["L2611-A      09:41","Eau 67.6 67.2 66.0","Sortie C5 31.4/33"],foot="Décaler ~0:48 C7"),"<b>Attendre</b> : temps avant le décalage."),
   (screen(["L2611-A      09:42","Eau 67.6 67.2 66.0","Sortie C5 33.1/33"],band="DÉCALER"),"<b>Décaler</b>, puis appuyer."),
   (screen(["L2611-A      10:15","Eau 63.1 66.8 65.9","Sortie C9 18.2/33"],band="ALARME"),"<b>Alarme</b> : voir le téléphone.")]
steps=[("Téléphone","Démarrer le lot","<b>Session</b> : n° de lot, T° chambre froide, cible, <b>Démarrer</b>."),
("Bouton","Caisse 1 avec le témoin","Témoin dans la caisse 1, entrer la caisse, <b>appuyer</b>."),
("Bouton","À chaque DÉCALER","Sortir à gauche, avancer, entrer à droite, <b>appuyer</b>."),
("Bouton","Caisse 1 sortie","Quand l'écran affiche « Témoin/modèle » : débrancher le témoin, <b>capuchon sur la prise</b>, témoin en chambre froide."),
("Téléphone","Dernière caisse entrée","<b>Fin des entrées</b>, puis continuer au bip."),
("Téléphone","« Terminé : clôturer »","<b>Session</b> : <b>Clôturer</b>, imprimer et signer le relevé.")]
probs=[("Appui en trop","Téléphone : <b>Annuler</b> (dans la minute)."),
("Une caisse de trop affichée","Téléphone : <b>Retirer la dernière caisse</b>."),
("« Sortie avant la cible »","<b>Isoler la caisse</b> et prévenir le responsable."),
("« Bain trop froid »","Vérifier cuve, niveau, pompe. Le décalage s'adapte seul."),
("Coupure de courant","Rien à faire : reprise automatique. Le signaler."),
("Le témoin ne marche plus","Finir le lot, prévenir le responsable (lot à vérifier).")]
st="".join(f'''<tr><td class="num"><div class="dot">{i+1}</div></td><td class="stp"><span class="st">{t}</span> <span class="tag {'tel' if w=='Téléphone' else 'btn'}">{w}</span><br>{x}</td></tr>''' for i,(w,t,x) in enumerate(steps))
sc=f'''<table class="scr"><tr><td>{S[0][0]}<div class="cap">{S[0][1]}</div></td><td>{S[1][0]}<div class="cap">{S[1][1]}</div></td></tr>
<tr><td>{S[2][0]}<div class="cap">{S[2][1]}</div></td><td>{S[3][0]}<div class="cap">{S[3][1]}</div></td></tr></table>'''
pr="".join(f'<tr><td class="k">{a}</td><td>{b}</td></tr>' for a,b in probs)
html=f'''<!doctype html><html lang="fr"><head><meta charset="utf-8"><style>
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
.sig .l{{font-size:13px;font-weight:700}} .sig .b{{font-size:22px;font-weight:700;line-height:28px}}
.g{{background:#e3f1e7;border-color:#0a6b2d}} .g .l{{color:#0a6b2d}}
.r{{background:#fbe9e7;border-color:#b3261e}} .r .l{{color:#b3261e}}
.n{{background:#f1efe8;border-color:#5f5e5a}} .n .l{{color:#444441}}
.mid{{margin-top:20px}} .mid>tbody>tr>td{{vertical-align:top}}
.num{{width:36px;vertical-align:top;padding:0 0 11px}}
.dot{{width:28px;height:28px;border-radius:14px;background:#141413;color:#fff;font-weight:700;font-size:15px;text-align:center;line-height:28px}}
.stp{{font-size:14px;line-height:19px;padding:2px 0 11px;color:#2b2b29}} .st{{font-size:16px;font-weight:700;color:#141413}}
.tag{{font-size:11.5px;font-weight:600;padding:1px 7px;border-radius:9px}} .tel{{color:#1b4f8a;background:#e6eef7}} .btn{{color:#0a6b2d;background:#e3f1e7}}
.note{{font-size:12.5px;line-height:16px;color:#3d3d3a;margin-bottom:10px}}
.scr td{{width:50%;vertical-align:top;padding:0 5px 12px}}
.oled{{background:#0b0b0b;border:5px solid #3a3a38;border-radius:5px;height:74px;padding:5px 7px;font-family:'JetBrains Mono',monospace;font-size:11px;line-height:14.5px;color:#B5D4F4;position:relative;overflow:hidden}}
.band{{position:absolute;left:0;right:0;bottom:0;height:22px;line-height:22px;background:#B5D4F4;color:#0b0b0b;font-family:Inter;font-weight:700;font-size:14px;text-align:center}}
.foot{{position:absolute;left:7px;bottom:3px}}
.cap{{font-size:12.5px;line-height:16px;color:#3d3d3a;margin-top:4px}}
.pb{{margin-top:6px;font-size:14px;line-height:19px;border-top:1px solid #c9c7bf}} .pb td{{padding:5px 10px 5px 0;border-bottom:1px solid #c9c7bf;vertical-align:top}} .pb .k{{font-weight:600;width:36%}}
.ft{{position:absolute;left:44px;width:706px;bottom:36px;border-top:1px solid #c9c7bf;padding-top:8px;font-size:13px;line-height:18px;color:#3d3d3a}}
</style></head><body><div class="page">
<table class="hd"><tr><td><div class="kick">PASTEURISATION · FICHE OPÉRATEUR</div><h1>Le compteur UP en 1 minute</h1></td>
<td class="addr">Page de l'appareil :<br><b style="color:#141413">http://compteur-up.local</b></td></tr></table>
<table class="sig"><tr>
<td class="g"><div class="l">1 BIP PAR SECONDE</div><div class="b">DÉCALER</div>Décaler, puis <b>appuyer sur le bouton</b>.</td>
<td class="r"><div class="l">3 BIPS RAPIDES</div><div class="b">ALARME</div>Lire le message sur le <b>téléphone</b>.</td>
<td class="n"><div class="l">APPUI LONG 2 S</div><div class="b">SILENCE</div>Coupe le buzzer 5 minutes.</td></tr></table>
<table class="mid"><tr><td style="width:53%;padding-right:24px"><h2>Un lot, étape par étape</h2><table>{st}</table></td>
<td><h2>Ce que montre l'écran</h2><div class="note">« Eau » : le bain, dans l'ordre du bain (sortie à gauche).</div>{sc}</td></tr></table>
<h2 style="margin-top:14px">En cas de problème</h2><table class="pb">{pr}</table>
<table class="ft"><tr><td>Ne pas toucher aux <b>Réglages</b> ni à l'<b>Étalonnage</b> · Ne pas débrancher pendant un lot</td>
<td style="text-align:right">Responsable : <b style="color:#141413">[NOM] · [TÉLÉPHONE]</b></td></tr></table>
</div></body></html>'''
open('/tmp/fiche_print.html','w').write(html)
weasyprint.HTML('/tmp/fiche_print.html').write_pdf(sys.argv[1])
