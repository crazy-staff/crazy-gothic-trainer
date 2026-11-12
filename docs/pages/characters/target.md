---
title: Target
description: Actions on the NPC you are looking at — party, body swap, pull, teleport, kill, revive, rob, spells and more.
order: 1
menu: Postacie › Cel
---

The **target** is the NPC under your view (the game's focus) or one you saved to the list. The card on top shows its portrait, name, guild, HP and what it is doing.

```trainer
{"path": "Characters › Target", "title": "Diego — Shadow", "rows": [
 {"t": "button", "label": "Add to party"},
 {"t": "button", "label": "Body swap"},
 {"t": "button", "label": "Pull to me"},
 {"t": "button", "label": "Revive / heal"}
]}
```

## Saved targets

**Save target** puts the NPC you look at on a list, so you can act on it from anywhere. **Use the NPC in view** goes back to whatever is under your view (the list stays).

## Actions

The buttons change with the state of the target — for example *Add to party* becomes *Remove from party*.

| Action | What it does |
|---|---|
| **Add to party** | the target fights on your side and follows you (mind control) — see [Party](doc:party) |
| **Body swap** | you play as the target; go back with the button in the menu header |
| **Pull to me** / **Teleport to target** | move it to you, or you to it |
| **Launch into the air** | it flies up (height in the action settings, default 15 m) |
| **Kill** | works on immortal NPCs too |
| **Revive / heal** | revives the same character in place — inventory and quests stay |
| **Rob to zero** | the target's whole inventory goes to you |
| **Make a portrait** | a new picture of the target for the list and the [Compendium](doc:compendium) |
| **Freeze / unfreeze** | only this one NPC stands still |
| **Cast selected spells** | random spells from those ticked in the *Magic on target* card, only at this target (damage adjustable, can hit immortal NPCs) |
| **Remove from the world** | the NPC disappears |

> **Warning:** Removing or killing story characters can break quests. Back up your save first.
