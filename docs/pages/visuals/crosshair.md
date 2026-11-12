---
title: Crosshair
description: A crosshair editor with four shapes, colours and presets.
order: 4
menu: Wizualia › Celownik
---

```trainer
{"path": "Visuals › Crosshair", "title": "Crosshair", "rows": [
 {"t": "tabs", "items": ["Cross", "Dot", "Circle", "Corners"], "active": 0},
 {"t": "slider", "label": "Gap", "value": 4, "min": 0, "max": 40, "unit": "px"},
 {"t": "slider", "label": "Length", "value": 12, "min": 1, "max": 60, "unit": "px"},
 {"t": "slider", "label": "Thickness", "value": 2, "min": 1, "max": 10, "unit": "px"},
 {"t": "toggle", "label": "Centre dot", "on": true}
]}
```

Each shape shows only the settings that make sense for it.

| Option | What it does | Default |
|---|---|---|
| **Shape** | cross, dot, circle, corners | cross |
| **Gap** | distance from the centre (circle: radius) | 4 px |
| **Length** | | 12 px |
| **Thickness** | | 2 px |
| **Rotation** / **Spin** | fixed angle / degrees per second (0 = still); not for dot and circle | 0 |
| **Opacity** | | 100% |
| **Dot size** / **Dot shape** | circle, square, diamond | 2 px |
| **Colour**, **outline** | | |

## Presets

Four ready presets plus your own: save the current crosshair (with its colour) under a name and load it with one click. Presets are stored in `G1CrazyTrainer_celowniki.txt`.
