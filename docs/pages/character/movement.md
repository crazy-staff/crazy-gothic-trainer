---
title: Movement
description: No-clip, glide, hero speed and sprint.
order: 4
menu: Postać › Ruch
---

Two ways to travel without the game's physics — **no-clip** flies through everything, **glide** walks on the ground — plus a speed boost for normal movement. No-clip and glide are tabs of one card: turning one on turns the other off.

## No-clip

Fly freely through walls, rocks and buildings.

```trainer
{"path": "Character › Movement", "title": "No-clip", "rows": [
 {"t": "tabs", "items": ["No-clip", "Glide"], "active": 0},
 {"t": "toggle", "label": "No-clip (flying)", "on": true, "note": "WASD, mouse turns, Space/Ctrl up/down, Shift x4; Esc turns it off"},
 {"t": "slider", "label": "Speed", "value": 9, "min": 1, "max": 100, "unit": "m/s"},
 {"t": "slider", "label": "Q/E turning", "value": 130, "min": 20, "max": 720, "unit": "°/s"},
 {"t": "toggle", "label": "Mouse turns the hero", "on": true},
 {"t": "toggle", "label": "Enemies ignore you", "on": true}
]}
```

| Key | Action |
|---|---|
| **W A S D** | move |
| **Space / Ctrl** | up / down |
| **Shift** | 4× faster |
| **Q / E** | turn |
| **Esc** | turn no-clip off — your feet land on the ground |

| Option | What it does | Default |
|---|---|---|
| **Speed** | flying speed (Shift ×4) | 9 m/s |
| **Q/E turning** | how fast Q and E turn | 130 °/s |
| **Mouse turns the hero** | moving the mouse sideways turns, up/down tilts the camera | on |
| **Mouse sensitivity** | for the option above | 1.0 |
| **Enemies ignore you** | monsters and enemies do not notice you while flying | — |

```media
video: Flying through the Old Camp wall with no-clip
```

## Glide

Walk over the ground without physics and animations — fast travel that stays on the terrain.

| Option | What it does | Default |
|---|---|---|
| **Glide (on the ground)** | turn it on; WASD, Shift ×4, Esc turns it off | — |
| **Collide with objects** | trees, rocks, bridges, grates, fences — you walk onto them instead of passing through | — |
| **Max slope** | steep walls up to this angle are climbed (90° = any wall) | 50° |
| **Speed / turning / mouse / enemies ignore you** | as in no-clip | 6 m/s |

> **Note:** Object collision on some bridges and grates is still being fixed.

## Speed

| Option | What it does | Default |
|---|---|---|
| **Hero speed** | animation speed of the hero — running, attacks, turning | 1.5× |
| **Sprint** | the running animation of the speed potion | — |
