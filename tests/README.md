# Tests

Deux programmes à compiler sur PC. Ils utilisent **le même code** que le firmware :

| Test | Ce qu'il vérifie |
|---|---|
| `test_core` | Formule des UP, bouteille virtuelle, suivi des caisses, cadence, correction par le témoin, sortie trop tôt et annulation, défauts de sondes, ajustement du modèle, mode mesure. |
| `test_firmware` | Simulation complète avec le vrai code de l'appareil (`app.cpp`, `report.cpp`…) et des modules matériels simulés (`shim/`). Déroulé : mesure d'essai à 6 min 40 → étalonnage du modèle → lot de 10 caisses avec bain qui oscille, creux de 4 min sur la place 1, appui en double puis annulé → clôture. Les relevés produits sont écrits dans `out/sd/`. |

```sh
make ARDUINOJSON=/chemin/vers/ArduinoJson/src
```

Les exemples du dossier `docs/exemples/` viennent de cette simulation.
