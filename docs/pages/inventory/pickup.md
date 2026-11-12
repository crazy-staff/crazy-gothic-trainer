---
title: Pickup
description: Automatic pickup around you, a list of every item lying in the world, and a pickup bot.
order: 3
menu: Ekwipunek › Zbieranie
---

## Auto-pickup

Items on the ground around you go straight to your inventory.

```trainer
{"path": "Inventory › Pickup", "title": "Auto-pickup", "rows": [
 {"t": "toggle", "label": "Pick up automatically", "on": true},
 {"t": "slider", "label": "Range", "value": 5, "min": 1, "max": 30, "unit": "m"},
 {"t": "slider", "label": "How many at once", "value": 5, "min": 1, "max": 50},
 {"t": "slider", "label": "Interval", "value": 0.5, "min": 0.1, "max": 5, "unit": "s"},
 {"t": "checks", "items": [["Weapons", true], ["Potions", true], ["Plants", true], ["Ammo", false]]}
]}
```

| Option | What it does | Default |
|---|---|---|
| **Range** | how far around you (with the animation at most 3.5 m) | 5 m |
| **How many at once** | items per pass | 5 |
| **Interval** | seconds between passes | 0.5 s |
| **Min. value** | only items worth at least this much ore (0 = everything) | 0 |
| **With the pickup animation** | the hero picks up one item at a time like in the game (slower). Skipped in no-clip and glide. | — |
| **Categories** | checkboxes for what to pick up | all |

## Items on the map

A list of **every item lying in the world** with its distance and coordinates. From the list you can teleport to an item or export the whole list to CSV.

## Pickup bot

Walks (teleports) from item to item along the shortest route and picks everything up. **Esc** stops it. The delay between items is adjustable (default 0.4 s).

```media
video: The pickup bot collecting plants around the Old Camp
```
