# Gothic 1 Crazy Trainer

Trainer / mod menu do Gothic 1 (Steam, `GothicMod.exe` 1.08k_mod) uruchamianego przez
Gothic Steam Workshop Launcher z modami **Union** i **GD3D11**. Tylko do gry jednoosobowej.

## Instalacja
Skopiuj `G1CrazyTrainer.dll` do `Gothic\system\Autorun`. W grze: **Insert** otwiera menu, **Esc** zamyka.

## Budowanie
Kompilacja krzyżowa z Linuksa (MinGW, 32-bit):
- `i686-w64-mingw32-g++-posix`
- Dear ImGui 1.92 w `/root/ext/imgui`, MinHook w `/root/ext/minhook`

```
./build.sh   # -> out/G1CrazyTrainer.dll
```

## Struktura
- `src/core` – adresy i wywołania silnika (ZenGin), katalog przedmiotów/NPC, wspólne helpery
- `src/mods` – moduły funkcji (każdy ma własne wiersze menu, logikę i hooki)
- `src/ui.cpp` – menu (ImGui), `src/portrait.cpp` – portrety postaci
- `src/trainer.cpp` – kolejka akcji, ochrona przed crashami, zapis ustawień
