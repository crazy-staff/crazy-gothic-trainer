---
title: Information
description: Name, guild, level, attributes and skills of your hero — and quick buttons to raise them.
order: 1
menu: Postać › Informacje
---

The **Information** page shows your hero at a glance — portrait, name, guild, level, experience, health, mana and all attributes — and lets you change them directly.

```trainer
{"path": "Character › Information", "title": "Progress", "rows": [
 {"t": "value", "label": "Add experience", "value": 1000, "min": 0, "max": 100000, "note": "through the game's own XP function — level-ups work"},
 {"t": "value", "label": "Add learning points", "value": 10, "min": 0, "max": 200},
 {"t": "select", "label": "Guild", "value": "Shadow"},
 {"t": "button", "label": "Max everything"}
]}
```

## Stats

Every attribute (strength, dexterity, max HP, max mana) has a field and a **Set** button. Values are written straight to the hero.

## Progress

| Option | What it does |
|---|---|
| **Add experience** | gives XP through the game's own function, so level-ups, learning points and messages work as in the game. Always applies to the character you control. |
| **Add learning points** | adds learning points (LP) to spend at teachers. |
| **Guild** | changes the hero's guild — NPCs react to the new guild right away. |

## Skills

**Max everything** (button in the card header) sets every skill to the maximum, the 6th circle of magic and 100% hit chance in one click.

> **Tip:** Changing the guild changes how camps react to you — set it back if guards suddenly attack.
