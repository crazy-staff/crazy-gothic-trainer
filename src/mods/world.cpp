// World module: pause-on-menu, world tempo, time of day, chapters. Also owns every model
// time scale (hero speed, sprint, world tempo for characters, Quicksilver) so they never fight.
#include "mods.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>

using namespace g1;
using namespace gx;

namespace {
bool g_timerTouched = false; float g_frozenTime = -1;
std::set<void*> g_scaled;

std::unordered_map<void*, float> g_yaw;   // last yaw of slowed npcs (turning is not animation-driven)
void TimeScales(void* hero, bool paused) {
    bool spd = On("speed"); int mode = (int)Val("speedmode");
    float npcScale = 1.f;
    if (spd && mode == 0) npcScale *= std::max(0.05f, Val("speed"));
    if (On("quicksilver")) npcScale *= std::max(0.02f, Val("qsfactor"));
    float heroScale = On("pspeed") ? Val("pspeed") : 1.f;
    if (On("noclip") && (int)Val("ncmode") == 2) heroScale *= std::max(1.f, Val("ncspeed") / 3.6f);   // no-clip "run" mode
    if (On("quicksilver")) heroScale *= std::max(1.f, Val("qshero"));
    if (paused) return;
    void* main = MainHero();
    ForEachNpc([&](void* n) {
        float want = (n == hero || n == main) ? (n == hero ? heroScale : 1.f) : (IsAlly(n) && On("qsallies") ? 1.f : npcScale);
        bool touched = g_scaled.count(n) != 0;
        if (want == 1.f && !touched) return;
        void* m = VobModel(n);
        if (!m) return;
        fld<float>(m, MODEL_TIMESCALE) = want;
        // turning speed ignores the animation time scale: limit the yaw change to the same factor
        float* tm = &fld<float>(n, VOB_TRAFO);
        float yaw = std::atan2(tm[2], tm[10]);
        if (want < 0.99f && !NpcDead(n)) {
            auto it = g_yaw.find(n);
            if (it != g_yaw.end()) {
                float d = yaw - it->second;
                while (d > 3.14159f) d -= 6.28318f;
                while (d < -3.14159f) d += 6.28318f;
                if (std::fabs(d) > 1e-4f) {
                    float ny = it->second + d * want, c = std::cos(ny), s = std::sin(ny);
                    tm[0] = c; tm[4] = 0; tm[8] = -s; tm[1] = 0; tm[5] = 1; tm[9] = 0; tm[2] = s; tm[6] = 0; tm[10] = c;
                    yaw = ny;
                }
            }
            g_yaw[n] = yaw;
        } else g_yaw.erase(n);
        if (want == 1.f) g_scaled.erase(n); else if (!touched) g_scaled.insert(n);
    });
}
}

void World_Hooks() {}
void World_World() { g_frozenTime = -1; g_scaled.clear(); g_yaw.clear(); }
void World_Tick(TickCtx& c) {
    bool spd = On("speed");
    bool clockMode = spd && (int)Val("speedmode") == 1;
    if (c.paused) { at<float>(ZTIMER) = 0.0001f; g_timerTouched = true; }
    else if (clockMode) { at<float>(ZTIMER) = std::max(0.05f, Val("speed")); g_timerTouched = true; }
    else if (g_timerTouched) { at<float>(ZTIMER) = 1.f; g_timerTouched = false; }
    TimeScales(c.hero, c.paused);

    void* wt = fld<void*>(c.game, OG_WLDTIMER);
    if (wt) {
        if (On("freezetime")) { if (g_frozenTime < 0) g_frozenTime = fld<float>(wt, 0); fld<float>(wt, 0) = g_frozenTime; }
        else g_frozenTime = -1;
        float t = fld<float>(wt, 0);
        c.s.day = fld<int>(wt, 4);
        c.s.hour = (int)(t / TICKS_PER_HOUR) % 24;
        c.s.minute = (int)(std::fmod(t, TICKS_PER_HOUR) / TICKS_PER_HOUR * 60.f);
    }
    c.s.speed = spd; c.s.speedVal = Val("speed");
    c.s.quicksilver = On("quicksilver");
    c.s.world = WorldName();
}

void World_Register() {
    const char* C = "Świat";
    const char* S = "Czas i fabuła";
    CardTitle("Tempo");
    T.Add(MkRow("speed", "Tempo świata", "0.1 = slow-mo, 3 = turbo", C, S, K_SLIDER, 0.3f, 0.1f, 5, nullptr, true));
    St(ST_OK, "build 4-5: tryb 'postacie' działa"); Icon(ICN_CLOCK); Excl({ "quicksilver" });
    T.Add(MkRow("speedmode", "Co zwalnia", "", C, S, K_VALUE, 0, 0, 1));
    Opts({ { 0, "Postacie wokół (ty i kamera normalnie)" }, { 1, "Całą grę (też ciebie i obrót kamery)" } });
    St(ST_NEW, "build 6: 'bohater normalnie' w trybie zegara usunięte - silnik zwalnia też obrót kamery, nie da się tego obejść");
    CardTitle("Pora dnia");
    T.Add(MkRow("hour", "Ustaw godzinę", "0-23", C, S, K_ACTION, 12, 0, 23, [](Row& r) {
        CallExt("Wld_SetTime", { Arg::Int((int)r.val), Arg::Int(0) }); g_frozenTime = -1;
        T.Toast("Godzina " + std::to_string((int)r.val) + ":00"); }));
    St(ST_OK, "działa"); Icon(ICN_CLOCK);
    T.Add(MkRow("freezetime", "Zatrzymaj czas dnia", "", C, S, K_TOGGLE)); St(ST_OK, "działa");
    CardTitle("Fabuła");
    T.Add(MkRow("chapter", "Skok do rozdziału", "B_Kapitelwechsel - może popsuć questy!", C, S, K_CHOICE, 2, 1, 6, [](Row& r) {
        bool ok; CallScript("B_Kapitelwechsel", { Arg::Int((int)r.val) }, &ok);
        T.Toast(ok ? "Rozdział " + std::to_string((int)r.val) : std::string("Brak B_Kapitelwechsel")); }));
    Opts({ { 1, "Rozdział 1" }, { 2, "Rozdział 2" }, { 3, "Rozdział 3" }, { 4, "Rozdział 4" }, { 5, "Rozdział 5" }, { 6, "Rozdział 6" } });
    St(ST_OK, "działa");

    CardTitle("Menu");
    T.Add(MkRow("pausemenu", "Pauza przy otwartym menu", "świat stoi, gdy menu trainera jest otwarte", "Konfiguracja", "Konfiguracja", K_TOGGLE));
    T.rows.back().on = true; St(ST_OK, "działa"); Icon(ICN_CLOCK);
    CardTitle("Okno aktywnych modów");
    T.Add(MkRow("statuswin", "Kiedy pokazywać", "małe okno z listą wszystkiego, co jest włączone (i z jakimi ustawieniami)", "Konfiguracja", "Konfiguracja", K_VALUE, 0, 0, 2));
    Opts({ { 0, "Wyłączone" }, { 1, "Tylko w menu" }, { 2, "W menu i w grze" } }); St(ST_NEW, "build 6: trzy tryby");
    T.Add(MkRow("statusalpha", "Przezroczystość tła", "%", "Konfiguracja", "Konfiguracja", K_VALUE, 70, 0, 100)); St(ST_NEW, "nowe w build 5");
}
