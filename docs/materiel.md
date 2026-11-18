# Matériel

Prix indicatifs (fin 2026, revendeurs en ligne courants), TTC. Total **≈ 100 à 130 €** hors thermomètre de référence.

## Liste des composants

| # | Composant | Qté | Prix ≈ | Remarques |
|---|---|---|---|---|
| 1 | **ESP32 DevKit V1** (ESP32-WROOM-32, 30 ou 38 broches) | 1 | 8 € | Toute carte ESP32 « classique » convient. Évitez les ESP32-S2/C3 (brochage différent). |
| 2 | **Sonde DS18B20 étanche, câble silicone, 3 m**, gaine inox Ø 6 × 50 mm | 1 | 6 € | La sonde du **témoin**. Prenez la version à **câble silicone** (tient > 100 °C), pas PVC. |
| 3 | **Sonde DS18B20 étanche, câble silicone, 2 m**, gaine inox Ø 6 × 50 mm | 3 | 3 × 5 € | Les 3 sondes de bain. |
| 4 | **Écran OLED 1,3" SH1106, I2C** (4 broches) | 1 | 7 € | Un 0,96" SSD1306 marche aussi (une ligne à changer dans `display.cpp`), mais il est petit à lire. |
| 5 | **Module horloge DS3231** (avec pile) | 1 | 4 € | Garde l'heure si le Wi-Fi tombe. Voir la note sur la pile ci-dessous. |
| 6 | **Module lecteur microSD (SPI)** avec régulateur 3,3 V | 1 | 2 € | Modèle courant « Micro SD Card Adapter » à 6 broches, alimenté en 5 V. |
| 7 | **Carte microSD 8 à 32 Go**, formatée en FAT32 | 1 | 6 € | Une carte de marque (relevés = documents de traçabilité). |
| 8 | **Bouton poussoir métal 22 mm, IP67, à impulsion** (contact NO) | 1 | 6 € | Le « gros bouton DÉCALAGE ». Version à LED possible (non utilisée). Ou un bouton « coup de poing » 40 mm étanche. |
| 9 | **Buzzer actif 5 V** (≥ 85 dB) | 1 | 1 € | Ou une sirène / un gyrophare 12 V via un module MOSFET (voir câblage). |
| 10 | Transistor NPN **2N2222** ou **S8050** + résistance **1 kΩ** | 1 | 0,5 € | Commande du buzzer. |
| 11 | Résistances **4,7 kΩ** | 4 | 0,5 € | Une par sonde (tirage du bus 1-Wire). |
| 12 | Connecteurs aviation **GX12 3 broches** (embase + fiche) | 4 | 7 € | Une prise par sonde : on débranche le témoin pour le ranger. |
| 13 | Connecteur **GX12 2 broches** | 1 | 2 € | Pour le bouton déporté. |
| 14 | **Boîtier ABS IP65** ≈ 160 × 120 × 70 mm, couvercle transparent si possible | 1 | 12 € | L'écran se lit à travers le couvercle. |
| 15 | Presse-étoupes PG7 / PG9 | 3 | 3 € | Alimentation, câble du bouton. |
| 16 | **Alimentation USB 5 V 2 A** + câble micro-USB / USB-C selon la carte | 1 | 8 € | À brancher hors des zones de projection. |
| 17 | Plaque à pastilles, borniers à vis, fils, gaine thermo | — | 6 € | |
| 18 | **Bouteille 33 cl identique à la production** + **bouchon caoutchouc conique** à percer | 1 | 2 € | Pour le témoin, voir plus bas. |
| 19 | Colliers / clips inox ou PVC | — | 3 € | Fixation des sondes de bain sur la paroi. |

### Fortement recommandé : thermomètre de référence

La précision des UP dépend directement de celle des sondes. **0,5 °C d'erreur = environ 18 % d'erreur sur les UP**, et 0,1 °C ≈ 3,4 %. Le DS18B20 est donné à ±0,5 °C (et les copies sont nombreuses). Il faut donc **corriger chaque sonde** par comparaison avec une référence :

- un **thermomètre de référence avec certificat d'étalonnage** (sonde Pt100 ou thermistance, résolution 0,01 °C, incertitude ≤ 0,1 à 0,2 °C entre 60 et 70 °C) ;
- compter 80 à 250 € selon le certificat. Il servira aussi aux vérifications périodiques du plan HACCP.

## La bouteille témoin

C'est la pièce la plus importante. Elle doit chauffer **comme vos bouteilles**.

1. Prendre une **bouteille de production** (même verre, même modèle).
2. La remplir d'**eau** au **même niveau** que le produit, pour garder le même espace de tête. L'eau chauffe un peu moins vite qu'une boisson sucrée à 40 g/L, ce qui va dans le sens de la prudence.
3. Percer un **bouchon caoutchouc conique** au diamètre de la gaine (6 mm), adapté au goulot : mesurer le diamètre intérieur, en général 16 à 18 mm pour une capsule couronne de 26 mm.
4. Glisser la sonde : **pointe dans l'axe de la bouteille, à environ 2 cm du fond**. C'est la zone qui chauffe le plus lentement dans un liquide chauffé par l'extérieur. Bloquer la hauteur avec un repère sur le câble, ou un petit tube silicone serré autour du câble au-dessus du bouchon.
5. Étanchéifier le passage du câble avec un **silicone alimentaire**.
6. Stocker le témoin **avec les bouteilles pleines** (chambre froide, 4 à 8 °C) pour qu'il parte de la même température.

> La position du témoin dans la caisse (la bouteille qui chauffe le moins vite) se cherche en mode Mesure, par passages successifs : voir [utilisation.md](utilisation.md), section 3.

## Notes

- **Pile du DS3231** : beaucoup de modules ont un circuit de charge prévu pour une pile rechargeable LIR2032. Avec une pile CR2032 classique, retirez la résistance de charge (souvent 200 Ω près de la diode) ou utilisez une LIR2032.
- **Sondes de bain** : à fixer sur la paroi du fond, **une face au centre de chaque place**, à mi-hauteur des bouteilles. Écartez-les d'environ 2 cm de la paroi, sinon elles mesurent l'isolant et pas l'eau.
- **Câble du témoin** : 3 m suffisent si le boîtier est près du milieu du bain. Le témoin parcourt les 3 places, puis on le retire à la sortie du bain.
