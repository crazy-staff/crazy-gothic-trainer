---
title: Character ESP
description: Boxes, names, health, weapons and lines to characters — through walls.
order: 1
menu: Wizualia › ESP postaci
---

Shows characters through walls, with exactly the details you tick.

```trainer
{"path": "Visuals › Character ESP", "title": "What to show", "rows": [
 {"t": "toggle", "label": "Character ESP", "on": true},
 {"t": "checks", "items": [["Box", true], ["Name", true], ["Health", true], ["Distance", true], ["Skeleton", false], ["Equipment", false], ["Who it attacks", true], ["Type tag", true]]},
 {"t": "slider", "label": "Max range", "value": 50, "min": 5, "max": 200, "unit": "m"},
 {"t": "slider", "label": "Max characters at once", "value": 40, "min": 5, "max": 150},
 {"t": "select", "label": "Box type", "value": "Corners"}
]}
```

| Option | What it does | Default |
|---|---|---|
| **What to show** | checkboxes: box, name, health, distance, skeleton, equipment, *who it attacks* (text and arrow to its fight target), type tag ([Arena], [Party], [Spawned]) | — |
| **Show** | filter: everyone, enemies, party… | everyone |
| **Max range** | | 50 m |
| **Max characters at once** | the closest ones; fewer = lighter for slower PCs | 40 |
| **Box type** | 2D box, corners, 3D box | — |

## Arrows and edge markers

- **Arrows around the crosshair** point to characters on a circle (radius 140 px by default).
- **Edge markers** sit on the screen edge where a box left the screen.

Colours are set in [Colours](doc:colours). A **preview** at the top of the page shows the result right away.

```media
screenshot: ESP with boxes, names and health in the New Camp
```
