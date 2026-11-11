// World module: pause-on-menu, world speed, time of day, chapters.
#include "mods.h"
#include <algorithm>
#include <cmath>

using namespace g1;
using namespace gx;

namespace { bool g_timerTouched = false; float g_frozenTime = -1; }

void World_Hooks() {}
void World_World() { g_frozenTime = -1; }
void World_Tick(TickCtx& c) {
    bool spd = On("speed");
    if (c.paused) { at<float>(ZTIMER) = 0.0001f; g_timerTouched = true; }
    else if (spd) { at<float>(ZTIMER) = std::max(0.05f, Val("speed")); g_timerTouched = true; }
    else if (g_timerTouched) { at<float>(ZTIMER) = 1.f; g_timerTouched = false; }

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
    c.s.world = WorldName();
}

void World_Register() {
    const char* C = "Świat";
    T.Add(MkRow("pausemenu", "Pauza przy otwartym menu", "świat stoi, gdy menu trainera jest otwarte", C, "Czas", K_TOGGLE));
    T.rows.back().on = true; St(ST_OK, "działa"); Icon(ICN_CLOCK);
    T.Add(MkRow("speed", "Tempo świata", "0.1 = slow-mo, 3 = turbo", C, "Czas", K_SLIDER, 0.3f, 0.1f, 5, nullptr, true));
    St(ST_OK, "działa; build 3: opcja 'tylko świat'"); Icon(ICN_CLOCK);
    T.Add(MkRow("speedonlyworld", "Tempo nie dotyczy bohatera", "bohater porusza się normalnie mimo slow-mo", C, "Czas", K_TOGGLE));
    St(ST_NEW, "nowe w build 3 (skala czasu animacji bohatera)");
    T.Add(MkRow("hour", "Ustaw godzinę", "0-23", C, "Czas", K_ACTION, 12, 0, 23, [](Row& r) {
        CallExt("Wld_SetTime", { Arg::Int((int)r.val), Arg::Int(0) }); g_frozenTime = -1;
        T.Toast("Godzina " + std::to_string((int)r.val) + ":00"); }));
    St(ST_OK, "działa");
    T.Add(MkRow("freezetime", "Zatrzymaj czas dnia", "", C, "Czas", K_TOGGLE)); St(ST_OK, "działa");
    T.Add(MkRow("chapter", "Skok do rozdziału", "B_Kapitelwechsel - może popsuć questy!", C, "Fabuła", K_CHOICE, 2, 1, 6, [](Row& r) {
        bool ok; CallScript("B_Kapitelwechsel", { Arg::Int((int)r.val) }, &ok);
        T.Toast(ok ? "Rozdział " + std::to_string((int)r.val) : std::string("Brak B_Kapitelwechsel")); }));
    Opts({ { 1, "Rozdział 1" }, { 2, "Rozdział 2" }, { 3, "Rozdział 3" }, { 4, "Rozdział 4" }, { 5, "Rozdział 5" }, { 6, "Rozdział 6" } });
    St(ST_OK, "działa");
}
