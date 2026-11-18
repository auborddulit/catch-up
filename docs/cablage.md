# Câblage

![Plan de câblage](cablage.png)

(Version vectorielle : [cablage.svg](cablage.svg).)

## Tableau des connexions

| Élément | Broche élément | Broche ESP32 | Remarque |
|---|---|---|---|
| Sonde **témoin** | DATA (jaune) | **GPIO 4** | 4,7 kΩ entre DATA et 3,3 V |
| Sonde **bain place 1** (entrée, droite) | DATA | **GPIO 16** | 4,7 kΩ |
| Sonde **bain place 2** | DATA | **GPIO 17** | 4,7 kΩ |
| Sonde **bain place 3** (sortie, gauche) | DATA | **GPIO 25** | 4,7 kΩ |
| Toutes les sondes | VDD (rouge) / GND (noir) | 3V3 / GND | **Alimentation 3 fils** (pas de mode parasite) |
| DS3231 | SDA / SCL / VCC / GND | 21 / 22 / 3V3 / GND | Adresse I2C 0x68 (module : EEPROM en 0x57, sans conflit) |
| Lecteur microSD | CS / SCK / MISO / MOSI | 5 / 18 / 19 / 23 | VCC sur VIN (5 V) si le module a un régulateur |
| Écran OLED 2,42" SSD1309 (SPI) | SCK (D0) / MOSI (D1) / CS / DC / RES / VCC / GND | 18 / 23 / **33** / **14** / **13** / 3V3 / GND | **SCK et MOSI partagés avec la carte SD.** Écran laissé en SPI (réglage d'usine Joy-IT) |
| Bouton DÉCALAGE | contact NO | **GPIO 32** et GND | Rappel interne, anti-rebond logiciel |
| Bouton ▲ | contact NO | **GPIO 34** et GND | **10 kΩ entre GPIO 34 et 3V3** (pas de rappel interne) |
| Bouton ▼ | contact NO | **GPIO 35** et GND | **10 kΩ entre GPIO 35 et 3V3** |
| Bouton OK | contact NO | **GPIO 39** et GND | **10 kΩ entre GPIO 39 et 3V3** |
| Buzzer | module KY-012 : S / + / − | **GPIO 27** / 3V3 ou 5 V / GND | Buzzer nu : via un NPN (2N2222, 1 kΩ dans la base), buzzer entre 5 V et le collecteur |

**Variante écran 1,3" SH1106 en I2C** : SDA 21, SCL 22, VCC 3V3, GND, et mettre en commentaire la ligne `#define DISPLAY_SSD1309_SPI` dans `config.h`. Si l'image du 2,42" apparaît décalée de 2 colonnes, remplacer `NONAME0` par `NONAME2` dans `display.cpp`.

Les couleurs des fils des sondes varient selon les fabricants. Vérifiez avec la fiche du vendeur, ou au multimètre (le fil relié à la gaine inox est en général le GND).

## Pourquoi une broche par sonde ?

Chaque sonde a son propre bus 1-Wire. L'appareil sait donc **quelle sonde est où d'après sa prise**, sans adresse à configurer. Vous pouvez échanger une sonde défectueuse sans rien régler (pensez juste à refaire son étalonnage). Une sonde en panne ne perturbe pas les autres.

## Conseils de montage

- **Témoin sur prise M12** (IP67, 4 broches codée A, 3 utilisées : 1 = VDD, 3 = GND, 4 = DATA) : on le débranche pour le ranger en chambre froide. **Capuchon vissé dès que la prise est libre** : elle n'est étanche que fermée. Les **3 sondes de bain** sont fixes et passent par des presse-étoupes, avec une étiquette (B1, B2, B3) sur chaque câble.
- Soudez les **résistances de 4,7 kΩ côté boîtier**, sur la plaque à pastilles.
- **Câbles longs** : jusqu'à environ 5 m par sonde, aucun souci avec 4,7 kΩ. En cas de lectures « absente » intermittentes, passez à 2,2 kΩ.
- Mettez le **boîtier à l'abri des projections**, au-dessus du niveau du bain, et branchez l'alimentation sur une prise protégée par un différentiel 30 mA.
- **Bouton DÉCALAGE** : à fixer à portée de main, côté sortie des caisses. Il fonctionne mains mouillées.
- **Boutons ▲ ▼ OK** : à côté de l'écran, marqués sur le couvercle. Sans leurs **résistances de 10 kΩ**, les broches 34, 35 et 39 « flottent » et l'appareil croit recevoir des appuis au hasard.
- **Boîtier hors de la vapeur du bain** : les petits boutons courants sont donnés pour +55 °C au plus.
- **Sondes de bain** : une face au centre de chaque place, à mi-hauteur des bouteilles, à environ 2 cm de la paroi, sans toucher les caisses. Elles ne doivent pas se trouver dans le jet direct d'arrivée d'eau chaude.

## Variante : sirène ou gyrophare 12 V

Si le buzzer n'est pas assez fort dans l'atelier :
1. Remplacez le transistor par un **module MOSFET** (ex. IRLZ44N ou AO3400, commande logique 3,3 V).
2. Branchez une **sirène ou un gyrophare 12 V** avec son alimentation 12 V, en reliant les masses.

Le motif (bip par seconde pour « DÉCALER », triple bip pour une alarme) reste le même.
