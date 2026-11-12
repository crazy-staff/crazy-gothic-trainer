---
title: Party
description: Companions that follow you, fight for you and do not fall out of the party.
order: 2
menu: Postacie › Drużyna
---

Anyone can join your party — humans and monsters. Party members follow you, fight on your side and can be ordered around.

```trainer
{"path": "Characters › Party", "title": "Party", "rows": [
 {"t": "text", "label": "Thorus", "value": "following"},
 {"t": "text", "label": "Wolf", "value": "following"},
 {"t": "toggle", "label": "Keep the party together", "on": true, "note": "every 3 s: whoever stopped following comes back"},
 {"t": "toggle", "label": "No talking", "on": true},
 {"t": "toggle", "label": "Stop fighting after teleport", "on": true}
]}
```

## Building the party

| Option | What it does |
|---|---|
| **Add to party** | on the [Target](doc:target) page, or from the selection list here (radius 30 m) |
| **Everyone around to the party** | everybody within the radius joins and follows you |
| **Disband** | allies go back to their routines |
| **Party list** | each member with portrait and actions: follow, wait (stays exactly where told), teleport to you, remove from the world |

## Keeping them with you

| Option | What it does |
|---|---|
| **No talking** | party members do not start dialogues with you (that used to pull them out of the party) |
| **Keep the party together** | every 3 s whoever stopped following comes back; whoever is too far (default 30 m) is moved to you |
| **Stop fighting after teleport** | a companion moved to you follows you instead of running back to its enemy |

## Fights

| Option (off = …) | When it is off |
|---|---|
| **Party can fight each other** | party members do not notice or hurt each other |
| **Party monsters attack neutrals** | your wolf bites only enemies |
| **Party humans attack neutrals** | they fight only enemies |
| **Neutrals attack party monsters** | nobody neutral touches your monster |

> **Note:** Fights inside the party are still being tuned — a monster can sometimes stay locked on its last target.
