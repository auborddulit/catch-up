# Matériel

Prix indicatifs (fin 2026, revendeurs en ligne courants), TTC. Total **≈ 140 à 180 €** hors thermomètre de référence.

## Liste des composants

| # | Composant | Qté | Prix ≈ | Remarques |
|---|---|---|---|---|
| 1 | **ESP32 DevKit V1** (ESP32-WROOM-32, 30 ou 38 broches) | 1 | 8 € | Toute carte ESP32 « classique » convient. Évitez les ESP32-S2/C3 (brochage différent). |
| 2 | **Sonde DS18B20 étanche, câble silicone, 3 m**, gaine inox Ø 6 × 50 mm | 1 | 6 € | La sonde du **témoin**. Prenez la version à **câble silicone** (tient > 100 °C), pas PVC. |
| 3 | **Sonde DS18B20 étanche, câble silicone, 2 m**, gaine inox Ø 6 × 50 mm | 3 | 3 × 5 € | Les 3 sondes de bain. |
| 4 | **Écran OLED 2,42" SSD1309 128×64, SPI** (ex. Joy-IT COM-OLED2.42, réglé en SPI d'usine) | 1 | 20 € | Lisible à environ 1 m. Barrette à souder (~7 broches). Variante : OLED 1,3" SH1106 en I2C (une ligne à changer dans `config.h`). |
| 5 | **Module horloge DS3231** (avec pile) | 1 | 4 € | Garde l'heure si le Wi-Fi tombe. Voir la note sur la pile ci-dessous. |
| 6 | **Module lecteur microSD (SPI)** avec régulateur 3,3 V | 1 | 2 € | Modèle courant « Micro SD Card Adapter » à 6 broches, alimenté en 5 V. Partage le bus SPI avec l'écran. |
| 7 | **Carte microSD 8 à 32 Go**, formatée en FAT32 | 1 | 6 € | Une carte de marque (relevés = documents de traçabilité). |
| 8 | **Gros bouton DÉCALAGE** : poussoir Ø 22 mm IP65/IP67 à impulsion, contact NO (ex. Schneider XB5AA31 vert) | 1 | 15 € | Monté sur le boîtier, côté sortie des caisses. |
| 9 | **3 petits boutons ▲ ▼ OK** : poussoirs à impulsion, contact NO, bornes à vis, IP65 (ex. Delock 22041, Ø 19 mm inox) | 3 | 3 × 4 € | Menus sur l'appareil (démarrer / terminer un lot, corrections). Marquer ▲ ▼ OK sur le couvercle. Limite souvent +55 °C : boîtier hors de la vapeur du bain. |
| 10 | **Résistances 10 kΩ** | 3 | 0,3 € | Rappel à 3,3 V des boutons ▲ ▼ OK (broches 34, 35, 39 sans rappel interne). **Indispensables.** |
| 11 | **Buzzer actif** : module KY-012 en direct sur la broche, ou buzzer 5 V + transistor NPN (2N2222, 1 kΩ) | 1 | 2 € | Ou une sirène / un gyrophare 12 V via un module MOSFET (voir câblage). |
| 12 | Résistances **4,7 kΩ** | 4 | 0,5 € | Une par sonde (tirage du bus 1-Wire). |
| 13 | **Prise M12 IP67 pour le témoin** : embase femelle 4 broches codée A (ex. Lutronic 1220 04 T16CW) + fiche mâle à vis (ex. Delock 60533) + **capuchon de protection M12 avec chaînette** | 1 jeu | 20 € | On débranche le témoin pour le ranger en chambre froide. La prise n'est étanche (IP67) que fiche vissée ou capuchon vissé. Les 3 sondes de bain, fixes, passent par des presse-étoupes, sans prise. |
| 14 | **Boîtier IP65 à couvercle transparent**, assez grand pour l'écran et les 4 boutons (ex. BOPLA M 238 G) | 1 | 25 € | L'écran se lit à travers le couvercle ; les boutons traversent le couvercle ou la face avant. |
| 15 | Presse-étoupes PG7 / PG9 | 4 | 4 € | Alimentation, 3 sondes de bain. |
| 16 | **Alimentation 5 V** : bloc USB 5 V 2 A, ou alimentation secteur intégrée au boîtier (ex. Mean Well APV-8-5) | 1 | 8–15 € | Prise sous différentiel 30 mA, hors des projections. Une alimentation secteur dans le boîtier doit être raccordée par une personne compétente. |
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
