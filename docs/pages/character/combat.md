---
title: Combat
description: Attack speed, infinite ammo, free aim with an over-the-shoulder camera, aimbot and trigger bot.
order: 5
menu: Postać › Walka
---

## Attack speed and ammo

| Option | What it does | Default |
|---|---|---|
| **Attack speed** | animation speed only while a weapon is drawn — swings, drawing a bow, reloading a crossbow, casting | 1.8× |
| **Infinite ammo** | arrows and bolts refill by themselves while a bow or crossbow is drawn | — |

## Free aim

Aim bows, crossbows and spells yourself. The camera moves **over the right shoulder**, the mouse aims **up and down**, and the shot flies at the character under the crosshair — also above or below you.

```trainer
{"path": "Character › Combat", "title": "Free aim", "rows": [
 {"t": "toggle", "label": "Free aim", "on": true, "note": "bow, crossbow, spells"},
 {"t": "slider", "label": "Mouse up/down sensitivity", "value": 1, "min": 0.2, "max": 4},
 {"t": "slider", "label": "Camera distance", "value": 2.4, "min": 1, "max": 6, "unit": "m"},
 {"t": "slider", "label": "Side offset", "value": 45, "min": -120, "max": 120, "unit": "cm"},
 {"t": "toggle", "label": "Highlight the target under the crosshair", "on": true}
]}
```

```media
video: Shooting a scavenger on a hill with free aim
```

## Aimbot and trigger bot

Both **need free aim** and are tabs of one card — only one can be on.

```trainer
{"path": "Character › Combat", "title": "Aim assist", "rows": [
 {"t": "tabs", "items": ["Aimbot", "Trigger bot"], "active": 0},
 {"t": "toggle", "label": "Aimbot", "on": true, "note": "requires free aim"},
 {"t": "select", "label": "Key", "value": "Mouse side button 4"},
 {"t": "select", "label": "Priority", "value": "Closest to the crosshair"},
 {"t": "slider", "label": "Field of view", "value": 90, "min": 5, "max": 360, "unit": "°"},
 {"t": "slider", "label": "Range", "value": 40, "min": 2, "max": 100, "unit": "m"},
 {"t": "checks", "items": [["Humans", true], ["Monsters", true], ["Party", false], ["Neutral", false]]}
]}
```

### Aimbot

While the key is held, it turns the hero to the target and puts the crosshair on it.

| Option | What it does | Default |
|---|---|---|
| **Key** | *Always (with a bow / spell)*, mouse side button 4 or 5, middle button, Caps Lock, V | side button 4 |
| **Delay after pressing** | wait before it locks on | 0 ms |
| **Priority** | closest to the crosshair, closest to you, lowest HP, highest HP | crosshair |
| **Field of view** | degrees around where you look | 90° |
| **Range** | max distance | 40 m |
| **Stick to the target** | do not switch while the target is alive and in range | — |
| **Also hidden targets** | targets behind walls | — |

### Trigger bot

Shoots or casts by itself when an allowed target is under the crosshair.

| Option | What it does | Default |
|---|---|---|
| **Reaction time** | delay before the shot | 120 ms |
| **Time between shots** | minimum interval | 450 ms |
| **Attack keys in the game** | *Ctrl + up arrow* or *Left mouse button + W* — as set in your controls | Ctrl + up |

**Targets** are chosen with checkboxes (humans, monsters, party, neutral…), for both modes.
