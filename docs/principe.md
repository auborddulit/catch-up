# Principe de calcul

## Unités de pasteurisation

À chaque mesure (toutes les 2 s), on ajoute :

```
UP += dt / 60 × 10^((T − 60) / z)        avec z = 6,94 °C, si T ≥ 60 °C
```

- 1 minute à 60 °C = 1 UP ; à 66,94 °C = 10 UP par minute ; chaque degré multiplie la vitesse par 1,39.
- Rien n'est compté sous **60 °C** (seuil réglable). C'est prudent : la létalité entre 55 et 60 °C est réelle mais faible.
- En session, seules les **UP du bain** comptent. Le passage à l'air reste une marge de sécurité.

## La bouteille virtuelle

Pour les caisses sans témoin, l'appareil calcule la température à cœur avec un modèle à **deux étages** :

```
eau du bain ──k1──► périphérie de la bouteille ──k2──► cœur
```

- Chaque caisse a sa bouteille virtuelle. Elle part de la **température saisie** au démarrage du lot et chauffe avec la **température de l'eau de la place où se trouve la caisse**, mesurée en continu.
- Si le bain fluctue, la bouteille virtuelle le reflète, et les UP aussi.
- Ce modèle à deux étages reproduit le **retard au démarrage** d'une bouteille réelle (le cœur ne bouge pas tout de suite), ce que ne fait pas un modèle à un seul étage.
- **k1 et k2** sont ajustés sur une vraie courbe du témoin : méthode des moindres carrés, avec plus de poids sur la zone au-dessus de 55 °C, celle qui produit les UP.

## Quand décaler ?

Le décalage est indiqué quand **la caisse en place 3 (sortie) a atteint la cible**. Chaque caisse, à son tour en sortie, impose donc sa propre durée. Si le bain baisse, le décalage est retardé. S'il est plus chaud, il est avancé.

Pendant le remplissage du tunnel (place 3 vide), aucune caisse ne peut dicter le décalage. L'appareil calcule alors la **cadence d'équilibre** : la durée T par place telle qu'une bouteille virtuelle partant de la température saisie, et passant T dans l'eau de chacune des 3 places, reçoive exactement la cible (après correction par le témoin, s'il y en a une). La recherche se fait par dichotomie. L'eau de chaque place est lissée sur environ 5 minutes, et le calcul est refait toutes les 30 s. Ainsi :

- une cible plus haute, ou un bain plus froid, allonge la cadence dès la 1re caisse ;
- les premières caisses arrivent en sortie à peu près à la cible, sans sous-chauffe ni sur-cuisson ;
- la sécurité ne dépend pas de ce calcul : en sortie, chaque caisse attend toujours d'avoir atteint la cible.

Le décalage nominal (réglage, 6 min 40 par défaut) ne sert plus qu'en secours (sondes de bain absentes) et comme minuteur du mode Mesure.

## Le contrôle par le témoin

La 1re caisse de chaque lot porte la bouteille témoin :

1. pour **cette caisse**, ce sont les **UP mesurées** qui décident du décalage et du statut ;
2. à sa sortie, l'appareil calcule le rapport **r = UP mesurées / UP du modèle** ;
3. si **r < 1** (modèle optimiste), les UP estimées des caisses suivantes sont **multipliées par r** pour la suite du lot. La cible est donc durcie d'autant ;
4. si **r ≥ 1** (modèle prudent), **rien n'est assoupli** ;
5. si **r < 0,85**, une alarme demande de refaire l'étalonnage du modèle.

Le rapport est sensible : comme les UP sont exponentielles, un modèle qui chauffe seulement 8 % trop vite peut donner r ≈ 0,75. Cette sensibilité est voulue : elle fait apparaître tout de suite une dérive (niveau d'eau, débit, bouteille différente).

## Choix prudents, en résumé

| Situation | Choix |
|---|---|
| Sonde de bain défaillante | On prend la **plus froide** des autres sondes |
| Aucune sonde de bain | La caisse ne reçoit plus d'UP et devient **non conforme** |
| Mesure du témoin perdue un instant | L'échantillon n'est **pas compté** |
| Témoin défaillant plus de 30 s | Caisse 1 évaluée par le modèle ; **lot non conforme** (contrôle absent) |
| Coupure de courant | La durée de la coupure **n'est pas comptée** |
| Passage à l'air | **Non compté** |
| Modèle plus pessimiste que la mesure | **Pas d'assouplissement** |

## Limites à connaître

- Le témoin mesure **une bouteille, à une position, dans une caisse**. Choisissez la position la plus défavorable (voir [utilisation.md](utilisation.md)).
- Le modèle est valable **dans les conditions où il a été étalonné** : bouteille, remplissage, niveau d'eau, débit de circulation, chargement des caisses. Recommencez l'étalonnage si l'une d'elles change.
- La **température de départ** des bouteilles est saisie : une valeur trop haute rend le modèle optimiste. Le contrôle par le témoin rattrape en partie cette erreur, car le témoin part de sa vraie température.
- La **précision des sondes** est déterminante : 0,1 °C d'erreur ≈ 3,4 % d'UP. D'où l'étalonnage et la vérification mensuelle.
- Les UP mesurent l'**effet thermique**. Elles ne disent rien de la charge microbienne de départ, de l'hygiène, ni de l'étanchéité des bouteilles.

## Vérification du code

Le dossier `tests/` contient :
- `test_core.cpp` : tests du calcul (UP, modèle, suivi des caisses, annulation, défauts, ajustement) ;
- `test_firmware.cpp` : une simulation complète (mesure d'essai, étalonnage, lot de 10 caisses avec fluctuations et creux du bain, appui en double annulé), qui produit des relevés réels avec le code du firmware.

Voir [tests/README.md](../tests/README.md).
