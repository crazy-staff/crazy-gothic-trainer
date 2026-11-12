// Marvin module: debug flags, console with macros, script calls.
#include "mods.h"
#include <sstream>

using namespace g1;
using namespace gx;

void Act_Console(const std::string& cmd) {
    Act_RunMacro({ "", cmd, 1 });
}
void Act_RunMacro(const Macro& m) {
    Macro copy = m;
    T.Queue("macro:" + (m.name.empty() ? std::string("konsola") : m.name), [copy] {
        std::string log;
        int n = 0;
        for (int r = 0; r < std::max(1, copy.repeat); r++) {
            std::stringstream ss(copy.cmds); std::string line;
            while (std::getline(ss, line)) {
                while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
                if (line.empty()) continue;
                std::string out = ConsoleExec(line);
                n++;
                if (n <= 12) log += "> " + line + (out.empty() ? "" : "\n  " + U(out)) + "\n";
            }
        }
        if (n > 12) log += "... (" + std::to_string(n) + " komend razem)\n";
        std::lock_guard<std::mutex> l(T.mtx);
        T.snap.marvinOut = log.empty() ? "(brak komend)" : log;
    });
}
void Act_RunScript(const std::string& fn) {
    T.Queue("script:" + fn, [=] {
        bool ok = false;
        int r = CallScript(fn.c_str(), {}, &ok);
        std::lock_guard<std::mutex> l(T.mtx);
        T.snap.marvinOut = ok ? ("Wywołano " + fn + "() -> " + std::to_string(r)) : ("Nie znaleziono funkcji " + fn);
    });
}

void Marvin_Hooks() {}
void Marvin_World() {}
// build 9 (#23): only write the game's flags when our switch changes. Writing them every frame turned off the
// Marvin mode enabled the original way (typing MARVIN in the status screen) whenever our switch was off.
void Marvin_Tick(TickCtx& c) {
    static int last[3] = { -1, -1, -1 };
    const char* ids[3] = { "marvin", "waynet", "frameinfo" };
    const int offs[3] = { OG_TESTMODE, OG_SHOWWAYNET, OG_FRAMEINFO };
    for (int i = 0; i < 3; i++) {
        int game = fld<int>(c.game, offs[i]) ? 1 : 0;
        int on = On(ids[i]) ? 1 : 0;
        if (last[i] != -1 && game != last[i]) {   // build 10: switched in the game itself (MARVIN in the status screen) -> show it
            if (Row* r = T.Get(ids[i])) r->on = game != 0;
            last[i] = game; continue;
        }
        if (on != last[i]) { if (last[i] != -1 || on) fld<int>(c.game, offs[i]) = on; last[i] = on; }
    }
}
void Marvin_Register() {
    {   // build 12: on-screen info window (Narzędzia › Nakładka) - drawn by the UI, these rows only choose what it shows
        const char* N = "Nakładka";
        CardTitle("Okno informacji");
        T.Add(MkRow("hud", "Pokaż okno informacji", "zostaje na ekranie w trakcie gry; przeciągnij je myszką w menu", "Narzędzia", N, K_TOGGLE)); St(ST_NEW, "nowe w build 12"); Icon(ICN_LIST);
        T.Add(MkRow("hudalpha", "Przezroczystość tła", "%", "Narzędzia", N, K_VALUE, 60, 0, 100)); St(ST_NEW, "nowe w build 12");
        T.Add(MkRow("hudsize", "Wielkość tekstu", "%", "Narzędzia", N, K_VALUE, 100, 70, 200)); St(ST_NEW, "nowe w build 12");
        CardTitle("Co pokazywać");
        struct { const char* id; const char* name; const char* note; bool on; } items[] = {
            { "hudfps", "FPS", "klatki na sekundę i czas klatki", true },
            { "hudpos", "Koordynaty", "X / Y / Z bohatera", true },
            { "hudspeed", "Prędkość ruchu", "m/s, liczona z ruchu bohatera", true },
            { "hudtime", "Czas gry", "dzień i godzina", true },
            { "hudworld", "Świat i miejsce", "nazwa świata i opis najbliższego miejsca", false },
            { "hudstats", "HP i mana", "", false },
            { "hudtarget", "Cel", "imię i HP postaci, w którą celujesz", false },
            { "hudmods", "Liczba włączonych opcji", "", false },
        };
        for (auto& it : items) { T.Add(MkRow(it.id, it.name, it.note, "Narzędzia", N, K_TOGGLE)); T.rows.back().on = it.on; St(ST_NEW, "nowe w build 12"); }
    }
    CardTitle("Tryb debug");
    T.Add(MkRow("marvin", "Tryb Marvina", "konsola F2, klawisze debug - bez wpisywania MARVIN", "Narzędzia", "Konsola", K_TOGGLE)); St(ST_NEW, "build 9: nie wyłącza już Marvina włączonego w grze (#23)"); Icon(ICN_TERMINAL);
    T.Add(MkRow("waynet", "Pokaż waynet", "ścieżki NPC", "Narzędzia", "Konsola", K_TOGGLE)); St(ST_OK, "działa");
    T.Add(MkRow("frameinfo", "Info o klatkach", "", "Narzędzia", "Konsola", K_TOGGLE)); St(ST_OK, "działa");
}
