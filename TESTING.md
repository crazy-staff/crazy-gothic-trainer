# How to test and report problems

Thanks for helping! Everything you need is in the game — in the **Tester hub**.
The in-game menu is in Polish for now, so the Polish menu names are given in *italics*.

## 1. Turn on tester mode

1. In the game press **Insert** → at the bottom of the sidebar open **Settings** (*Konfiguracja*).
2. Turn on **Tester mode** (*Tryb testera*) and click **Open Tester hub** (*Otwórz Centrum testera*) — or **Tests** (*Testy*) in the sidebar.
3. Enter **your nickname** in the top-right corner of the hub — every report is signed with it.

In tester mode every option in the menu shows:
- a **dot** — the author's status for the option (green OK, yellow BUG, red CRASH, purple TEST = new, to be checked). Hover it to see the history.
- a **flag** — opens a report form for that option. An orange flag = there are known problems already.

## 2. Test

**Checklist** tab (*Checklista*): for each option click **Works / Partly / Does not work** (*Działa / Częściowo / Nie działa*).
- The **author's dot** filter shows options by status — in alpha builds test mainly **TEST (new)** and **BUG**, not everything again.
- Nothing is overwritten: every rating goes to the history (with the build number), so later versions show what changed.
- After "Partly" / "Does not work" a form opens — describe what went wrong (optional, but it helps a lot).
- Click the entry counter next to an option to see **everything known about it**: author's notes, known problems from GitHub and all earlier reports.

**+ New report** tab (*+ Nowe zgłoszenie*) — a form for a bug, crash, suggestion or question:
- **What is it about** — pick the option from the list (there is a search box) or "General". Known problems of that option are shown below: check whether yours is already there.
- **Steps** — what you did, in order. This is the most important field: good steps make a bug reproducible and fixable.
- **Attach game state** (leave it on) — the trainer adds the world, position, enabled options and recent events.
- **Attachments**: take a screenshot (**F11**, or **Shift+F11** for an area) and press **Ctrl+V** in the form, or "Add file from disk". Screenshots go into the report (.zip).
- You can also report a **whole category, subcategory, card or aspect** (UI, UX, performance…): the flag next to a card / subcategory header or the choice in "What is it about".

The **Log** tab (*Dziennik*) shows what the trainer recorded in this session (options turned on, actions, caught errors). It is attached to reports automatically.

**Crash?** Start the game again — the trainer detects what the game was doing when it crashed and prepares a draft report in the **Reports** tab (*Zgłoszenia*). Just add what you were doing.

## 3. Send

**Send** tab (*Wyślij*) → **Export unsent** (*Eksportuj niewysłane*). This creates one file in `Gothic\system\Autorun\G1CrazyTrainer_raporty\` (.txt, or .zip with attachments) with everything: reports, ratings, log, errors from the log file, PC info. **Send this file to the author** (Discord / e-mail) — GitHub issues are created from the reports.

There is also "Copy for Discord" (short text) for quick notes, but the report file is much better.

Sent reports are marked and are not sent twice. **You can edit your reports from the current build** (pencil in the Reports tab) — e.g. when the game crashed while you were writing. After editing, a report goes back to unsent.

## A good report

- One report = one problem.
- The title says what happens: "Spell rain closes the game with 10 targets", not "does not work".
- Steps one by one, starting from loading the game.
- Text fields can be resized — drag the bottom edge.
- Write in English.

## Privacy

A report contains: nickname, Windows version, CPU and GPU name, amount of RAM, resolution, the list of DLL files in the game folder, trainer settings and its log. Nothing else. The file is plain text — you can read it before sending.
