---
title: Widgets, radar and compass
description: A dock of on-screen widgets — health and mana, radar, threats, target, ammo, XP, info — and a compass.
order: 3
menu: Wizualia › Widżety
---

All widgets live in **one dock** in a corner of the screen. A **preview** in the right proportions of your screen shows where everything goes.

```trainer
{"path": "Visuals › Widgets", "title": "Dock", "rows": [
 {"t": "select", "label": "Position", "value": "Top right"},
 {"t": "slider", "label": "Width", "value": 230, "min": 150, "max": 500, "unit": "px"},
 {"t": "slider", "label": "Opacity", "value": 80, "min": 10, "max": 100, "unit": "%"},
 {"t": "checks", "items": [["Health / mana", true], ["Radar", true], ["Threats", true], ["Target", true], ["Ammo", false], ["XP", false], ["Info", true]]}
]}
```

## Dock

| Option | Default |
|---|---|
| **Position** — any screen corner | top left |
| **Width** | 230 px |
| **Opacity** | 80% |
| **Also with the menu open** | — |

## Widgets

| Widget | What it shows |
|---|---|
| **Health / mana** | bars of your hero |
| **Radar** | characters, items and chests around you (range 60 m), rotating with the hero; what to show is chosen with checkboxes |
| **Threats** | enemies attacking you within 30 m |
| **Target** | health bar of what you are fighting or aiming at |
| **Ammo** | arrows / bolts left |
| **Experience** | progress to the next level |
| **Weapon / FPS** | drawn weapon, frames per second |
| **Info** | the lines you pick: FPS and frame-time graph, coordinates, speed, time, world, HP/mana, target, memory, character and item counts, heading |

## Compass

A bar at the top of the screen with markers of characters (its own *what to show*, separate from the radar).

| Option | Default |
|---|---|
| **Width** | 440 px |
| **Range** — degrees visible on the bar | 120° |

```media
screenshot: The widget dock with radar, threats and the compass
```
