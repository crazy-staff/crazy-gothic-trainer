---
title: Console
description: Run game console commands and scripts, Marvin mode, waynet and frame info.
order: 1
menu: Narzędzia › Konsola
---

Type Gothic console commands (like `insert`, `goto`, `set time`) in a text field instead of the F2 console, and keep a history of what you ran.

```trainer
{"path": "Tools › Console", "title": "Console", "rows": [
 {"t": "text", "label": "> insert ORC_WARRIOR", "value": "ok"},
 {"t": "toggle", "label": "Marvin mode", "on": false, "note": "F2 console and debug keys, without typing MARVIN"},
 {"t": "toggle", "label": "Show the waynet", "on": false, "note": "NPC paths"},
 {"t": "toggle", "label": "Frame info", "on": false}
]}
```

| Option | What it does |
|---|---|
| **Marvin mode** | the game's F2 console and debug keys without typing MARVIN |
| **Show the waynet** | draws the paths NPCs walk on |
| **Frame info** | the game's frame statistics |

> **Tip:** Characters created with `insert` show up on the [NPC spawner](doc:npc-spawner) list.
