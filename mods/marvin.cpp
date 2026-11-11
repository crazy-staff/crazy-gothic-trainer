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
void Marvin_Tick(TickCtx& c) {
    fld<int>(c.game, OG_TESTMODE) = On("marvin") ? 1 : 0;
    fld<int>(c.game, OG_SHOWWAYNET) = On("waynet") ? 1 : 0;
    fld<int>(c.game, OG_FRAMEINFO) = On("frameinfo") ? 1 : 0;
}
void Marvin_Register() {
    CardTitle("Tryb debug");
    T.Add(MkRow("marvin", "Tryb Marvina", "konsola F2, klawisze debug - bez wpisywania MARVIN", "Narzędzia", "Konsola", K_TOGGLE)); St(ST_OK, "działa"); Icon(ICN_TERMINAL);
    T.Add(MkRow("waynet", "Pokaż waynet", "ścieżki NPC", "Narzędzia", "Konsola", K_TOGGLE)); St(ST_OK, "działa");
    T.Add(MkRow("frameinfo", "Info o klatkach", "", "Narzędzia", "Konsola", K_TOGGLE)); St(ST_OK, "działa");
}
