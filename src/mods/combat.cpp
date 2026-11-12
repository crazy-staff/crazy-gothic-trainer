// Combat module (build 16): attack speed, infinite ammo, free aim, aimbot, trigger bot.
// Gothic 1 aims automatically at the "focus" NPC chosen by the C_FOCUS presets (FOCUS_MELEE/RANGED/MAGIC).
// Free aim = those presets get a zero azimuth (nothing gets locked, you shoot where you face).
// Aimbot   = picks a target by priority and turns the hero towards it (the engine then focuses it).
// Trigger  = when a valid target is in front and in range, presses the game's attack (Action + Forward).
#include "mods.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>

using namespace g1;
using namespace gx;

namespace {
// ---- focus presets ---------------------------------------------------------------------------------
struct FocusSave { void* p = nullptr; float azi = 0; bool saved = false; };
FocusSave g_focus[3];   // melee, ranged, magic
const char* kFocusInst[3] = { "FOCUS_MELEE", "FOCUS_RANGED", "FOCUS_MAGIC" };
const char* kFreeIds[3] = { "freemelee", "freeranged", "freemagic" };
void FocusTick() {
    static int aziOff = -2;
    if (aziOff == -2) aziOff = FieldOff("C_FOCUS.NPC_AZI", -1);
    if (aziOff < 0) return;
    for (int i = 0; i < 3; i++) {
        Sym* s = SymByName(kFocusInst[i]);
        void* p = s ? (void*)(uintptr_t)s->offset : nullptr;
        if (!p || IsBadReadPtr((char*)p + aziOff, 4)) continue;
        FocusSave& f = g_focus[i];
        if (f.p != p) { f.p = p; f.saved = false; }
        float& azi = fld<float>(p, aziOff);
        bool want = On("freeaim") && On(kFreeIds[i]);
        if (want) {
            if (!f.saved) { f.azi = azi; f.saved = true; }
            azi = 0.f;
        } else if (f.saved) { azi = f.azi; f.saved = false; }
    }
}
// ---- targets ---------------------------------------------------------------------------------------
std::unordered_map<void*, std::pair<double, bool>> g_host;
bool Hostile(void* n, void* h) {
    auto& c = g_host[n];
    if (NowSec() - c.first > 1.0) { c.first = NowSec(); c.second = CallExt("Npc_GetAttitude", { Arg::Npc(n), Arg::Npc(h) }) == K.attHostile; }
    return c.second;
}
bool Allowed(void* n, void* h) {
    if (!n || n == h || n == MainHero() || NpcDead(n) || IsAlly(n)) return false;
    bool host = Hostile(n, h), mon = IsMonster(n);
    if (host ? !On("aimen") : !On("aimne")) return false;
    if (mon ? !On("aimmo") : !On("aimhu")) return false;
    return true;
}
bool Visible(void* h, void* n) {
    using fn = int(__fastcall*)(void*, void*, void*, int);
    return reinterpret_cast<fn>(F_NPC_CANSEE)(h, nullptr, n, 1) != 0;
}
// signed yaw (radians) from the hero's facing to a point; + = turn right (same convention as no-clip Q/E)
float YawTo(void* h, const zVEC3& p) {
    const float* m = &fld<float>(h, VOB_TRAFO);
    zVEC3 hp = Pos(h);
    float tx = p.x - hp.x, tz = p.z - hp.z;
    float fx = m[2], fz = m[10];
    return std::atan2(tx * fz - tz * fx, tx * fx + tz * fz);
}
void Turn(void* h, float yaw) {
    float* m = &fld<float>(h, VOB_TRAFO);
    float c = std::cos(yaw), s = std::sin(yaw);
    for (int j = 0; j < 3; j++) { float a = m[j], b = m[8 + j]; m[j] = c * a + s * b; m[8 + j] = -s * a + c * b; }
}
void* g_aimT = nullptr;
void* PickTarget(void* h, float range, float fovDeg) {
    int prio = (int)Val("aimprio");
    bool needVis = !On("aimnonvis");
    void* best = nullptr; float bestScore = 1e30f;
    zVEC3 hp = Pos(h);
    NpcsInRadius(range, [&](void* n) {
        if (!Allowed(n, h)) return;
        float ang = std::fabs(YawTo(h, Pos(n))) * 57.29578f;
        if (ang > fovDeg * 0.5f) return;
        if (needVis && !Visible(h, n)) return;
        float d = Dist(Pos(n), hp), score;
        switch (prio) {
        case 1: score = d; break;
        case 2: score = (float)Attr(n, 0); break;
        case 3: score = -(float)Attr(n, 0); break;
        default: score = ang; break;
        }
        if (score < bestScore) { bestScore = score; best = n; }
    });
    return best;
}
bool WeaponDrawn(void* h) { return CallExt("Npc_IsInFightMode", { Arg::Npc(h), Arg::Int(ConstInt("FMODE_NONE", 0)) }) == 0; }
bool RangedOrMagic(void* h) {
    return CallExt("Npc_HasReadiedRangedWeapon", { Arg::Npc(h) }) != 0 || CallExt("Npc_IsInFightMode", { Arg::Npc(h), Arg::Int(ConstInt("FMODE_MAGIC", 7)) }) != 0;
}
bool AimKeyDown() {
    switch ((int)Val("aimkey")) {
    case 1: return (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    case 2: return (GetAsyncKeyState(VK_LMENU) & 0x8000) != 0;
    case 3: return (GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0;
    default: return true;
    }
}
// ---- trigger: Action + Forward through SendInput (the game reads input with DirectInput) ------------
int g_trigPhase = 0; double g_trigAt = 0, g_trigSeen = 0; void* g_trigT = nullptr;
void Key(WORD sc, bool ext, bool up) {
    INPUT in{}; in.type = INPUT_KEYBOARD; in.ki.wScan = sc;
    in.ki.dwFlags = KEYEVENTF_SCANCODE | (ext ? KEYEVENTF_EXTENDEDKEY : 0) | (up ? KEYEVENTF_KEYUP : 0);
    SendInput(1, &in, sizeof in);
}
void Mouse(bool up) { INPUT in{}; in.type = INPUT_MOUSE; in.mi.dwFlags = up ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_LEFTDOWN; SendInput(1, &in, sizeof in); }
void Attack(bool up) {
    if ((int)Val("trigkeys") == 1) { Mouse(up); Key(0x11, false, up); }       // LMB + W
    else { Key(0x1D, false, up); Key(0x48, true, up); }                       // Ctrl + Up
}
}

void Combat_Hooks() {}
void Combat_World() { g_aimT = nullptr; g_trigT = nullptr; g_host.clear(); for (auto& f : g_focus) f = FocusSave(); }
void Combat_Tick(TickCtx& c) {
    void* h = c.hero;
    if (!h || NpcDead(h)) return;
    FocusTick();
    if (g_host.size() > 500) g_host.clear();
    bool live = !c.paused && !T.menuOpen && GameFocused() && !On("noclip") && !On("glide");
    bool drawn = (On("aimbot") || On("trigbot") || On("infammo")) ? WeaponDrawn(h) : false;
    // infinite ammunition
    static double ammoAt = 0;
    if (On("infammo") && drawn && T.gameClock - ammoAt > 0.5) {
        ammoAt = T.gameClock;
        for (const char* id : { "ITAMARROW", "ITAMBOLT" }) { int s = SymIndex(id); if (s >= 0 && CallExt("Npc_HasItems", { Arg::Npc(h), Arg::Int(s) }) < 10) CreateItems(h, s, 30); }
    }
    // aimbot
    static double keyAt = -1;
    if (On("aimbot") && live && drawn) {
        bool k = AimKeyDown();
        if (k && keyAt < 0) keyAt = NowSec();
        if (!k) { keyAt = -1; g_aimT = nullptr; }
        if (k && NowSec() - keyAt >= Val("aimdelay") / 1000.f) {
            float range = Val("aimdist") * 100.f;
            bool keep = On("aimstick") && g_aimT && NpcExists(g_aimT) && Allowed(g_aimT, h) && Dist(Pos(g_aimT), Pos(h)) <= range;
            if (!keep) g_aimT = PickTarget(h, range, Val("aimfov"));
            if (g_aimT) {
                float d = YawTo(h, Pos(g_aimT));
                float step = On("aimsmooth") ? d * std::min(1.f, Val("aimspeed") / 100.f * c.dt * 20.f) : d;
                Turn(h, step);
                fld<void*>(h, NPC_FOCUSVOB) = g_aimT;
            }
        }
    } else { keyAt = -1; g_aimT = nullptr; }
    c.s.aimTarget = g_aimT ? U(NpcName(g_aimT)) : std::string();
    // trigger bot
    if (g_trigPhase == 1 && NowSec() - g_trigAt > 0.09) { Attack(true); g_trigPhase = 0; g_trigAt = NowSec(); }
    if (On("trigbot") && live && drawn && g_trigPhase == 0) {
        bool ranged = RangedOrMagic(h);
        float range = ranged ? Val("aimdist") * 100.f : Val("trigrange") * 100.f;
        void* t = PickTarget(h, range, Val("trigfov"));
        if (t != g_trigT) { g_trigT = t; g_trigSeen = NowSec(); }
        if (t && NowSec() - g_trigSeen >= Val("trigdelay") / 1000.f && NowSec() - g_trigAt >= Val("trigcd") / 1000.f) {
            Attack(false); g_trigPhase = 1; g_trigAt = NowSec();
        }
    } else if (g_trigPhase == 0) g_trigT = nullptr;
}

void Combat_Register() {
    const char* C = "Postać"; const char* W = "Walka";
    CardTitle("Szybkość walki");
    T.Add(MkRow("atkspeed", "Szybkość ataku", "tempo animacji tylko z bronią w ręku: ciosy, napinanie łuku, przeładowanie kuszy, rzucanie czarów", C, W, K_SLIDER, 1.8f, 1.f, 5.f, nullptr, true));
    St(ST_NEW, "nowe w build 16"); Icon(ICN_BOLT);
    T.Add(MkRow("infammo", "Nieskończona amunicja", "strzały i bełty dokładają się same, gdy łuk/kusza jest w ręku", C, W, K_TOGGLE)); St(ST_NEW, "nowe w build 16"); Icon(ICN_CROSSHAIR);
    CardTitle("Swobodne celowanie");
    T.Add(MkRow("freeaim", "Swobodne celowanie", "gra nie przyciąga celownika do postaci - trafiasz tam, gdzie patrzysz", C, W, K_TOGGLE)); St(ST_NEW, "nowe w build 16"); Icon(ICN_CROSSHAIR);
    for (int i = 0; i < 3; i++) { static const char* nm[3] = { "Walka wręcz", "Łuki i kusze", "Czary" }; T.Add(MkRow(kFreeIds[i], nm[i], "", C, W, K_TOGGLE)); T.rows.back().on = true; Hidden(); St(ST_NEW, "nowe w build 16"); }
    CardTitle("Celowanie");
    T.Add(MkRow("aimbot", "Aimbot", "obraca bohatera do celu wybranego wg priorytetu (gra sama go wtedy namierza)", C, W, K_TOGGLE)); Excl({ "trigbot" }); St(ST_NEW, "nowe w build 16"); Icon(ICN_CROSSHAIR);
    T.Add(MkRow("aimkey", "Klawisz", "", C, W, K_VALUE, 1, 0, 3)); Opts({ { 0, "Zawsze z bronią w ręku" }, { 1, "Prawy przycisk myszy" }, { 2, "Lewy Alt" }, { 3, "Lewy Shift" } }); St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("aimdelay", "Opóźnienie po wciśnięciu", "ms", C, W, K_VALUE, 0, 0, 1000)); St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("aimprio", "Priorytet", "", C, W, K_VALUE, 0, 0, 3)); Opts({ { 0, "Najbliżej celownika" }, { 1, "Najbliżej ciebie" }, { 2, "Najmniej HP" }, { 3, "Najwięcej HP" } }); St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("aimfov", "Pole widzenia", "stopnie wokół kierunku patrzenia", C, W, K_VALUE, 90, 5, 360)); St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("aimdist", "Zasięg", "m (też zasięg strzału trigger bota)", C, W, K_VALUE, 25, 2, 100)); St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("aimspeed", "Szybkość obrotu", "%", C, W, K_VALUE, 35, 1, 100)); St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("aimsmooth", "Płynny obrót", "wyłączone = natychmiast", C, W, K_TOGGLE)); T.rows.back().on = true; St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("aimstick", "Trzymaj się celu", "nie przełącza na inny, dopóki cel żyje i jest w zasięgu", C, W, K_TOGGLE)); T.rows.back().on = true; St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("aimnonvis", "Też niewidocznych", "cele za ścianą", C, W, K_TOGGLE)); St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("trigbot", "Trigger bot", "sam atakuje, gdy dozwolony cel jest przed tobą w zasięgu (broń w ręku)", C, W, K_TOGGLE)); Excl({ "aimbot" }); St(ST_NEW, "nowe w build 16"); Icon(ICN_BOLT);
    T.Add(MkRow("trigrange", "Zasięg wręcz", "m (łuk, kusza, czary: zasięg z Aimbota)", C, W, K_VALUE, 3, 1, 10, nullptr, true)); St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("trigfov", "Kąt", "stopnie - jak dokładnie cel musi być przed tobą", C, W, K_VALUE, 20, 2, 120)); St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("trigdelay", "Czas reakcji", "ms", C, W, K_VALUE, 120, 0, 2000)); St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("trigcd", "Odstęp między atakami", "ms", C, W, K_VALUE, 450, 50, 3000)); St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("trigkeys", "Klawisze ataku w grze", "jak masz ustawione sterowanie", C, W, K_VALUE, 0, 0, 1)); Opts({ { 0, "Ctrl + strzałka w górę" }, { 1, "Lewy przycisk myszy + W" } }); St(ST_NEW, "nowe w build 16");
    CardTitle("Cele");
    auto chk = [&](const char* id, const char* name, bool def) { T.Add(MkRow(id, name, "", C, W, K_TOGGLE)); T.rows.back().on = def; Hidden(); St(ST_NEW, "nowe w build 16"); };
    chk("aimen", "Wrogowie", true); chk("aimne", "Neutralni", false); chk("aimmo", "Potwory", true); chk("aimhu", "Ludzie", true);
}
