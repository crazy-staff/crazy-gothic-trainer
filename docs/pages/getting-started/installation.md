---
title: Installation
description: Requirements, install, update and uninstall.
---

## Requirements

| What | Notes |
|---|---|
| Gothic 1 from Steam | with `GothicMod.exe` 1.08k_mod (Player Kit) |
| [Union](https://steamcommunity.com/sharedfiles/filedetails/?id=2787491081) | loads DLLs from `system\Autorun` — the trainer will not start without it |
| [GD3D11](https://github.com/Kirides/GD3D11) | the menu is drawn through the DirectX 11 renderer — it does not show on the original renderer |

> **Note:** The easiest way is the **Steam Workshop for Gothic**: subscribe to Union and GD3D11 and start the game through the Workshop launcher.

## Install

### 1. Download

Get `G1CrazyTrainer_buildN.zip` from the [latest release](https://github.com/dawidseipold/gothic1-crazy-trainer/releases/latest) and unpack it.

### 2. Copy the files

Copy **all files** from the package into `Gothic\system\Autorun`. In Steam: right-click Gothic → *Manage* → *Browse local files*. If there is no `Autorun` folder, create it in `Gothic\system`.

```text
Gothic\
└─ system\
   └─ Autorun\
      ├─ G1CrazyTrainer.dll
      └─ G1CrazyTrainer_znane.txt
```

### 3. Start the game

Start Gothic as usual (with Union + GD3D11), load a save or start a new game and press **Insert**.

## Update

Replace `G1CrazyTrainer.dll` and `G1CrazyTrainer_znane.txt` with the new ones. Settings, hotkeys and report history are kept.

## Uninstall

Delete `G1CrazyTrainer.dll` and every file and folder starting with `G1CrazyTrainer` from `system\Autorun`.
