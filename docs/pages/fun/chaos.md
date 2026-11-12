---
title: Chaos
description: Launch everyone into the air, freeze the world, Quicksilver mode.
order: 3
menu: Fun › Chaos
---

| Option | What it does | Default |
|---|---|---|
| **Everyone into the air** | NPCs within the radius fly 15 m up | — |
| **Freeze all NPCs** | the world stops, only you move | — |
| **Quicksilver mode** | everything around nearly stops, you move normally (or faster) | — |
| **World speed** (Quicksilver) | `0.05` = almost stopped | 0.12 |
| **Your speed** (Quicksilver) | `1` = normal | 1.3 |
| **Party fast too** | your party moves at your speed | — |

```trainer
{"path": "Fun › Chaos", "title": "Quicksilver", "rows": [
 {"t": "toggle", "label": "Quicksilver mode", "on": true},
 {"t": "slider", "label": "World speed", "value": 0.12, "min": 0.05, "max": 1},
 {"t": "slider", "label": "Your speed", "value": 1.3, "min": 0.5, "max": 3},
 {"t": "toggle", "label": "Party fast too", "on": false}
]}
```

```media
video: Quicksilver mode — arrows hanging in the air
```
