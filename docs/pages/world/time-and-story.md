---
title: Time and story
description: World speed, time of day, stopping time and jumping to a chapter.
order: 1
menu: Świat › Czas i fabuła
---

```trainer
{"path": "World › Time and story", "title": "Time", "rows": [
 {"t": "slider", "label": "World speed", "value": 0.3, "min": 0.1, "max": 3, "note": "0.1 = slow-mo, 3 = turbo"},
 {"t": "select", "label": "What slows down", "value": "Characters around"},
 {"t": "toggle", "label": "Party at normal speed", "on": true},
 {"t": "value", "label": "Set the hour", "value": 12, "min": 0, "max": 23},
 {"t": "toggle", "label": "Stop the time of day", "on": false}
]}
```

| Option | What it does |
|---|---|
| **World speed** | slow motion or turbo — `0.1` to `3` |
| **What slows down** | the whole game or only the characters around you |
| **Party at normal speed** | your party ignores the slow motion (only in *characters around* mode) |
| **Set the hour** | 0–23; your party and spawned characters stay with you |
| **Stop the time of day** | the clock stands still |
| **Jump to a chapter** | runs the game's chapter change |

> **Warning:** Jumping to a chapter runs the same script as the story does — it can break quests of the chapters you skip.
