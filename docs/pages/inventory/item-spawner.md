---
title: Item spawner
description: Find any item in the game and put it in your inventory — weapons, armour, runes, potions, quest items.
order: 1
menu: Ekwipunek › Spawner
---

Every item of the game in one searchable list, sorted into categories (melee weapons, ranged weapons, armour, runes, scrolls, potions, food, plants, keys, amulets and rings, quest items…). Each item shows its icon and price in ore.

```trainer
{"path": "Inventory › Spawner", "title": "Items", "rows": [
 {"t": "tabs", "items": ["All", "Weapons", "Armour", "Magic", "Potions", "Other"], "active": 1},
 {"t": "text", "label": "Ore Baron's sword", "value": "1000 ore"},
 {"t": "value", "label": "Amount", "value": 1, "min": 1, "max": 100},
 {"t": "button", "label": "Add to inventory"}
]}
```

- **Search** by name, filter by category, set the **amount** and add the item.
- **Add ore** gives you nuggets of ore directly.
- Item icons are generated in the game the first time — you can regenerate them in [Compendium](doc:compendium).

```media
screenshot: The item list with icons and categories
```
