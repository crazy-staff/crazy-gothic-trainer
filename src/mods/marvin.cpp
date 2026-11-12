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
    {   // build 13: rescue tools (Narzędzia › Diagnostyka)
        const char* D = "Diagnostyka";
        CardTitle("Zrzuty ekranu");
        T.Add(MkRow("winkeyfix", "Zrzuty ekranu (Win+Shift+S)", "po naciśnięciu klawisza Windows gra na 8 s oddaje mysz i klawiaturę systemowi", "Narzędzia", D, K_TOGGLE));
        T.rows.back().on = true; St(ST_NEW, "nowe w build 14 (UI-02)"); Icon(ICN_EYE);
        CardTitle("Ratunek");
        T.Add(MkRow("rescuedialog", "Zamknij zawieszoną rozmowę", "gdy okno dialogu nie reaguje", "Narzędzia", D, K_ACTION, 0, 0, 0, [](Row&) {
            void* h = Player(); if (!h) return;
            CallExt("AI_StopProcessInfos", { Arg::Npc(h) });
            if (void* f = FocusNpc()) { CallExt("Npc_ClearAIQueue", { Arg::Npc(f) }); CallExt("AI_StopProcessInfos", { Arg::Npc(f) }); }
            CallExt("Npc_ClearAIQueue", { Arg::Npc(h) });
            T.Toast("Zamykam rozmowę"); })); NoVal(); St(ST_NEW, "nowe w build 13"); Icon(ICN_TIMES);
        T.Add(MkRow("rescuehero", "Odblokuj bohatera", "wyłącza no-clip/ślizg, przywraca fizykę i kolizje, stawia na ziemi", "Narzędzia", D, K_ACTION, 0, 0, 0, [](Row&) {
            for (const char* id : { "noclip", "glide" }) if (Row* r = T.Get(id)) if (r->on) ApplyRow(*r);
            void* h = Player(); if (!h) return;
            SetSleeping(h, false);
            fld<uint32_t>(h, VOB_FLAGS) |= VOBF_PHYSICS | VOBF_COLLSTAT | VOBF_COLLDYN;
            CallExt("Npc_ClearAIQueue", { Arg::Npc(h) }); CallExt("AI_StandupQuick", { Arg::Npc(h) });
            zVEC3 p = Pos(h), g;
            const float* bb = &fld<float>(h, VOB_BBOX);
            float feet = std::max(40.f, std::min(140.f, p.y - bb[1]));
            if (FindGround(p, 200.f, 20000.f, g)) SafeSetPos(h, { g.x, g.y + feet + 6.f, g.z });
            T.Toast("Bohater odblokowany"); })); NoVal(); St(ST_NEW, "nowe w build 13"); Icon(ICN_REFRESH);
        T.Add(MkRow("rescuetarget", "Odblokuj cel", "przerywa akcje celu, budzi go, stawia na nogi", "Narzędzia", D, K_ACTION, 0, 0, 0, [](Row&) {
            void* t = Target(); if (!t || !NpcReady(t)) { T.Toast("Brak celu"); return; }
            SetSleeping(t, false);
            CallExt("Npc_ClearAIQueue", { Arg::Npc(t) }); CallExt("AI_StandupQuick", { Arg::Npc(t) });
            T.Toast("Odblokowano: " + U(NpcName(t))); })); NoVal(); St(ST_NEW, "nowe w build 13"); Icon(ICN_USER);
    }
    {   // build 12: on-screen info window (Narzędzia › Nakładka) - drawn by the UI, these rows only choose what it shows
        const char* N = "Nakładka";
        CardTitle("Okno informacji");
        T.Add(MkRow("hud", "Pokaż okno informacji", "zostaje na ekranie w trakcie gry; przeciągnij je myszką w menu", "Narzędzia", N, K_TOGGLE)); St(ST_NEW, "nowe w build 12"); Icon(ICN_LIST);
        T.Add(MkRow("hudalpha", "Przezroczystość tła", "%", "Narzędzia", N, K_VALUE, 60, 0, 100)); St(ST_NEW, "nowe w build 12");
        T.Add(MkRow("hudsize", "Wielkość tekstu", "%", "Narzędzia", N, K_VALUE, 100, 70, 200)); St(ST_NEW, "nowe w build 12");
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
        for (auto& it : items) { T.Add(MkRow(it.id, it.name, it.note, "Narzędzia", N, K_TOGGLE)); T.rows.back().on = it.on; Hidden(); St(ST_OK, "build 12: działa"); }
    }
    CardTitle("Tryb debug");
    T.Add(MkRow("marvin", "Tryb Marvina", "konsola F2, klawisze debug - bez wpisywania MARVIN", "Narzędzia", "Konsola", K_TOGGLE)); St(ST_NEW, "build 9: nie wyłącza już Marvina włączonego w grze (#23)"); Icon(ICN_TERMINAL);
    T.Add(MkRow("waynet", "Pokaż waynet", "ścieżki NPC", "Narzędzia", "Konsola", K_TOGGLE)); St(ST_OK, "działa");
    T.Add(MkRow("frameinfo", "Info o klatkach", "", "Narzędzia", "Konsola", K_TOGGLE)); St(ST_OK, "działa");
}
