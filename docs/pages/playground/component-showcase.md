---
title: Component showcase
description: Every element a docs page can use, in one place. Pick what you like; the rest can go.
order: 1
---

This page is a test bed. Each section shows one element and how it is written in the markdown files under `docs/pages`.

## Text

Plain paragraphs, **bold**, *italic*, ~~strikethrough~~ and `inline code`. Links to [another docs page](doc:installation), to a [section of a page](doc:installation#requirements) and to [an external site](https://github.com/dawidseipold/gothic1-crazy-trainer).

### A third-level heading

Third- and fourth-level headings show up indented in the outline on the right.

#### A fourth-level heading

Use these sparingly; most pages only need `##` and `###`.

## Keys and status badges

Inline code with a prefix becomes a component: `key:F1` opens the menu, `key:Ctrl+K` opens search, `key:Shift+F5` is a combination.

Option status flags: `status:OK` `status:BUG` `status:CRASH` `status:TEST` `status:NEW`.

A table of key bindings:

```keys
[
  {"keys": "F1 / Insert", "action": "Open or close the trainer menu"},
  {"keys": "Ctrl+K", "action": "Search the docs"},
  {"keys": "Shift+F5", "action": "Quick save, see [Time and story](doc:time-and-story)"}
]
```

## Callouts

> **Note:** Neutral information, the default kind of remark.

> **Tip:** A shortcut or a good habit.

> **Warning:** Something that can go wrong if you are not careful.

> **Danger:** Something that can break a save or crash the game.

> **Success:** A confirmation that a step worked.

> **Note - With a custom title:** Any callout can carry its own title after a dash.

## Lists

- An unordered list
- with a second item
  - and a nested one

1. An ordered list
2. with numbers in the accent colour
3. and a third step

- [x] A task that is done
- [ ] A task that is still open

## Table

| Option | Default | What it does |
|---|---|---|
| **God mode** | off | You take no damage |
| **Infinite mana** | off | Spells cost nothing |
| **Run speed** | 1.0× | Multiplies movement speed |

## Option reference

The same information as an interactive table: click a row to see the description.

```options
{
  "God mode": {"type": "toggle", "default": "off", "status": "OK", "description": "You take no damage from anything, including falls."},
  "Run speed": {"type": "slider 0.5-5", "default": "1.0", "status": "TEST", "description": "Multiplies movement speed. Above **3** you can clip through thin walls."},
  "Free aim": {"type": "toggle", "default": "off", "status": "BUG", "description": "Aim bows and spells with the mouse. Known issue: the crosshair hides after loading a save."}
}
```

## Trainer menu replica

A drawing of a card from the in-game menu, so readers know what to look for:

```trainer
{"path": "Character › Cheats", "title": "Cheats", "rows": [
  {"t": "toggle", "label": "God mode", "on": true},
  {"t": "toggle", "label": "Infinite mana", "on": false, "note": "spells and scrolls"},
  {"t": "slider", "label": "Run speed", "value": 1.6, "min": 0.5, "max": 5, "unit": "×"},
  {"t": "select", "label": "Hotkey", "value": "F6"},
  {"t": "tabs", "items": ["Player", "Target", "Everyone"], "active": 0},
  {"t": "checks", "items": [["Humans", true], ["Monsters", false]]},
  {"t": "button", "label": "Max everything"}
]}
```

## Steps

```steps
[
  {"title": "Download the build", "body": "Get the newest `G1CrazyTrainer.zip` from the [releases page](https://github.com/dawidseipold/gothic1-crazy-trainer/releases)."},
  {"title": "Copy the DLL", "body": "Put `G1CrazyTrainer.dll` into `Gothic\\system\\Autorun`."},
  {"title": "Start the game", "body": "Press `key:F1` in game to open the menu."}
]
```

## Tabs

```tabs
[
  {"title": "Steam", "body": "Right-click Gothic in the library → **Manage** → **Browse local files**."},
  {"title": "GOG", "body": "Open GOG Galaxy → Gothic → **Manage installation** → **Show folder**."},
  {"title": "Disc", "body": "The default folder is `C:\\Program Files (x86)\\Gothic`."}
]
```

## Accordion

```accordion
[
  {"title": "Does the trainer work with Gothic 2?", "body": "Not yet. A Gothic 2 version is planned; the docs will get a switch at the top of the sidebar when it exists."},
  {"title": "Does it disable achievements?", "body": "No, Gothic 1 on Steam has no achievements."}
]
```

## File tree

```files
Gothic/
  system/
    Autorun/
      G1CrazyTrainer.dll
      G1CrazyTrainer.ini
    GothicMod.exe
  saves/
```

## Cards

```cards
[
  {"title": "Installation", "description": "Requirements and setup.", "href": "doc:installation", "icon": "RocketLaunchIcon"},
  {"title": "Cheats", "description": "God mode, infinite mana and more.", "href": "doc:cheats", "icon": "BoltIcon"}
]
```

## Code

```ini
; G1CrazyTrainer.ini
[Menu]
Hotkey=F1
Scale=1.0
Theme=Purple
```

```cpp
// A code block with syntax highlighting and a copy button
void OnFrame() {
    if (godMode) player->attribute[NPC_ATR_HITPOINTS] = player->attribute[NPC_ATR_HITPOINTSMAX];
}
```

## Media

Placeholders until real screenshots and clips are recorded:

```media
screenshot: The trainer menu open on the Character page
video: Free aim with a crossbow, about 10 seconds
```

---

That is everything. Tell me what to keep, change or drop.
