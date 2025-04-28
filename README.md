# area_matching

## Context

Open Maps For Europe 2 est un projet qui a pour objectif de développer un nouveau processus de production dont la finalité est la construction d'un référentiel cartographique pan-européen à grande échelle (1:10 000).

L'élaboration de la chaîne de production a nécessité le développement d'un ensemble de composants logiciels qui constituent le projet [OME2](https://github.com/openmapsforeurope2/OME2).


## Description

Le présent outil est dédié à la mise en cohérence des surfaces de deux pays autour de leur(s) frontière(s) commune(s).

Lorsqu'elle est lancée l'application traite un couple de pays frontaliers. Pour raccorder l'ensemble du réseau d'un pays le programme doit être lancé successivement sur ses différentes frontières (en considérant l'ensemble de ses pays limitrophes).


## Fonctionnement

Le programme ne manipule pas directement les données de production. Les données à traiter, localisées autour de la frontière, sont extraites dans une table de travail. A l'issu du traitement les données dérivées sont injectées dans la table source en remplacement des données initiales.

Le processus de mise en cohérence est décomposé en plusieurs étapes. Un numéro est attribué à chaque étape. Une table de travail préfixée de ce numéro est délivrée en sortie de chaque étape. Chaque étape prend en données d'entrées les tables de travail générées lors d'étapes antérieures.

Voici l'ensemble des étapes constitutives du processus de raccordement:

**301** - ajout des surfaces de la table standing_water dans la table watercourse_area
<br>
**310** - génération des _'cutting lines'_
<br>
**320** - suppression des surfaces hors pays
<br>
**330** - nettoyage des _'cutting lines'_ orphelines
<br>
**334** - génération de surfaces par intersection des surfaces des deux pays. Ces surfaces sont stockées dans une table dédiée.
<br>
**335** - génération des _'cutting point'
<br>
**340** - fusion des surfaces des deux pays présentant des zones de chevauchement
<br>
**350** - découpe des surfaces fusionnées avec les _'cutting lines'_ et les sections générées à partir des _'cutting point'
<br>
**360** - affectation des attributs aux surfaces résultant de la fusion/découpe
<br>
**370** - fusion des petites surfaces et des surfaces possédant les mêmes attributs
<br>
**399** - extraction des surfaces ajoutées à l'étape 301 et réintégration dans leur table d'origine standing_water

> _Précisions_ :
> - _'Cutting line' : arc représentant la portion de contour partagé entre deux surface adjacente d'un même pays. Cet arc est utilisé pour la découpe de surfaces._
> - _'Cutting point' : point particulier du contour d'une surface. Ce point est utilisé comme référence pour créer une section de découpe de surface._

Pour le moment, seul le thème hydrographie est concerné par ce traitement.


## Configuration

L'outil s'appuie sur de nombreux paramètres de configuration permettant d'adapter le comportement des algorithmes en fonctions des spécificités nationales (sémantique, précision, échelle, conventions de modélisation...).

On trouve dans le [dossier de configuration](https://github.com/openmapsforeurope2/area_matching/tree/main/config) les fichiers suivants :

- epg_parameters.ini : regroupe des paramètres de base issus de la bibliothèque libepg qui constitue le socle de développement l'outil. Ce fichier est aussi le fichier chapeau qui pointe vers les autres fichiers de configurations.
- db_conf.ini : informations de connexion à la base de données.
- theme_parameters.ini : configuration des paramètres spécifiques à l'application.


## Utilisation

L'outil s'utilise en ligne de commande.

Paramètres:
* c [obligatoire] : chemin vers le fichier de configuration
* cc [obligatoire] : code pays double (exemple : be#fr)
* sp [obligatoire] : étape(s) à executer (exemples: 320 ; 320,340 ; 310-399)

<br>

Exemple d'appel pour lancer successivement l'ensemble des étapes sur la frontière franco-belge :
~~~
bin/area_matching --c path/to/config/epg_parmaters.ini --cc be#fr
~~~

Exemple d'appel pour ne lancer qu'une seule étape :
~~~
bin/area_matching --c path/to/config/epg_parmaters.ini --cc be#fr --sp 350
~~~