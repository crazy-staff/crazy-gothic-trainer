---
title: Magic
description: Thanos snap, meteors and spell rain.
order: 2
menu: Fun › Magia
---

```trainer
{"path": "Fun › Magic", "title": "Spell rain", "rows": [
 {"t": "toggle", "label": "Spell rain", "on": true, "note": "spells fall on random characters around you"},
 {"t": "slider", "label": "Damage", "value": 80, "min": 1, "max": 1000},
 {"t": "slider", "label": "Every", "value": 0.6, "min": 0.1, "max": 5, "unit": "s"},
 {"t": "slider", "label": "Radius", "value": 30, "min": 5, "max": 100, "unit": "m"},
 {"t": "select", "label": "At whom", "value": "Enemies"}
]}
```

## Thanos snap

NPCs within the radius turn to dust. Choose **who** (everyone, enemies, monsters…), **what share** of them (%), and whether immortal ones count.

## Meteor

A rain of fire on everybody within the radius (damage 150 by default, optionally immortal ones too).

## Spell rain

Spells fall on random characters around you while it is on.

| Option | Default |
|---|---|
| **Damage** | 80 |
| **Every** | 0.6 s |
| **Targets at once** | 1 |
| **Radius** | 30 m |
| **At whom** | filter |
| **Also immortal** | — |

> **Note:** The number of spells in the air is limited to keep the game stable.
