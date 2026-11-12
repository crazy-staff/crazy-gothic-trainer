---
title: The menu
description: How the menu is organised, keys, hotkeys, search and presets.
order: 3
---

Press **Insert** to open or close the menu. By default the world stops while the menu is open (**Settings › Behaviour › Pause while the menu is open**).

## Keys

| Key | Action |
|---|---|
| **Insert** | open / close the menu |
| **Esc** | close the menu, leave no-clip or glide, stop portrait generation |
| **F11** | screenshot (with the menu) — copied to the clipboard and saved to `G1CrazyTrainer_zrzuty` |
| **Shift + F11** | screenshot of a selected area (the image freezes while you select) |
| **Ctrl + F** | search in the current page |

## Layout

The sidebar on the left has the sections (**Character**, **Inventory**, **Characters**, **Fun**, **World**, **Visuals**, **Tools**). Each has a few pages, and every page is made of **cards** with options.

```trainer
{"path": "Character › Movement", "title": "Movement", "rows": [
 {"t": "tabs", "items": ["No-clip", "Glide"], "active": 0},
 {"t": "toggle", "label": "No-clip (flying)", "on": false, "note": "through walls; WASD, Space/Ctrl up/down, Shift x4"},
 {"t": "slider", "label": "Speed", "value": 9, "min": 1, "max": 100, "unit": "m/s"},
 {"t": "toggle", "label": "Enemies ignore you", "on": true}
]}
```

- **Switches** turn an option on and off, **sliders and values** set how strong it is, **buttons** do something once.
- Some cards have **tabs** for modes that cannot be on together (for example *No-clip | Glide*, *Aimbot | Trigger bot*). Turning one on turns the other off.
- Options that need another one say so — e.g. the aimbot needs free aim.

## Hotkeys

Every option can get its own key: click the hotkey field next to the option and press a key or a combination (Ctrl / Alt / Shift + key). Press the same field again to clear it.

## Search

Every page has a search box. It filters cards and options as you type and — if you want — remembers what you typed when you switch pages (**Settings › Remember search**).

## Settings

The gear at the bottom of the sidebar opens **Settings** (*Konfiguracja*), split into tabs:

| Tab | What is there |
|---|---|
| **Menu look** | menu scale, opacity, corner rounding, dimming the game behind the menu, accent colour |
| **Behaviour** | remember search per page, pause while the menu is open, restore the switches that were on last time |
| **Windows and pictures** | a small status window listing everything that is on; make icons of all items |
| **Hotkeys** | every hotkey you set, in one list |
| **Tester mode** | status dots and report flags next to options, the Tester hub — see [Testing](doc:testing) |

```media
screenshot: The menu with the sidebar, a page of cards and the search box
```
