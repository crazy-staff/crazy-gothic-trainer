---
title: Appearance
description: Change the head, face, body, skin colour, size and weight of your hero — with a live preview.
order: 2
menu: Postać › Wygląd
---

Change how the Nameless Hero looks. All parts are in one card with a **preview** on a copy of the hero, so you can try things before applying them.

## In the menu

```trainer
{"path": "Character › Appearance", "title": "Look", "rows": [
 {"t": "value", "label": "Head", "value": 2, "min": 0, "max": 10},
 {"t": "value", "label": "Face", "value": 18, "min": 0, "max": 160, "note": "face texture number"},
 {"t": "value", "label": "Body", "value": 0, "min": 0, "max": 3, "note": "body texture"},
 {"t": "value", "label": "Skin colour", "value": 1, "min": 0, "max": 3},
 {"t": "button", "label": "Apply look"}
]}
```

## Options

| Option | What it does |
|---|---|
| **Head** | head model |
| **Face** | face texture number |
| **Body** | body texture |
| **Skin colour** | skin tone |
| **Scale** | size of the hero — `1` = normal |
| **Weight** | thin to fat — `0` = normal |

The preview updates automatically (or with a button, if you turn auto-preview off). Nothing changes on the hero until you press **Apply look**.

## See it in action

```media
screenshot: The look card with the preview of the hero
```
