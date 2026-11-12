---
title: NPC spawner
description: Spawn any character or monster of the game — or move the original to you.
order: 4
menu: Postacie › Spawner
---

Every NPC and monster of the game in a searchable list. Pick one, set how many, and they appear next to you.

```trainer
{"path": "Characters › Spawner", "title": "Spawn", "rows": [
 {"t": "text", "label": "Orc warrior", "value": "monster"},
 {"t": "value", "label": "Amount", "value": 3, "min": 1, "max": 20},
 {"t": "tabs", "items": ["Move the original", "Create a copy"], "active": 0},
 {"t": "button", "label": "Spawn"}
]}
```

- **When the character already exists** — *move the original* (the real NPC with its quests comes to you) or *create a copy*.
- **Spawn by instance name** — type any instance (like the console `insert`) to spawn something that is not on the list.
- **Spawned list** — everything you spawned, with removal; characters from the `insert` command land here too.

> **Tip:** Spawned NPCs can be added to the party right away from the list.
