---
title: Combat and arena
description: Arena with waves and rewards, everyone vs everyone, peace, zombie apocalypse and polymorph.
order: 1
menu: Fun › Walka
---

## Arena

Waves of enemies appear 6–10 m around you. They only **knock you down** — being knocked down means you lost.

```trainer
{"path": "Fun › Combat", "title": "Arena", "rows": [
 {"t": "slider", "label": "Arena mode", "value": 3, "min": 1, "max": 20, "note": "starting wave"},
 {"t": "value", "label": "Number of waves", "value": 10, "min": 0, "max": 50, "note": "0 = endless"},
 {"t": "toggle", "label": "Rewards after a wave", "on": true, "note": "ore + 1-2 random items"},
 {"t": "button", "label": "Start"}
]}
```

| Option | What it does | Default |
|---|---|---|
| **Arena mode** | the starting wave | 1 |
| **Number of waves** | 0 = endless | 0 |
| **Rewards after a wave** | ore and 1–2 random items | on |

Arena enemies are named *Arena: …* so you can tell them apart.

## Factions

| Option | What it does |
|---|---|
| **Everyone vs everyone** | every guild is hostile to every other; turning it off makes peace within 60 m |
| **Random guilds** | humans within the radius get a random guild |
| **Peace** | everybody within the radius stops fighting, including with you |
| **Everyone vs you** | while on, everybody within the radius (default 30 m) attacks you; off = peace |
| **Kill enemies in radius** | optionally immortal ones too |

## Zombie apocalypse

The dead rise around you.

| Option | What it does | Default |
|---|---|---|
| **Apocalypse** | how many undead rise | — |
| **Spawn distance** | metres from you | 10 m |
| **Health** | HP of each (±25%) | 150 |
| **Protection** | against every damage type | 20 |
| **Can be killed** | off = they are immortal | on |
| **Remove the undead of the apocalypse** | only those created by this option | — |

Story undead are never touched.

## Polymorph

Turns characters within the radius into creatures (and the other way round) — with effects and a choice of targets. At most 30 at once by default.

> **Warning:** Polymorph **removes** the original NPCs — quests that need them can break.

```media
video: An arena wave and the zombie apocalypse around the Old Camp
```
