---
title: Item ESP
description: Items and chests through walls, filtered by value.
order: 2
menu: Wizualia › ESP przedmiotów
---

## Options

| Option | What it does | Default |
|---|---|---|
| **Item ESP** | shows items lying in the world and chests | — |
| **What to show** | checkboxes by category; weapons in hands and flying arrows/bolts are hidden by default | — |
| **Max range** | items and chests | 50 m |
| **Min. value** | only items worth at least this much ore | 0 |
| **Max items / chests at once** | the closest ones | 40 / 20 |

## In the menu

```trainer
{"path": "Visuals › Item ESP", "title": "Items", "rows": [
 {"t": "toggle", "label": "Item ESP", "on": true},
 {"t": "slider", "label": "Min. value", "value": 50, "min": 0, "max": 1000, "unit": "ore"},
 {"t": "checks", "items": [["Weapons", true], ["Potions", true], ["Plants", false], ["Chests", true], ["Weapons in hands", false], ["Arrows and bolts", false]]}
]}
```
