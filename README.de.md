# Gothic 1 Crazy Trainer

🇬🇧 [English](README.md) · 🇵🇱 [Polski](README.pl.md) · 🇩🇪 Deutsch · 🇷🇺 [Русский](README.ru.md)

> 🗺️ **[Roadmap: Fehler, Pläne, Ideen — abstimmen 👍](ROADMAP.md)** · Seite: https://dawidseipold.github.io/gothic1-crazy-trainer/
> 💡 **[Ideen und Diskussionen](https://github.com/dawidseipold/gothic1-crazy-trainer/discussions/categories/ideas)** — jede Idee ist ein eigener Thread, abstimmen mit dem Pfeil nach oben.

Ein Trainer / Mod-Menü für die Steam-Version von **Gothic 1** (`GothicMod.exe` 1.08k_mod), mit **Union** und dem Renderer **GD3D11**.
Werte, Inventar, Item- und NPC-Spawner, Teleport, ESP, Radar, Gruppe, Arena, „Fun“-Modi (Zauberregen, Meteore, Zombies, Verwandlung…) und vieles mehr — alles in einem Menü im Spiel.

> Nur für den Einzelspielermodus. Der Trainer ist in der Testphase: Sichere vorher deine Spielstände.
> **Das Menü im Spiel ist vorerst auf Polnisch.** Eine englische Oberfläche ist geplant.

## Download

Die neueste Version gibt es immer unter **[Releases](https://github.com/dawidseipold/gothic1-crazy-trainer/releases/latest)** — lade `G1CrazyTrainer_buildN.zip` herunter.

## Voraussetzungen

| Was | Hinweis |
|---|---|
| Gothic 1 von Steam | mit `GothicMod.exe` 1.08k_mod (Player Kit) |
| [Union](https://steamcommunity.com/sharedfiles/filedetails/?id=2787491081) | lädt DLLs aus `system\Autorun` — ohne Union startet der Trainer nicht |
| [GD3D11](https://github.com/Kirides/GD3D11) | das Menü wird über den DirectX-11-Renderer gezeichnet — mit dem Original-Renderer erscheint es nicht |

Am einfachsten über den **Steam Workshop für Gothic** (Union und GD3D11 abonnieren und das Spiel über den Workshop-Launcher starten).

## Installation

1. Die heruntergeladene `.zip` entpacken.
2. **Alle Dateien** nach `Gothic\system\Autorun` kopieren (Steam: Rechtsklick auf Gothic → *Verwalten* → *Lokale Dateien durchsuchen*; fehlt der Ordner `Autorun`, einfach anlegen).
3. Das Spiel (mit Union + GD3D11) starten und einen Spielstand laden oder ein neues Spiel beginnen.
4. **Einfg** (Insert) drücken — das Menü öffnet sich.

**Aktualisieren:** `G1CrazyTrainer.dll` und `G1CrazyTrainer_znane.txt` ersetzen. Einstellungen bleiben erhalten.
**Deinstallieren:** alle Dateien und Ordner, die mit `G1CrazyTrainer` beginnen, aus `system\Autorun` löschen.

## Steuerung

| Taste | Funktion |
|---|---|
| **Einfg / Insert** | Menü öffnen / schließen (das Spiel ist dabei pausiert) |
| **Esc** | Menü schließen, No-Clip beenden |
| **F11 / Umschalt+F11** | Screenshot / Screenshot eines Bereichs (für Fehlerberichte) |
| eigene Tastenkürzel | auf das Kürzel-Feld neben einer Option klicken und eine Taste drücken |

## Fehler und Ideen

Auf GitHub schreiben wir **auf Englisch** — Regeln und wo was hingehört: **[CONTRIBUTING.md](CONTRIBUTING.md)**.
Testen mit dem Tester-Hub im Spiel: **[TESTING.md](TESTING.md)**.

Für Entwickler: siehe die [englische README](README.md#for-developers).
