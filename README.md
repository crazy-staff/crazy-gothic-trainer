# Gothic 1 Crazy Trainer

Trainer / mod menu do **Gothic 1** w wersji Steam (`GothicMod.exe` 1.08k_mod), uruchamianego z **Union** i rendererem **GD3D11**.
Statystyki, ekwipunek, spawner przedmiotów i postaci, teleport, ESP, arena, tryby „fun” (deszcz zaklęć, meteoryty, zombie, polimorfia…) i sporo więcej — wszystko z jednego menu w grze.

> Tylko do gry jednoosobowej. Trainer jest w fazie testów: zanim zaczniesz, zrób kopię zapisów gry.

## Pobieranie

Najnowsza wersja jest zawsze w **[Releases](https://github.com/dawidseipold/gothic1-crazy-trainer/releases/latest)** — pobierz plik `G1CrazyTrainer_buildN.zip`.

## Wymagania

| Co | Uwagi |
|---|---|
| Gothic 1 ze Steama | z `GothicMod.exe` 1.08k_mod (Player Kit) |
| [Union](https://steamcommunity.com/sharedfiles/filedetails/?id=2787491081) | ładuje pliki DLL z folderu `system\Autorun` — bez niego trainer się nie uruchomi |
| [GD3D11](https://github.com/Kirides/GD3D11) | menu rysuje się przez renderer DirectX 11 — na zwykłym rendererze menu się nie pokaże |

Najprościej mieć to wszystko przez **Steam Workshop dla Gothic** (Union i GD3D11 subskrybujesz w Warsztacie i uruchamiasz grę przez launcher Warsztatu).

## Instalacja

1. Rozpakuj pobrany `.zip`.
2. Skopiuj **wszystkie pliki** z paczki do folderu `Gothic\system\Autorun`.
   Folder gry znajdziesz w Steamie: prawy przycisk na Gothic → *Zarządzaj* → *Przeglądaj pliki lokalne*.
   Jeśli folderu `Autorun` nie ma, utwórz go w `Gothic\system`.
   ```
   Gothic\
   └─ system\
      └─ Autorun\
         ├─ G1CrazyTrainer.dll
         └─ G1CrazyTrainer_znane.txt
   ```
3. Uruchom grę jak zwykle (z Union + GD3D11) i wczytaj zapis albo zacznij nową grę.
4. Naciśnij **Insert** — otworzy się menu.

### Aktualizacja
Podmień `G1CrazyTrainer.dll` i `G1CrazyTrainer_znane.txt` na nowe. Ustawienia, skróty i historia zgłoszeń zostają.

### Odinstalowanie
Usuń z `system\Autorun` plik `G1CrazyTrainer.dll` oraz wszystkie pliki i foldery zaczynające się od `G1CrazyTrainer`.

## Sterowanie

| Klawisz | Działanie |
|---|---|
| **Insert** | otwiera / zamyka menu (gra jest wtedy zatrzymana) |
| **Esc** | zamyka menu, wychodzi z no-clipa, przerywa generowanie portretów |
| własne skróty | dowolna opcja: kliknij pole skrótu przy opcji i naciśnij klawisz / kombinację |

## Problemy i pomysły

Chcesz pomóc w testach? Przeczytaj **[TESTOWANIE.md](TESTOWANIE.md)** — w grze jest Centrum testera, które samo zbiera wszystko, czego potrzeba (zgłoszenia, dziennik, błędy, zrzuty), i pakuje to w jeden plik raportu do wysłania autorowi.

[Zgłoś błąd](https://github.com/dawidseipold/gothic1-crazy-trainer/issues/new?template=blad.yml) ·
[Zgłoś crash](https://github.com/dawidseipold/gothic1-crazy-trainer/issues/new?template=crash.yml) ·
[Zaproponuj funkcję](https://github.com/dawidseipold/gothic1-crazy-trainer/issues/new?template=propozycja.yml)

Pliki, które trainer tworzy w `system\Autorun`:

| Plik | Co to |
|---|---|
| `G1CrazyTrainer.ini` | ustawienia i skróty |
| `G1CrazyTrainer.log` | log (przydaje się przy crashach) |
| `G1CrazyTrainer_zgloszenia.txt` | historia twoich zgłoszeń i ocen |
| `G1CrazyTrainer_raporty\` | wyeksportowane raporty dla autora |
| `G1CrazyTrainer_obrazki\` | portrety postaci i ikony przedmiotów (można usunąć, wygenerują się ponownie) |

## Dla programistów

Kompilacja krzyżowa z Linuksa (MinGW, 32-bit, statycznie):
- `i686-w64-mingw32-g++-posix`
- Dear ImGui 1.92 w `/root/ext/imgui`, MinHook w `/root/ext/minhook`

```
./build.sh   # -> out/G1CrazyTrainer.dll
```

Struktura:
- `src/core` – adresy i wywołania silnika (ZenGin), katalog przedmiotów/NPC, wspólne helpery
- `src/mods` – moduły funkcji (każdy ma własne wiersze menu, logikę i hooki)
- `src/ui.cpp` – menu (ImGui), `src/portrait.cpp` – portrety postaci
- `src/tester.cpp` – Centrum testera: historia zgłoszeń, dziennik zdarzeń, eksport raportów, linki do GitHuba
- `src/trainer.cpp` – kolejka akcji, ochrona przed crashami, zapis ustawień
- `tools/reports.py` – łączy raporty testerów w jedno podsumowanie i zakłada z nich issues (`--github`)
- `tools/known_issues.py` – generuje `G1CrazyTrainer_znane.txt` (znane problemy pokazywane w grze) z issues na GitHubie
- `.github/workflows/release.yml` – dodanie `docs/release/bN.md` na main buduje DLL i publikuje release `bN`
- `.github/ISSUE_TEMPLATE` – formularze zgłoszeń przez stronę, `.github/workflows/triage.yml` – automatyczne etykiety `opcja:<id>`, `build-N` i wskazywanie duplikatów
