# HACCP : utiliser compteur-up pour surveiller le CCP « pasteurisation »

Cette page résume comment l'appareil s'intègre dans un plan HACCP. C'est un modèle à adapter à votre produit et à votre procédé. Il ne remplace ni la validation par un laboratoire ou un centre technique, ni votre propre analyse des dangers.

## Ce que l'appareil apporte

| Exigence HACCP | Réponse de l'appareil |
|---|---|
| Limite critique mesurable | UP au cœur (bain seul) ≥ cible, pour **chaque caisse** |
| Surveillance continue | Mesure toutes les 2 s ; témoin réel dans la 1re caisse de chaque lot ; bouteille virtuelle pour les autres |
| Détection des écarts | Alarmes (buzzer + téléphone) : sortie avant la cible, bain froid, sonde défaillante, écart témoin / modèle |
| Actions correctives tracées | Annulation horodatée, champ « actions correctives » sur le relevé |
| Enregistrements | Relevé par lot (HTML / PDF à viser) + CSV bruts, conservés sur la carte SD |
| Vérification | Étalonnage des sondes daté sur chaque relevé ; rapport témoin / modèle à chaque lot |

## Dangers

- **Cible du CCP** : levures, moisissures végétatives et bactéries d'altération. Leur survie peut entraîner une refermentation en bouteille, donc une **surpression et un risque d'éclatement du verre**. Les bactéries pathogènes végétatives acido-tolérantes sont aussi visées.
- **Non maîtrisés par ce CCP** :
  - les ascospores de moisissures thermorésistantes (Byssochlamys, Neosartorya, Talaromyces), à maîtriser par les matières premières et l'hygiène ;
  - les spores bactériennes. Le pH < 4,6 empêche le développement de C. botulinum, il doit donc être **contrôlé à chaque lot**.

## Validation de la cible : étapes conseillées

1. Trouver le point froid : hauteur de la sonde et position dans la caisse (mode Mesure, sonde auxiliaire).
2. Mesurer les UP du procédé actuel : au moins 3 mesures, retenir la plus faible.
3. Fixer la cible : par exemple UP actuelles + 10 %, pour ne pas faire moins bien qu'aujourd'hui.
4. Faire confirmer la cible par un laboratoire ou un centre technique : données de thermorésistance des levures, ou test d'épreuve.
5. Contrôler la stabilité sur 3 lots : étuvage, par exemple 14 jours à 30 °C, et dénombrements.
6. Revalider à chaque changement de recette, de bouteille, de remplissage, de niveau d'eau, de circulation ou de cadence.

## Vérifications périodiques

- **Sondes** : comparaison mensuelle avec un thermomètre de référence étalonné. Écart toléré ≤ 0,2 °C.
- **Modèle** : rapport témoin / modèle ≥ 0,85 à chaque lot.
- **Appareil** : essai à blanc tous les 6 mois et après toute mise à jour du firmware.
- **Produit fini** : analyses ou étuvage à une fréquence définie dans le plan.

Une fiche HACCP complète (dangers, limites, surveillance, actions correctives, validation, vérification, enregistrements) accompagne ce projet sous forme de document modifiable.
