---
title: Graphics
description: Colour filters (brightness, gamma, contrast…), GD3D11 draw distances and graphics presets.
order: 5
menu: Wizualia › Grafika
---

## Image

A colour pass over the whole game.

```trainer
{"path": "Visuals › Graphics", "title": "Image", "rows": [
 {"t": "select", "label": "Filter", "value": "None"},
 {"t": "slider", "label": "Brightness", "value": 0, "min": -50, "max": 50, "unit": "%"},
 {"t": "slider", "label": "Gamma", "value": 100, "min": 50, "max": 200, "unit": "%"},
 {"t": "slider", "label": "Contrast", "value": 100, "min": 50, "max": 200, "unit": "%"},
 {"t": "slider", "label": "Saturation", "value": 100, "min": 0, "max": 200, "unit": "%"}
]}
```

| Option | Default |
|---|---|
| **Filter** — none, black and white, sepia, night vision, negative | none |
| **Brightness** | 0% |
| **Gamma** (more = brighter shadows) | 100% |
| **Contrast** / **Saturation** | 100% |
| **Colour temperature** (− colder / + warmer) | 0 |
| **Sharpening** / **Vignette** | 0% |
| **Restore defaults** | |

## GD3D11

Settings of the GD3D11 renderer, changed live.

| Option | Default |
|---|---|
| **Object draw distance** | 300 m |
| **Small objects** (grass, stones) | 100 m |
| **Characters** / **Effects** | 100 m |
| **World** — terrain sections around you | 4 |
| **Fog density**, **Sun**, **Shadows**, **Wind** — % of the game's value | 100% |

## Presets

Ready presets and your own (saved in `G1CrazyTrainer_grafika.txt`) store all of the settings above at once.

```media
screenshot: Night vision filter in the Old Mine
```
