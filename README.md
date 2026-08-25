# gas-payload-node

![CI](https://github.com/hh1ll/gas-payload-node/actions/workflows/ci.yml/badge.svg)

Nœud de charge utile de détection de gaz pour aéronef autonome — surveillance
de fuites de méthane depuis un drone ou un dirigeable.

> **État : séance 2 / 6.** Socle logique, tests unitaires sur hôte,
> vérification automatique à chaque modification.
> Ni matériel ni intégration PX4 pour l'instant.

## Ce que fait ce dépôt aujourd'hui

Une bibliothèque `core` sans aucune dépendance matérielle, testée sur PC.
Le premier composant est `GasMonitor` : une machine à états à hystérésis qui
décide quand lever une alerte de concentration, sans que l'alarme clignote
quand le capteur oscille autour du seuil.

## Compiler et tester

```bash
cmake -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Structure

```
include/core/   interfaces publiques de la bibliothèque
src/core/       implémentation — aucun #include matériel, jamais
test/           tests unitaires GoogleTest, exécutés sur hôte
```

La contrainte « aucun `#include` matériel dans `core/` » n'est pas une
préférence de style : c'est elle qui permettra, en séance 2, de faire tourner
la totalité de ces tests sur un serveur d'intégration continue où aucun
capteur n'est branché.

## Feuille de route

- [x] **1** — socle CMake, `GasMonitor`, tests sur hôte
- [x] **2** — intégration continue GitHub Actions
- [ ] **3** — couche d'abstraction matérielle et implémentations factices
- [ ] **4** — télémétrie MAVLink, visualisation dans QGroundControl
- [ ] **5** — PX4 SITL en conteneur, test d'intégration automatisé
- [ ] **6** — driver capteur réel sur STM32, puis test hardware-in-the-loop
