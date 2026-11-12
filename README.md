# Gothic 1 Crazy Trainer

🇬🇧 English · 🇵🇱 [Polski](README.pl.md) · 🇩🇪 [Deutsch](README.de.md) · 🇷🇺 [Русский](README.ru.md)

> 🗺️ **[Roadmap: bugs, plans, ideas — vote 👍](ROADMAP.md)** · site: https://dawidseipold.github.io/gothic1-crazy-trainer/
> 💡 **[Ideas & discussion](https://github.com/dawidseipold/gothic1-crazy-trainer/discussions/categories/ideas)** — every idea is its own thread, vote with the up arrow.

A trainer / mod menu for the Steam version of **Gothic 1** (`GothicMod.exe` 1.08k_mod), running with **Union** and the **GD3D11** renderer.
Stats, inventory, item and NPC spawner, teleports, ESP, radar, party management, arena, "fun" modes (spell rain, meteors, zombies, polymorph…) and much more — all from one in-game menu.

> Single-player only. The trainer is in testing: back up your saves before you start.
> **The in-game menu is in Polish for now.** An English interface (with Polish as a translation) is planned — see the [idea thread](https://github.com/dawidseipold/gothic1-crazy-trainer/discussions/categories/ideas).

## Download

The latest version is always in **[Releases](https://github.com/dawidseipold/gothic1-crazy-trainer/releases/latest)** — download `G1CrazyTrainer_buildN.zip`.

## Requirements

| What | Notes |
|---|---|
| Gothic 1 from Steam | with `GothicMod.exe` 1.08k_mod (Player Kit) |
| [Union](https://steamcommunity.com/sharedfiles/filedetails/?id=2787491081) | loads DLLs from `system\Autorun` — the trainer will not start without it |
| [GD3D11](https://github.com/Kirides/GD3D11) | the menu is drawn through the DirectX 11 renderer — it will not show on the original renderer |

The easiest way is the **Steam Workshop for Gothic** (subscribe to Union and GD3D11 in the Workshop and start the game through the Workshop launcher).

## Installation

1. Unpack the downloaded `.zip`.
2. Copy **all files** from the package into `Gothic\system\Autorun`.
   Find the game folder in Steam: right-click Gothic → *Manage* → *Browse local files*.
   If there is no `Autorun` folder, create it in `Gothic\system`.
   ```
   Gothic\
   └─ system\
      └─ Autorun\
         ├─ G1CrazyTrainer.dll
         └─ G1CrazyTrainer_znane.txt
   ```
3. Start the game as usual (with Union + GD3D11) and load a save or start a new game.
4. Press **Insert** — the menu opens.

### Updating
Replace `G1CrazyTrainer.dll` and `G1CrazyTrainer_znane.txt` with the new ones. Settings, hotkeys and report history are kept.

### Uninstalling
Delete `G1CrazyTrainer.dll` and every file and folder starting with `G1CrazyTrainer` from `system\Autorun`.

## Controls

| Key | Action |
|---|---|
| **Insert** | open / close the menu (the game is paused meanwhile) |
| **Esc** | close the menu, leave no-clip, stop portrait generation |
| **F11 / Shift+F11** | screenshot / screenshot of an area (for reports) |
| custom hotkeys | any option: click the hotkey field next to it and press a key / combination |

## Bugs and ideas

Want to help test? Read **[TESTING.md](TESTING.md)** — the in-game Tester hub collects everything needed (reports, event log, errors, screenshots) into one report file.

[Report a bug](https://github.com/dawidseipold/gothic1-crazy-trainer/issues/new?template=blad.yml) ·
[Report a crash](https://github.com/dawidseipold/gothic1-crazy-trainer/issues/new?template=crash.yml) ·
[Suggest a feature (Discussions)](https://github.com/dawidseipold/gothic1-crazy-trainer/discussions/categories/ideas)

**Please read [CONTRIBUTING.md](CONTRIBUTING.md) first** — where to post what, how ideas become features, and the rule that everything on GitHub is in English.

Files the trainer creates in `system\Autorun`:

| File | What it is |
|---|---|
| `G1CrazyTrainer.ini` | settings and hotkeys |
| `G1CrazyTrainer.log` | log (useful for crashes) |
| `G1CrazyTrainer_zgloszenia.txt` | history of your reports and ratings |
| `G1CrazyTrainer_raporty\` | exported reports for the author |
| `G1CrazyTrainer_obrazki\` | character portraits and item icons (safe to delete, they regenerate) |

## For developers

Cross-compiled from Linux (MinGW, 32-bit, static):
- `i686-w64-mingw32-g++-posix`
- Dear ImGui 1.92 in `/root/ext/imgui`, MinHook in `/root/ext/minhook`

```
./build.sh   # -> out/G1CrazyTrainer.dll
```

Layout:
- `src/core` – engine (ZenGin) addresses and calls, item/NPC catalogue, shared helpers
- `src/mods` – feature modules (each has its own menu rows, logic and hooks)
- `src/ui.cpp` – menu (ImGui), `src/portrait.cpp` – character portraits
- `src/tester.cpp` – Tester hub: report history, event log, report export, GitHub links
- `src/trainer.cpp` – action queue, crash protection, settings
- `tools/reports.py` – merges tester reports into one overview and files issues (`--github`)
- `tools/known_issues.py` – generates `G1CrazyTrainer_znane.txt` (known problems shown in game) from GitHub issues
- `tools/discussions.py` + `docs/discussions/*.json` – ideas in Discussions (one thread per idea, `category:` labels)
- `tools/ai_triage.py` – AI triage and moderation of new issues and discussions (GitHub Models)
- `tools/idea_flow.py` – approved idea → linked issue → idea done
- `tools/roadmap.py` – builds `ROADMAP.md` and the roadmap site from issues and Discussions
- `.github/workflows/release.yml` – adding `docs/release/bN.md` on main builds the DLL and publishes release `bN`
- `.github/ISSUE_TEMPLATE` – web report forms, `.github/workflows/triage.yml` – automatic `option: <id>`, `build: N` labels and duplicate hints

Commits, issues and discussions are in English — see [CONTRIBUTING.md](CONTRIBUTING.md).
