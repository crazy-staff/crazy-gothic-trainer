---
title: Testing and reporting
description: Help test new builds with the in-game Tester hub.
order: 1
---

Thanks for helping! Everything you need is in the game — in the **Tester hub**. The in-game menu is in Polish for now, so the Polish names are given in *italics*.

## 1. Turn on tester mode

1. In the game press **Insert** → at the bottom of the sidebar open **Settings** (*Konfiguracja*).
2. Turn on **Tester mode** (*Tryb testera*) and click **Open Tester hub** (*Otwórz Centrum testera*).
3. Enter **your nickname** in the top-right corner of the hub — every report is signed with it.

In tester mode every option shows a **dot** — the author's status — and a **flag** that opens a report for that option.

```trainer
{"path": "Any page, tester mode", "title": "Status dots", "rows": [
 {"t": "toggle", "label": "● green — OK", "on": true},
 {"t": "toggle", "label": "● yellow — BUG", "on": false},
 {"t": "toggle", "label": "● red — CRASH", "on": false},
 {"t": "toggle", "label": "● purple — TEST: new, please check", "on": true}
]}
```

## 2. Test

**Checklist** (*Checklista*): for each option click **Works / Partly / Does not work**.

- In alpha builds test mainly **TEST (new)** and **BUG** — use the dot filter.
- Nothing is overwritten: every rating goes to the history with the build number.
- After *Partly* / *Does not work* a form opens — describe what went wrong.

**+ New report** (*+ Nowe zgłoszenie*) — a bug, crash, suggestion or question:

- **What is it about** — the option (searchable list) or "General". Known problems of that option are shown below.
- **Steps** — what you did, in order. The most important field.
- **Attach game state** — world, position, enabled options and recent events, added automatically.
- **Attachments** — screenshot with **F11** / **Shift+F11**, then **Ctrl+V** in the form.

**Crash?** Start the game again — the trainer prepares a draft report by itself.

## 3. Send

**Send** (*Wyślij*) → **Export unsent** creates one file in `G1CrazyTrainer_raporty` (.txt or .zip). **Send it to the author** (Discord / e-mail).

> **Note:** A report contains your nickname, Windows version, CPU/GPU name, RAM, resolution, the list of DLLs in the game folder, trainer settings and its log. Nothing else — it is plain text, read it before sending.
