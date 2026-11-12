---
title: Diagnostics
description: Screenshots, rescue tools when something gets stuck, and the party test scene.
order: 3
menu: Narzędzia › Diagnostyka
---

## Screenshots

| Option | Key | What it does |
|---|---|---|
| **Full screenshot** | F11 | the whole screen with the menu — to the clipboard and `G1CrazyTrainer_zrzuty` |
| **Area screenshot** | Shift + F11 | select a rectangle with the mouse (the image freezes; Esc cancels) |

Paste a screenshot into a report with **Ctrl + V** in the Tester hub.

## Rescue

When something gets stuck:

| Button | What it does |
|---|---|
| **Close a stuck conversation** | when a dialogue window does not react |
| **Unlock the hero** | turns no-clip / glide off, restores physics and collisions, puts you on the ground |
| **Unlock the target** | stops the target's actions, wakes it up, puts it on its feet |

```trainer
{"path": "Tools › Diagnostics", "title": "Rescue", "rows": [
 {"t": "button", "label": "Close a stuck conversation"},
 {"t": "button", "label": "Unlock the hero"},
 {"t": "button", "label": "Unlock the target"}
]}
```

## Party test scene

Creates a test scene for party fights: Thorus and a wolf in your party, a guard, a bandit, a digger and a molerat as neutrals, a scavenger as an enemy. **Remove the test scene** cleans it up.
