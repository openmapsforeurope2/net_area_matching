# area_matching

## Context: Ome2

Thèmes

## Description

L'outil area_matching est dédié à la mise en cohérences des surfaces de deux pays autour de leur(s) frontière(s) commune(s)

## Configuration


## Etapes

Travail sur une frontière (2 pays)
Etape préliminaire d'extraction du réseau autour des frontières

On travail sur la table de travail. A chaque étape une table de travail intermédiaire préfixée du numéro d'étape est créée.

Pour le moment, seul le thème hydrographie est traité

301 : ajout des surfaces de la table standing_water dans la table watercourse_area
310 : génération des 'cutting lines'
320 : suppression des surfaces hors pays
330 : nettoyage des 'cutting lines' orphelines
334 : génération de surfaces par intersection des surfaces des deux pays. Ces surfaces sont stockées dans une table dédiée.
335 : génération des 'cutting point'
340 : fusion des surfaces des deux pays présentant des zones de chevauchement
350 : découpe des surfaces fusionnées avec les 'cutting lines' et les sections générées à partir des 'cutting point'
360 : affectation des attributs aux surfaces résultant de la fusion/découpe
370 : fusion des petites surfaces et des surfaces possédant les mêmes attributs
399 : extraction des surfaces ajoutées à l'étape 301 et réintégration dans leur table d'origine standing_water



## dépendances
## compilation
## utilisation


## Définition

'Cutting line' : 
'Cutting point' :