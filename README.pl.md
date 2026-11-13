# Gothic Crazy Trainer

🇬🇧 [English](README.md) · 🇵🇱 Polski · 🇩🇪 [Deutsch](README.de.md) · 🇷🇺 [Русский](README.ru.md)

> 🗺️ **[Mapa drogowa: błędy, plany, pomysły — głosuj 👍](ROADMAP.md)** · strona: https://dawidseipold.github.io/gothic-crazy-trainer/
> 💡 **[Pomysły i dyskusje](https://github.com/dawidseipold/gothic-crazy-trainer/discussions/categories/ideas)** — każdy pomysł to osobny wątek, głosuj strzałką w górę.

Trainer / mod menu do **Gothic 1** w wersji Steam (`GothicMod.exe` 1.08k_mod), uruchamianego z **Union** i rendererem **GD3D11**.
Statystyki, ekwipunek, spawner przedmiotów i postaci, teleport, ESP, radar, drużyna, arena, tryby „fun” (deszcz zaklęć, meteoryty, zombie, polimorfia…) i sporo więcej — wszystko z jednego menu w grze.

> Tylko do gry jednoosobowej. Trainer jest w fazie testów: zanim zaczniesz, zrób kopię zapisów gry.

## Obsługiwane gry

Trainer powstaje z myślą o kilku grach. Każda gra ma w kodzie własny moduł, bo silniki różnią się adresami i zachowaniem; gdy moduł zostanie przetestowany na innej grze lub rendererze, wspólne części trafiają do kodu wspólnego.

| Gra | Renderer | Status |
|---|---|---|
| Gothic 1 (Steam, `GothicMod.exe` 1.08k_mod) | GD3D11 | ✅ w testach |
| Gothic 1 | oryginalny renderer | ⏳ planowane |
| Gothic 2: Noc Kruka | GD3D11 | ⏳ planowane |
| Mody fabularne (np. Kroniki Myrtany) | GD3D11 | 💭 później |

## Pobieranie

Najnowsza wersja jest zawsze w **[Releases](https://github.com/dawidseipold/gothic-crazy-trainer/releases/latest)** — pobierz `G1CrazyTrainer_buildN.zip`.

## Wymagania

| Co | Uwagi |
|---|---|
| Gothic 1 ze Steama | z `GothicMod.exe` 1.08k_mod (Player Kit) |
| [Union](https://steamcommunity.com/sharedfiles/filedetails/?id=2787491081) | ładuje pliki DLL z `system\Autorun` — bez niego trainer się nie uruchomi |
| [GD3D11](https://github.com/Kirides/GD3D11) | menu rysuje się przez renderer DirectX 11 — na zwykłym rendererze się nie pokaże |

Najprościej przez **Steam Workshop dla Gothic** (subskrybujesz Union i GD3D11 i uruchamiasz grę przez launcher Warsztatu).

## Instalacja

1. Rozpakuj pobrany `.zip`.
2. Skopiuj **wszystkie pliki** do `Gothic\system\Autorun` (Steam: prawy przycisk na Gothic → *Zarządzaj* → *Przeglądaj pliki lokalne*; jeśli nie ma folderu `Autorun`, utwórz go).
3. Uruchom grę (z Union + GD3D11) i wczytaj zapis albo zacznij nową grę.
4. Naciśnij **Insert** — otworzy się menu.

**Aktualizacja:** podmień `G1CrazyTrainer.dll` i `G1CrazyTrainer_znane.txt`. Ustawienia zostają.
**Odinstalowanie:** usuń z `system\Autorun` wszystkie pliki i foldery zaczynające się od `G1CrazyTrainer`.

## Sterowanie

| Klawisz | Działanie |
|---|---|
| **Insert** | otwiera / zamyka menu (gra jest wtedy zatrzymana) |
| **Esc** | zamyka menu, wychodzi z no-clipa |
| **F11 / Shift+F11** | zrzut ekranu / zrzut obszaru (do zgłoszeń) |
| własne skróty | kliknij pole skrótu przy opcji i naciśnij klawisz |

## Błędy i pomysły

Wszystko na GitHubie piszemy **po angielsku** — zasady i to, gdzie co zgłaszać, są w **[CONTRIBUTING.md](CONTRIBUTING.md)**.
Testowanie z Centrum testera w grze: **[TESTING.md](TESTING.md)**.

Dla programistów: zobacz [README po angielsku](README.md#for-developers).
