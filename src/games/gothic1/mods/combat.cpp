// Combat module (build 16): attack speed, infinite ammo, free aim, aimbot, trigger bot.
// Gothic 1 aims automatically at the "focus" NPC chosen by the C_FOCUS presets (FOCUS_MELEE/RANGED/MAGIC).
// Free aim (build 17) = for bows, crossbows and spells: those presets get a zero azimuth (nothing locks by itself),
//            an over-the-shoulder camera follows the mouse up/down, and whatever character is under the screen
//            centre becomes the focus - the game then aims the arrow / spell at it, also up and down.
// Aimbot   = picks a target by priority, turns the hero with the game's own AI_TurnToNPC and tilts the camera.
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
const char* kFreeIds[3] = { "freemelee", "freeranged", "freemagic" };   // build 17: melee is never changed (pointless in Gothic)
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
        bool want = i > 0 && On("freeaim") && On(kFreeIds[i]);
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
// build 17: keys the game does not use (Alt = jump, Shift / right button had game actions)
bool AimKeyDown() {
    static const int vk[] = { 0, VK_XBUTTON1, VK_XBUTTON2, VK_MBUTTON, VK_CAPITAL, 'V' };
    int k = std::max(0, std::min(5, (int)Val("aimkey")));
    return k == 0 || (GetAsyncKeyState(vk[k]) & 0x8000) != 0;
}
// ---- free aim camera + what is under the screen centre ---------------------------------------------
float g_faPitch = 0.f; bool g_faCam = false;
zVEC3 g_camPos{}, g_camDir{};
void FreeAimCamera(void* h, float pitchOverride, bool useOverride) {
    if (!useOverride) {
        int dy = at<int>(0x86CCB0);
        g_faPitch = std::max(-1.0f, std::min(0.9f, g_faPitch - dy * 0.0022f * std::max(0.2f, Val("famsens"))));
    } else g_faPitch += (pitchOverride - g_faPitch) * 0.35f;
    zVEC3 H = Pos(h);
    zVEC3 a = AtVec(h);
    float l = std::sqrt(a.x * a.x + a.z * a.z); if (l < 1e-3f) return;
    a.x /= l; a.z /= l;
    float cp = std::cos(g_faPitch), sp = std::sin(g_faPitch);
    zVEC3 f{ a.x * cp, sp, a.z * cp };
    zVEC3 rgt{ f.z, 0, -f.x }; float rl = std::sqrt(rgt.x * rgt.x + rgt.z * rgt.z); rgt.x /= rl; rgt.z /= rl;
    float dist = Val("famdist") * 100.f, side = Val("famside");
    zVEC3 tgt{ H.x + rgt.x * side, H.y + 110.f, H.z + rgt.z * side };
    zVEC3 c{ tgt.x - f.x * dist, tgt.y - f.y * dist, tgt.z - f.z * dist };
    zVEC3 u{ f.y * rgt.z - f.z * rgt.y, f.z * rgt.x - f.x * rgt.z, f.x * rgt.y - f.y * rgt.x };
    float m[12] = { rgt.x, u.x, f.x, c.x, rgt.y, u.y, f.y, c.y, rgt.z, u.z, f.z, c.z };
    Cam_Override(m);
    g_camPos = c; g_camDir = f; g_faCam = true;
}
// nearest NPC whose box the centre ray crosses before it hits a wall
void* RayNpc(void* h, float range) {
    zVEC3 o = g_camPos, d = g_camDir;
    TraceHit w; float wall = range;
    if (TraceRay(o, { d.x * range, d.y * range, d.z * range }, TR_NEAREST | TR_STAT_POLY | TR_VOB_IGNORE | TR_POLY_IGNORE_TRANSP, w)) wall = Dist(o, w.pos);
    void* best = nullptr; float bt = wall;
    NpcsInRadius(range, [&](void* n) {
        if (n == h || NpcDead(n)) return;
        const float* b = &fld<float>(n, VOB_BBOX);
        float t0 = 0, t1 = bt; float oo[3] = { o.x, o.y, o.z }, dd[3] = { d.x, d.y, d.z };
        for (int k = 0; k < 3; k++) {
            float lo = b[k] - 15.f, hi = b[3 + k] + 15.f;
            if (std::fabs(dd[k]) < 1e-6f) { if (oo[k] < lo || oo[k] > hi) return; continue; }
            float a1 = (lo - oo[k]) / dd[k], a2 = (hi - oo[k]) / dd[k];
            if (a1 > a2) std::swap(a1, a2);
            t0 = std::max(t0, a1); t1 = std::min(t1, a2);
            if (t0 > t1) return;
        }
        if (t0 < bt) { bt = t0; best = n; }
    });
    return best;
}
float PitchTo(void* n) {
    const float* b = &fld<float>(n, VOB_BBOX); zVEC3 p = Pos(n);
    zVEC3 aim{ p.x, (b[1] + b[4]) * 0.5f + (b[4] - b[1]) * 0.2f, p.z };
    float dx = aim.x - g_camPos.x, dy = aim.y - g_camPos.y, dz = aim.z - g_camPos.z;
    return std::atan2(dy, std::sqrt(dx * dx + dz * dz));
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
void Combat_World() { g_aimT = nullptr; g_trigT = nullptr; g_host.clear(); for (auto& f : g_focus) f = FocusSave(); g_faCam = false; g_faPitch = 0; }
void Combat_Tick(TickCtx& c) {
    void* h = c.hero;
    if (!h || NpcDead(h)) return;
    if (g_trigPhase == 1 && NowSec() - g_trigAt > 0.09) { Attack(true); g_trigPhase = 0; g_trigAt = NowSec(); }   // never leave a key held
    Guard("combat:focus", [] { FocusTick(); });
    if (g_host.size() > 500) g_host.clear();
    // build 17: aimbot / trigger bot need free aim; switching free aim off switches them off
    if (!On("freeaim")) for (const char* id : { "aimbot", "trigbot" }) if (Row* r = T.Get(id)) if (r->on) { r->on = false; T.Toast(r->name + ": wyłączone (wymaga Swobodnego celowania)"); }
    bool live = !c.paused && !T.menuOpen && GameFocused() && !On("noclip") && !On("glide");
    bool drawn = WeaponDrawn(h);
    bool ranged = drawn && RangedOrMagic(h);
    // infinite ammunition
    static double ammoAt = 0;
    if (On("infammo") && drawn && T.gameClock - ammoAt > 0.5) {
        ammoAt = T.gameClock;
        for (const char* id : { "ITAMARROW", "ITAMBOLT" }) { int s = SymIndex(id); if (s >= 0 && CallExt("Npc_HasItems", { Arg::Npc(h), Arg::Int(s) }) < 10) CreateItems(h, s, 30); }
    }
    c.s.faOn = false; c.s.faHas = false;
    bool magic = ranged && CallExt("Npc_IsInFightMode", { Arg::Npc(h), Arg::Int(ConstInt("FMODE_MAGIC", 7)) }) != 0;
    bool fa = On("freeaim") && ranged && live && (magic ? On("freemagic") : On("freeranged"));
    if (!fa) { g_faCam = false; g_faPitch = 0; g_aimT = nullptr; c.s.aimTarget.clear(); return; }
    c.s.faOn = true;
    // aimbot: pick a target while the key is held, turn the hero with the engine, tilt the camera towards it
    static double keyAt = -1, turnAt = 0;
    void* aim = nullptr;
    if (On("aimbot")) {
        bool k = AimKeyDown();
        if (k && keyAt < 0) keyAt = NowSec();
        if (!k) { keyAt = -1; g_aimT = nullptr; }
        if (k && NowSec() - keyAt >= Val("aimdelay") / 1000.f) {
            float range = Val("aimdist") * 100.f;
            bool keep = On("aimstick") && g_aimT && NpcExists(g_aimT) && Allowed(g_aimT, h) && Dist(Pos(g_aimT), Pos(h)) <= range;
            if (!keep) g_aimT = PickTarget(h, range, Val("aimfov"));
            aim = g_aimT;
            if (aim && T.gameClock - turnAt > 0.15 && std::fabs(YawTo(h, Pos(aim))) > 0.03f) {
                turnAt = T.gameClock;
                Guard("combat:turn", [&] { CallExt("AI_TurnToNPC", { Arg::Npc(h), Arg::Npc(aim) }); });
            }
        }
    } else { keyAt = -1; g_aimT = nullptr; }
    float pitchAim = 0; bool usePitch = false;
    if (aim && g_faCam) { pitchAim = PitchTo(aim); usePitch = true; }
    Guard("combat:camera", [&] { FreeAimCamera(h, pitchAim, usePitch); });
    // what the screen centre points at becomes the game's focus -> the shot goes there (also up / down)
    void* pointed = nullptr;
    Guard("combat:ray", [&] { pointed = RayNpc(h, Val("aimdist") * 100.f); });
    void* focus = aim ? aim : pointed;
    if (focus) {
        fld<void*>(h, NPC_FOCUSVOB) = focus;
        const float* b = &fld<float>(focus, VOB_BBOX); zVEC3 p = Pos(focus);
        if (On("fahl") && Project({ p.x, b[4] + 10.f, p.z }, c.s.faX, c.s.faY)) c.s.faHas = true;
    }
    c.s.aimTarget = focus ? U(NpcName(focus)) : std::string();
    // trigger bot: shoots / casts when an allowed character is under the crosshair
    if (On("trigbot") && g_trigPhase == 0) {
        void* t = (pointed && Allowed(pointed, h)) ? pointed : nullptr;
        if (t != g_trigT) { g_trigT = t; g_trigSeen = NowSec(); }
        if (t && NowSec() - g_trigSeen >= Val("trigdelay") / 1000.f && NowSec() - g_trigAt >= Val("trigcd") / 1000.f) { Attack(false); g_trigPhase = 1; g_trigAt = NowSec(); }
    }
}

void Combat_Register() {
    const char* C = "Postać"; const char* W = "Walka";
    CardTitle("Szybkość walki");
    T.Add(MkRow("atkspeed", "Szybkość ataku", "tempo animacji tylko z bronią w ręku: ciosy, napinanie łuku, przeładowanie kuszy, rzucanie czarów", C, W, K_SLIDER, 1.8f, 1.f, 5.f, nullptr, true));
    St(ST_NEW, "nowe w build 16"); Icon(ICN_BOLT);
    T.Add(MkRow("infammo", "Nieskończona amunicja", "strzały i bełty dokładają się same, gdy łuk/kusza jest w ręku", C, W, K_TOGGLE)); St(ST_NEW, "nowe w build 16"); Icon(ICN_CROSSHAIR);
    CardTitle("Swobodne celowanie");
    T.Add(MkRow("freeaim", "Swobodne celowanie", "łuk, kusza, czary: kamera za ramieniem, mysz góra/dół celuje - strzał leci w postać pod celownikiem (też w górę i w dół)", C, W, K_TOGGLE));
    St(ST_NEW, "build 16: tylko lewo-prawo; build 17: kamera z celowaniem w pionie, cel = postać pod środkiem ekranu"); Icon(ICN_CROSSHAIR);
    T.Add(MkRow("famsens", "Czułość myszy góra/dół", "", C, W, K_VALUE, 1.f, 0.2f, 4.f, nullptr, true)); St(ST_NEW, "nowe w build 17");
    T.Add(MkRow("famdist", "Odległość kamery", "m", C, W, K_VALUE, 2.4f, 1.f, 6.f, nullptr, true)); St(ST_NEW, "nowe w build 17");
    T.Add(MkRow("famside", "Przesunięcie w bok", "cm (kamera nad prawym ramieniem)", C, W, K_VALUE, 45, -120, 120)); St(ST_NEW, "nowe w build 17");
    T.Add(MkRow("fahl", "Podświetlaj cel pod celownikiem", "", C, W, K_TOGGLE)); T.rows.back().on = true; St(ST_NEW, "nowe w build 17");
    T.Add(MkRow("freemelee", "Walka wręcz", "", C, W, K_TOGGLE)); Hidden(); St(ST_NEW, "build 17: usunięte - wręcz nic nie blokuje");
    for (int i = 1; i < 3; i++) { static const char* nm[3] = { "", "Łuki i kusze", "Czary" }; T.Add(MkRow(kFreeIds[i], nm[i], "", C, W, K_TOGGLE)); T.rows.back().on = true; Hidden(); St(ST_NEW, "nowe w build 16"); }
    CardTitle("Celowanie");
    T.Add(MkRow("aimbot", "Aimbot", "wymaga swobodnego celowania; z wciśniętym klawiszem obraca bohatera do celu i ustawia na nim celownik", C, W, K_TOGGLE)); Excl({ "trigbot" }); Req({ "freeaim" });
    St(ST_NEW, "build 16: crash / nie działało, skok zamiast strzału (Alt); build 17: obrót silnikiem gry, inne klawisze"); Icon(ICN_CROSSHAIR);
    T.Add(MkRow("aimkey", "Klawisz", "", C, W, K_VALUE, 1, 0, 5)); Opts({ { 0, "Zawsze (z łukiem / czarem)" }, { 1, "Boczny przycisk myszy 4" }, { 2, "Boczny przycisk myszy 5" }, { 3, "Środkowy przycisk myszy" }, { 4, "Caps Lock" }, { 5, "Klawisz V" } }); St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("aimdelay", "Opóźnienie po wciśnięciu", "ms", C, W, K_VALUE, 0, 0, 1000)); St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("aimprio", "Priorytet", "", C, W, K_VALUE, 0, 0, 3)); Opts({ { 0, "Najbliżej celownika" }, { 1, "Najbliżej ciebie" }, { 2, "Najmniej HP" }, { 3, "Najwięcej HP" } }); St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("aimfov", "Pole widzenia", "stopnie wokół kierunku patrzenia", C, W, K_VALUE, 90, 5, 360)); St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("aimdist", "Zasięg", "m", C, W, K_VALUE, 40, 2, 100)); St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("aimspeed", "Szybkość obrotu", "%", C, W, K_VALUE, 35, 1, 100)); Hidden(); St(ST_NEW, "build 17: obrót robi silnik gry");
    T.Add(MkRow("aimsmooth", "Płynny obrót", "", C, W, K_TOGGLE)); Hidden(); St(ST_NEW, "build 17: obrót robi silnik gry");
    T.Add(MkRow("aimstick", "Trzymaj się celu", "nie przełącza na inny, dopóki cel żyje i jest w zasięgu", C, W, K_TOGGLE)); T.rows.back().on = true; St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("aimnonvis", "Też niewidocznych", "cele za ścianą", C, W, K_TOGGLE)); St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("trigbot", "Trigger bot", "wymaga swobodnego celowania; strzela / rzuca czar, gdy dozwolony cel jest pod celownikiem", C, W, K_TOGGLE)); Excl({ "aimbot" }); Req({ "freeaim" }); St(ST_NEW, "build 17: cel pod celownikiem"); Icon(ICN_BOLT);
    T.Add(MkRow("trigrange", "Zasięg wręcz", "m", C, W, K_VALUE, 3, 1, 10, nullptr, true)); Hidden(); St(ST_NEW, "build 17: tylko łuk / czary");
    T.Add(MkRow("trigfov", "Kąt", "stopnie", C, W, K_VALUE, 20, 2, 120)); Hidden(); St(ST_NEW, "build 17: cel pod celownikiem");
    T.Add(MkRow("trigdelay", "Czas reakcji", "ms", C, W, K_VALUE, 120, 0, 2000)); St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("trigcd", "Odstęp między strzałami", "ms", C, W, K_VALUE, 450, 50, 3000)); St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("trigkeys", "Klawisze ataku w grze", "jak masz ustawione sterowanie", C, W, K_VALUE, 0, 0, 1)); Opts({ { 0, "Ctrl + strzałka w górę" }, { 1, "Lewy przycisk myszy + W" } }); St(ST_NEW, "nowe w build 16");
    CardTitle("Cele");
    auto chk = [&](const char* id, const char* name, bool def) { T.Add(MkRow(id, name, "", C, W, K_TOGGLE)); T.rows.back().on = def; Hidden(); St(ST_NEW, "nowe w build 16"); };
    chk("aimen", "Wrogowie", true); chk("aimne", "Neutralni", false); chk("aimmo", "Potwory", true); chk("aimhu", "Ludzie", true);
}
