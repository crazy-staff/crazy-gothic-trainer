// Player module: survival, combat, stats, talents, movement (noclip/glide), hero speed.
#include "mods.h"
#include "tester.h"
#include "MinHook.h"
#include <algorithm>
#include <cmath>
#include <cstring>

using namespace g1;
using namespace gx;

namespace {
void* g_percSelf = nullptr;   // npc currently running its perception scripts
uint32_t g_ncSaved = 0; void* g_ncNpc = nullptr; int g_ncMode = -1;

struct Tal { const char* id; const char* name; const char* konst; int maxLvl; std::vector<std::pair<int, std::string>> opts; };
const std::vector<std::pair<int, std::string>> kLvl3 = { { 0, "Brak" }, { 1, "Uczeń" }, { 2, "Mistrz" } };
const std::vector<std::pair<int, std::string>> kLvl2 = { { 0, "Brak" }, { 1, "Opanowane" } };
const std::vector<std::pair<int, std::string>> kCircle = { { 0, "Brak" }, { 1, "1. krąg" }, { 2, "2. krąg" }, { 3, "3. krąg" },
                                                         { 4, "4. krąg" }, { 5, "5. krąg" }, { 6, "6. krąg" } };
const Tal kTal[] = {
    { "tal1h", "Broń jednoręczna", "NPC_TALENT_1H", 2, kLvl3 }, { "tal2h", "Broń dwuręczna", "NPC_TALENT_2H", 2, kLvl3 },
    { "talbow", "Łuk", "NPC_TALENT_BOW", 2, kLvl3 }, { "talxbow", "Kusza", "NPC_TALENT_CROSSBOW", 2, kLvl3 },
    { "talmage", "Krąg magii", "NPC_TALENT_MAGE", 6, kCircle }, { "tallock", "Otwieranie zamków", "NPC_TALENT_PICKLOCK", 2, kLvl3 },
    { "talpocket", "Kradzież kieszonkowa", "NPC_TALENT_PICKPOCKET", 2, kLvl3 }, { "talsneak", "Skradanie", "NPC_TALENT_SNEAK", 1, kLvl2 },
    { "talacro", "Akrobatyka", "NPC_TALENT_ACROBAT", 1, kLvl2 },
};

// Raise/lower a talent through the game's own teaching script (gives overlays, hit chance, UI).
void SetTalent(const Tal& t, int target) {
    void* h = Subject();
    int tal = ConstInt(t.konst, 1);
    int cur = CallExt("Npc_GetTalentSkill", { Arg::Npc(h), Arg::Int(tal) });
    bool script = SymIndex("B_GIVESKILL") >= 0;
    if (target > cur && script) {
        for (int l = cur + 1; l <= target; l++) CallScript("B_GiveSkill", { Arg::Npc(h), Arg::Int(tal), Arg::Int(l), Arg::Int(0) });
    }
    int now = CallExt("Npc_GetTalentSkill", { Arg::Npc(h), Arg::Int(tal) });
    if (now != target) CallExt("Npc_SetTalentSkill", { Arg::Npc(h), Arg::Int(tal), Arg::Int(target) });
}

// ---- noclip ---------------------------------------------------------------------------------
const uint32_t NC_MASK = VOBF_PHYSICS | VOBF_COLLSTAT | VOBF_COLLDYN;
// build 13: no-clip (fly) and glide are separate switches (exclusive); NcOn() = either of them
bool NcOn() { return On("noclip") || On("glide"); }
// build 14: no-clip and glide have their own settings (gl* / nc*), picked by the active mode
const char* NcP(const char* suffix) { static std::string b; b = std::string(On("glide") ? "gl" : "nc") + suffix; return b.c_str(); }
bool NcIgnore() { return (On("noclip") && On("ncignore")) || (On("glide") && On("glignore")); }
// build 15: rays that see the level mesh and every vob that has collision (bridges, grates, trees, rocks) by
// its polygons; vobs without collision (grass, decorations), items and characters are passed through
const int TR_FLOOR = TR_NEAREST | TR_STAT_POLY | TR_VOB_IGNORE_CHAR;
bool SolidRay(zVEC3 a, zVEC3 d, int flags, TraceHit& out) {
    float len = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
    if (len < 1e-3f) return false;
    zVEC3 u{ d.x / len, d.y / len, d.z / len };
    for (int k = 0; k < 6; k++) {
        if (!TraceRay(a, d, flags, out)) return false;
        if (!out.vob || (flags & TR_VOB_IGNORE)) return true;
        uint32_t vf = fld<uint32_t>(out.vob, VOB_FLAGS);
        if ((vf & (VOBF_COLLSTAT | VOBF_COLLDYN)) && !IsNpc(out.vob) && !IsItem(out.vob)) return true;
        float done = Dist(a, out.pos) + 2.f;   // pass through and look further
        if (done >= len) return false;
        a = { out.pos.x + u.x * 2.f, out.pos.y + u.y * 2.f, out.pos.z + u.z * 2.f };
        len -= done; d = { u.x * len, u.y * len, u.z * len };
    }
    return false;
}
float g_cPitchMouse = -0.28f;
void NoclipRestore() {
    if (!g_ncNpc) return;
    void* n = g_ncNpc; int mode = g_ncMode;
    g_ncNpc = nullptr; g_ncMode = -1;
    g_stealMouse = false; g_mouseDx = g_mouseDy = 0;
    if (!NpcExists(n)) return;
    if (mode == 0 || mode == 1) {
        SetSleeping(n, false);
        zVEC3 p = Pos(n);
        TraceHit h1, h2;
        // build 9 (#21): the first surface right below the hero wins - also see-through ones (grates over the Old Camp
        // prison were ignored, so the hero landed a floor lower). From above only when nothing is below (under the map).
        const int down = TR_FLOOR;
        const int fromTop = TR_NEAREST | TR_VOB_IGNORE | TR_STAT_POLY | TR_POLY_IGNORE_TRANSP;
        bool b = SolidRay({ p.x, p.y + 20, p.z }, { 0, -200000, 0 }, down, h1);
        bool a = !b && TraceRay({ p.x, p.y + 200000, p.z }, { 0, -199990, 0 }, fromTop, h2);
        zVEC3 dst = p; bool any = false;
        if (b) { dst = h1.pos; any = true; }
        else if (a) { dst = h2.pos; any = true; }
        // build 13: feet on the floor (the origin is the middle of the body - +40 left the hero half in the ground)
        const float* bb = &fld<float>(n, VOB_BBOX);
        float feet = std::max(40.f, std::min(140.f, p.y - bb[1]));
        if (any) SetPos(n, { dst.x, dst.y + feet + 6.f, dst.z });
        uint32_t& f = fld<uint32_t>(n, VOB_FLAGS);
        f = (f & ~NC_MASK) | (g_ncSaved & NC_MASK);
    }
}
// ---- glide (build 14) ------------------------------------------------------------------------
// Moves in 20 cm sub-steps. Per step: a ray at knee and at chest height looks ahead - a face steeper than the
// allowed angle stops the hero (sliding along it); an allowed steep face (up to a vertical wall at 90) is climbed
// straight up. Then a ray from knee height down finds the floor of the new spot. "Kolizja z obiektami" decides
// whether vobs (trees, rocks, bridges, grates, fences) count, or only the level mesh.
int GlideFlags() { return TR_NEAREST | TR_STAT_POLY | TR_POLY_NORMAL | TR_VOB_IGNORE_CHAR | (On("glcoll") ? 0 : TR_VOB_IGNORE); }
float SlopeDeg(const zVEC3& n) { float y = std::max(-1.f, std::min(1.f, n.y)); return std::acos(y) * 57.29578f; }
void GlideMove(void* h, zVEC3 p, float dx, float dz) {
    const float* bb = &fld<float>(h, VOB_BBOX);
    float feet = std::max(40.f, std::min(140.f, p.y - bb[1]));
    float footY = p.y - feet;
    float maxAng = std::max(0.f, std::min(90.f, Val("glideslope")));
    const float KNEE = 45.f, LOOK = 28.f;
    float total = std::sqrt(dx * dx + dz * dz);
    if (total < 1e-3f) return;
    int steps = std::max(1, (int)std::ceil(total / 20.f));
    float sx = dx / steps, sz = dz / steps, sl = total / steps;
    const int F = GlideFlags();
    zVEC3 cur = p;
    for (int i = 0; i < steps; i++) {
        float ux = sx / sl, uz = sz / sl;
        bool climb = false, blocked = false;
        for (int tries = 0; tries < 2 && !climb && !blocked; tries++) {
            bool again = false;
            for (float hgt : { footY + KNEE, footY + std::min(feet * 1.6f, 150.f) }) {
                TraceHit w;
                if (!SolidRay({ cur.x, hgt, cur.z }, { ux * (sl + LOOK), 0, uz * (sl + LOOK) }, F, w)) continue;
                float ang = SlopeDeg(w.normal);
                if (ang <= 60.f) continue;                       // a ramp - the floor ray below handles it
                if (ang <= maxAng + 0.5f) { climb = true; break; } // allowed steep face: go up it
                // too steep: slide along it (drop the part of the move that goes into the wall)
                float nl = std::sqrt(w.normal.x * w.normal.x + w.normal.z * w.normal.z);
                if (tries == 1 || nl < 1e-3f) { blocked = true; break; }
                float nx = w.normal.x / nl, nz = w.normal.z / nl, d = ux * nx + uz * nz;
                float vx = ux - nx * d, vz = uz - nz * d, vl = std::sqrt(vx * vx + vz * vz);
                if (vl < 0.15f) { blocked = true; break; }
                ux = vx / vl; uz = vz / vl; sl *= vl; again = true; break;
            }
            if (!again) break;
        }
        if (blocked) break;
        if (climb) {   // straight up, unless the head hits something
            TraceHit up;
            if (SolidRay({ cur.x, footY + feet * 2.f, cur.z }, { 0, sl + 10.f, 0 }, F, up)) break;
            footY += sl;
            continue;
        }
        float nx = cur.x + ux * sl, nz = cur.z + uz * sl;
        TraceHit g;
        if (SolidRay({ nx, footY + KNEE, nz }, { 0, -100000.f, 0 }, F, g)) {
            float rise = g.pos.y - footY;
            if (rise > 1.f && SlopeDeg(g.normal) > maxAng + 0.5f) break;   // steeper floor than allowed: stop
            footY = g.pos.y;
        }   // no floor at all (over the void): keep the height
        cur.x = nx; cur.z = nz;
    }
    SetPos(h, { cur.x, footY + feet, cur.z });
}
void NoclipFrame(float dt) {
    void* h = Player();
    if (!h) return;
    int mode = On("glide") ? 1 : 0;
    if (h != g_ncNpc || mode != g_ncMode) {
        NoclipRestore();
        g_ncNpc = h; g_ncMode = mode;
        g_ncSaved = fld<uint32_t>(h, VOB_FLAGS);
    }
    // fly and glide (build 12: glide too): no physics, no collision, AI asleep - the hero stands still instead of
    // playing run/stop animations on its own, and cannot fall when the floor ends
    fld<uint32_t>(h, VOB_FLAGS) &= ~NC_MASK;
    if ((fld<uint8_t>(h, VOB_SLEEPMODE) & 3) != 0) SetSleeping(h, true);
    g_stealMouse = false;   // build 11: the mouse is read from the engine's own state (works with ZMouseFix)
    if (T.menuOpen || !GameFocused()) return;
    auto k = [](int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; };
    float* m = &fld<float>(h, VOB_TRAFO);
    float yaw = 0;
    float turn = Val(NcP("turn")) * 0.0174533f;
    if (k('Q') || k(VK_LEFT)) yaw -= turn * dt;
    if (k('E') || k(VK_RIGHT)) yaw += turn * dt;
    if (On(NcP("mouse"))) {   // mouse: X turns the hero, Y tilts the camera. Engine mouse state (LeGo: Cursor_Ptr 0x86CCAC: relX, relY)
        int dx = at<int>(0x86CCAC), dy = at<int>(0x86CCB0);
        float sens = Val(NcP("msens")) * 0.0022f;
        yaw += dx * sens;
        g_cPitchMouse = std::max(-1.1f, std::min(0.9f, g_cPitchMouse - dy * sens));
    }
    if (yaw != 0) {
        float c = std::cos(yaw), s = std::sin(yaw);
        for (int j = 0; j < 3; j++) {
            float a = m[0 * 4 + j], b = m[2 * 4 + j];
            m[0 * 4 + j] = c * a + s * b;
            m[2 * 4 + j] = -s * a + c * b;
        }
    }
    zVEC3 at{ m[2], 0, m[10] }, right{ m[0], 0, m[8] };
    float sp = Val(NcP("speed")) * 100.f * (k(VK_SHIFT) ? 4.f : 1.f) * dt;
    zVEC3 p = Pos(h), d{ 0, 0, 0 };
    if (k('W')) { d.x += at.x; d.z += at.z; }
    if (k('S')) { d.x -= at.x; d.z -= at.z; }
    if (k('D')) { d.x += right.x; d.z += right.z; }
    if (k('A')) { d.x -= right.x; d.z -= right.z; }
    if (mode == 0) { if (k(VK_SPACE)) d.y += 1; if (k(VK_CONTROL)) d.y -= 1; }
    float len = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
    if (len < 1e-4f) return;
    zVEC3 np{ p.x + d.x / len * sp, p.y + d.y / len * sp, p.z + d.z / len * sp };
    if (mode == 1) { GlideMove(h, p, np.x - p.x, np.z - p.z); return; }
    SetPos(h, np);
}
// chase camera (build 9): fixed distance behind the hero like the game's run camera; the hero's yaw (mouse X,
// Q/E) turns it, mouse Y tilts it. No values are captured from the game camera any more - capturing it mid-
// transition is what sent the camera sideways in build 7/8.
bool g_camRel = false; float g_cDist = 330, g_cPitch = -0.28f, g_cUp = 120;
void NoclipCamera(void* h) {
    void* cv = fld<void*>(Game(), OG_CAMVOB);
    // build 10: when the mouse turns the hero the game camera no longer turns -> our camera must follow (#20)
    if (!cv) { g_camRel = false; return; }   // build 12: always our camera in no-clip (the switch did nothing - removed)
    zVEC3 H = Pos(h);
    if (!g_camRel) { g_cPitchMouse = -0.28f; g_camRel = true; }
    g_cPitch = g_cPitchMouse;
    zVEC3 a = AtVec(h);
    float l = std::sqrt(a.x * a.x + a.z * a.z); if (l < 1e-3f) return;
    a.x /= l; a.z /= l;
    float cp = std::cos(g_cPitch), sp = std::sin(g_cPitch);
    zVEC3 f{ a.x * cp, sp, a.z * cp };
    zVEC3 tgt{ H.x, H.y + g_cUp, H.z };
    zVEC3 c{ tgt.x - f.x * g_cDist, tgt.y - f.y * g_cDist, tgt.z - f.z * g_cDist };
    zVEC3 r{ f.z, 0, -f.x };
    float rl = std::sqrt(r.x * r.x + r.z * r.z); r.x /= rl; r.z /= rl;
    zVEC3 u{ f.y * r.z - f.z * r.y, f.z * r.x - f.x * r.z, f.x * r.y - f.y * r.x };
    float m[12] = { r.x, u.x, f.x, c.x, r.y, u.y, f.y, c.y, r.z, u.z, f.z, c.z };
    Cam_Override(m);
}

// ---- hooks ---------------------------------------------------------------------------------
using fnChangeAttr = void(__fastcall*)(void*, void*, int, int);
fnChangeAttr oChangeAttr = nullptr;
bool g_arenaHit = false, g_arenaClamped = false;
void __fastcall hkChangeAttr(void* self, void* edx, int attr, int value) {
    void* hero = Player();
    if (value < 0 && hero) {
        // arena: knocked down, never killed. Build 13: only clamp here; the loss is decided after the hit from the
        // real HP (god mode / no-clip take no damage - with them a hit never counts as a loss)
        if (g_arenaHit && self == hero && attr == 0 && !On("god") && !NcOn() && Attr(hero, 0) + value <= 0) {
            value = 1 - Attr(hero, 0); g_arenaClamped = true;
        }
        if (self == hero && attr == 2 && On("mana")) value = 0;
        if (attr == 0 && On("god") && self == MainHero()) value = 0;   // main body stays immortal while swapped
        if (attr == 0 && NcOn() && self == hero) value = 0;      // build 12: no damage in no-clip (falls, hits)
        if (self != hero && attr == 0) {
            Row* r = T.Get("dmg");
            if (r && r->on && r->val > 1.f && fld<void*>(hero, NPC_FOCUSVOB) == self) value = (int)std::lround(value * r->val);
        }
    }
    oChangeAttr(self, edx, attr, value);
}
using fnCanSee = int(__fastcall*)(void*, void*, void*, int);
fnCanSee oCanSee = nullptr;
int __fastcall hkCanSee(void* self, void* edx, void* vob, int freeLos) {
    if (vob && On("ghost")) { void* h = Player(); if (vob == h && self != h) return 0; }
    if (vob && NcOn() && NcIgnore()) { void* h = Player(); if (vob == h && self != h) return 0; }   // build 15
    if (vob && !g_allies.empty() && Party_IgnoreSense(self, vob)) return 0;   // build 14
    return oCanSee(self, edx, vob, freeLos);
}
// sight + hearing + smell: covers guards noticing a drawn weapon, gate dialogs etc.
using fnCanSense = int(__fastcall*)(void*, void*, void*);
fnCanSense oCanSense = nullptr;
int __fastcall hkCanSense(void* self, void* edx, void* vob) {
    void* h = Player();
    if (vob && vob == h && self != h) {
        if (On("ghost")) return 0;
        if (NcOn() && NcIgnore()) return 0;
        if (On("allynotalk") && IsAlly(self)) return 0;   // allies stop interrupting the player with dialogs
    }
    if (vob && !g_allies.empty() && Party_IgnoreSense(self, vob)) return 0;   // build 14
    return oCanSense(self, edx, vob);
}
// passive perceptions sent by/about the player (draw weapon, attack, theft...) are dropped in ghost mode
using fnPassive = void(__fastcall*)(void*, void*, int, void*, void*);
fnPassive oPassive = nullptr;
void __fastcall hkPassive(void* self, void* edx, int perc, void* other, void* victim) {
    void* h = Player();
    if (On("ghost") && (self == h || other == h)) return;
    static int pDraw = -2; if (pDraw == -2) pDraw = ConstInt("PERC_DRAWWEAPON", -1);
    if (On("noweapon") && perc == pDraw && self == h) return;
    oPassive(self, edx, perc, other, victim);
}
// per-npc active perception pass: perceptions about a drawn weapon are switched off for the
// duration of the call (and restored right after), so nobody reacts to your weapon
using fnPercCheck = void(__fastcall*)(void*, void*);
fnPercCheck oPercCheck = nullptr;
void __fastcall hkPercCheck(void* self, void* edx) {
    static int pFight = -2, pDraw = -2;
    if (pFight == -2) { pFight = ConstInt("PERC_ASSESSFIGHTER", -1); pDraw = ConstInt("PERC_DRAWWEAPON", -1); }
    if (!On("noweapon") || self == Player()) { oPercCheck(self, edx); return; }
    g_percSelf = self;
    int n = std::min(33, fld<int>(self, NPC_PERCCOUNT));
    int* list = &fld<int>(self, NPC_PERCLIST);
    int saved[33]; bool any = false;
    for (int i = 0; i < n; i++) { saved[i] = list[i * 2]; if ((list[i * 2] == pFight || list[i * 2] == pDraw) && list[i * 2] > 0) { list[i * 2] = 0; any = true; } }
    oPercCheck(self, edx);
    g_percSelf = nullptr;
    if (any) { int m = std::min(n, fld<int>(self, NPC_PERCCOUNT)); for (int i = 0; i < m; i++) if (list[i * 2] == 0 && saved[i] > 0) list[i * 2] = saved[i]; }
}
// hit reaction (stumble / knock-back) only - damage still applies
using fnDmgAnim = void(__fastcall*)(void*, void*, void*);
fnDmgAnim oDmgAnim = nullptr;
void __fastcall hkDmgAnim(void* self, void* edx, void* desc) {
    if (self && On("nohit") && (self == Player() || self == MainHero())) return;
    oDmgAnim(self, edx, desc);
}
// while an npc processes its perceptions, the player's drawn weapon is invisible to its scripts
using fnFightMode = int(__fastcall*)(void*, void*, int);
fnFightMode oFightMode = nullptr;
int __fastcall hkFightMode(void* self, void* edx, int mode) {
    if (g_percSelf && self == Player() && On("noweapon")) return 0;
    return oFightMode(self, edx, mode);
}
// the real hit path (melee, ranged, magic) - build 3 hooked a legacy wrapper that melee never used
using fnDamage = void(__fastcall*)(void*, void*, void*);
fnDamage oDamage = nullptr;
void __fastcall hkDamage(void* self, void* edx, void* desc) {
    if (self && desc && !g_allies.empty() && Party_BlockDamage(self, fld<void*>(desc, 8))) return;   // build 14
    if (self && desc && (self == Player() || self == MainHero())) {
        void* att = fld<void*>(desc, 8);                  // oSDamageDescriptor::pNpcAttacker
        if (IsNpc(att) && IsAlly(att)) { Party_AllyHitPlayer(att); return; }   // party never hurts you
        if (Arena_IsEnemy(att)) {
            g_arenaHit = true; g_arenaClamped = false;
            oDamage(self, edx, desc);
            g_arenaHit = false;
            if (g_arenaClamped && Attr(self, 0) <= 1) Arena_PlayerDown();   // confirmed: the hit really took us to 1 HP
            g_arenaClamped = false;
            return;
        }
    }
    oDamage(self, edx, desc);
}
}

void Player_Hooks() {
    MH_CreateHook((LPVOID)F_NPC_CHANGEATTR, (LPVOID)&hkChangeAttr, (LPVOID*)&oChangeAttr);
    MH_CreateHook((LPVOID)F_NPC_CANSEE, (LPVOID)&hkCanSee, (LPVOID*)&oCanSee);
    MH_CreateHook((LPVOID)F_NPC_CANSENSE, (LPVOID)&hkCanSense, (LPVOID*)&oCanSense);
    MH_CreateHook((LPVOID)F_NPC_PASSIVEPERC, (LPVOID)&hkPassive, (LPVOID*)&oPassive);
    MH_CreateHook((LPVOID)F_NPC_ONDAMAGE_DESC, (LPVOID)&hkDamage, (LPVOID*)&oDamage);
    MH_CreateHook((LPVOID)0x6B7400, (LPVOID)&hkPercCheck, (LPVOID*)&oPercCheck);   // oCNpc::PerceptionCheck
    MH_CreateHook((LPVOID)0x741990, (LPVOID)&hkDmgAnim, (LPVOID*)&oDmgAnim);       // oCNpc::OnDamage_Anim
    MH_CreateHook((LPVOID)0x6A19E0, (LPVOID)&hkFightMode, (LPVOID*)&oFightMode);   // oCNpc::IsInFightMode
}
void Player_World() { g_ncNpc = nullptr; g_ncMode = -1; }

void Player_Tick(TickCtx& c) {
    void* h = c.hero;
    at<int>(GODMODE) = On("god") ? 1 : 0;
    if (On("mana")) SetAttr(h, 2, Attr(h, 3));
    if (void* sb = Subject()) {
        c.s.subjectName = U(NpcName(sb)); c.s.subjectPtr = (uintptr_t)sb; c.s.subjectInst = InstName(sb);
    }
    static bool ncWas = false;
    if (NcOn() && !ncWas && NcIgnore()) NpcsInRadius(4000.f, [](void* n) { Peace(n); });   // old aggro is dropped too
    {   // build 15: whoever still fights you while ignoring is on (spawned hostile, old aggro) is calmed every second
        static double igAt = 0;
        if (NcOn() && NcIgnore() && !c.paused && T.gameClock - igAt > 1.0) {
            igAt = T.gameClock;
            NpcsInRadius(5000.f, [](void* n) {
                if (IsAlly(n) || NpcDead(n)) return;
                bool fight = (K.stAttack >= 0 && CallExt("Npc_IsInState", { Arg::Npc(n), Arg::Int(K.stAttack) })) ||
                             (K.stMmAttack >= 0 && CallExt("Npc_IsInState", { Arg::Npc(n), Arg::Int(K.stMmAttack) }));
                if (fight) Peace(n);
            });
        }
    }
    ncWas = NcOn();
    if (NcOn()) { NoclipFrame(c.paused ? 0.f : c.dt); NoclipCamera(h); }
    else { g_camRel = false; if (g_ncNpc) NoclipRestore(); }

    Snapshot& s = c.s;
    s.hp = Attr(h, 0); s.hpmax = Attr(h, 1); s.mana = Attr(h, 2); s.manamax = Attr(h, 3);
    s.str = Attr(h, 4); s.dex = Attr(h, 5); s.guild = fld<int>(h, K.offGuild); s.level = fld<int>(h, K.offLevel);
    static int lpOff = -1, expOff = -1, expNOff = -1;
    if (lpOff < 0) { lpOff = FieldOff("C_NPC.LP", 0x308); expOff = FieldOff("C_NPC.EXP", 0x300); expNOff = FieldOff("C_NPC.EXP_NEXT", 0x304); }
    s.lp = fld<int>(h, lpOff); s.exp = fld<int>(h, expOff); s.expNext = fld<int>(h, expNOff);
    s.bodyName = U(NpcName(h)); s.guildName = GuildName(s.guild);
    s.ore = K.nugget >= 0 ? CallExt("Npc_HasItems", { Arg::Npc(h), Arg::Int(K.nugget) }) : 0;
    s.swapped = Swapped();
    s.ghost = On("ghost"); s.noclip = NcOn();
}

// utf8 (menu) -> cp1250 (game strings), Polish letters only
std::string ToCp1250(const std::string& u) {
    static const struct { const char* u; char c; } kMap[] = {
        { "ą", '\xB9' }, { "ć", '\xE6' }, { "ę", '\xEA' }, { "ł", '\xB3' }, { "ń", '\xF1' }, { "ó", '\xF3' }, { "ś", '\x9C' }, { "ź", '\x9F' }, { "ż", '\xBF' },
        { "Ą", '\xA5' }, { "Ć", '\xC6' }, { "Ę", '\xCA' }, { "Ł", '\xA3' }, { "Ń", '\xD1' }, { "Ó", '\xD3' }, { "Ś", '\x8C' }, { "Ź", '\x8F' }, { "Ż", '\xAF' } };
    std::string o;
    for (size_t i = 0; i < u.size();) {
        bool hit = false;
        for (auto& m : kMap) { size_t l = strlen(m.u); if (u.compare(i, l, m.u) == 0) { o += m.c; i += l; hit = true; break; } }
        if (!hit) { if ((unsigned char)u[i] < 0x80) o += u[i]; i++; }
    }
    return o;
}
void Act_SetName(const std::string& utf8) {
    T.Queue("row:setname", [utf8] {
        void* sb = Subject(); if (!sb || utf8.empty()) return;
        static int off = -1; if (off < 0) off = FieldOff("C_NPC.NAME", 0x104);
        ZAssign(&fld<zSTRING>(sb, off), ToCp1250(utf8).c_str());
        T.Toast("Nowe imię: " + utf8);
    });
}
const char* kHeads[] = { "HUM_HEAD_FIGHTER", "HUM_HEAD_BALD", "HUM_HEAD_THIEF", "HUM_HEAD_PONY", "HUM_HEAD_PSIONIC", "HUM_HEAD_FATBALD" };
void Act_ApplyLook() {
    tst::Event("vhead", "Zastosuj wygląd (przycisk)");
    T.Queue("row:look", [] {
        void* sb = Subject();
        if (!sb || IsMonster(sb)) { T.Toast("Wygląd da się zmienić tylko ludziom"); return; }
        int head = std::max(0, std::min(5, (int)Val("vhead")));
        CallExt("Mdl_SetVisualBody", { Arg::Npc(sb), Arg::Str("HUM_BODY_NAKED0"), Arg::Int((int)Val("vbody")), Arg::Int((int)Val("vskin")),
                                       Arg::Str(kHeads[head]), Arg::Int((int)Val("vface")), Arg::Int(0), Arg::Int(-1) });
        CallExt("AI_EquipBestArmor", { Arg::Npc(sb) });   // the body swap drops the armour visual
        Schedule(0.6, [sb] { if (NpcExists(sb)) Photo_Refresh(sb); });
        T.Toast("Zmieniono wygląd");
    });
}

void Player_Register() {
    const char* C = "Postać";
    T.Add(MkRow("subject", "Kogo edytujesz", "", C, "Informacje", K_VALUE, 0, 0, 2)); Hidden();
    Opts({ { 0, "Obecna postać" }, { 1, "Bezimienny" }, { 2, "Cel" } }); St(ST_NEW, "nowe w build 5");

    const char* SS = "Informacje";   // build 12: stats + skills in one place
    CardTitle("Atrybuty");
    auto attr = [&](const char* id, const char* name, int idx, float def, float mx) {
        T.Add(MkRow(id, name, "ustawia wartość po kliknięciu Ustaw", C, SS, K_ACTION, def, 1, mx, [idx](Row& r) {
            void* sb = Subject(); SetAttr(sb, idx, (int)r.val);
            if (idx == 1) SetAttr(sb, 0, std::min(Attr(sb, 0), (int)r.val));
            if (idx == 3) SetAttr(sb, 2, std::min(Attr(sb, 2), (int)r.val));
            T.Toast(r.name + " = " + std::to_string((int)r.val)); }));
        St(ST_OK, "działa; build 7: przycisk Ustaw zamiast stałego nadpisywania");
    };
    attr("str", "Siła", 4, 200, 1000); attr("dex", "Zręczność", 5, 200, 1000);
    attr("hpmax", "Max HP", 1, 1000, 10000); attr("manamax", "Max mana", 3, 500, 10000);
    CardTitle("Doświadczenie");
    T.Add(MkRow("xp", "Dodaj doświadczenie", "przez B_GiveXP (awanse działają) - zawsze sterowana postać", C, SS, K_ACTION, 1000, 1, 1000000, [](Row& r) {
        bool ok; CallScript("B_GiveXP", { Arg::Int((int)r.val) }, &ok);
        T.Toast(ok ? "+" + std::to_string((int)r.val) + " PD" : std::string("Brak B_GiveXP")); }));
    St(ST_OK, "działa"); Icon(ICN_STAR);
    T.Add(MkRow("lp", "Dodaj punkty nauki", "PN", C, SS, K_ACTION, 50, 1, 1000, [](Row& r) {
        fld<int>(Subject(), FieldOff("C_NPC.LP", 0x308)) += (int)r.val; T.Toast("+" + std::to_string((int)r.val) + " PN"); }));
    St(ST_OK, "działa"); Icon(ICN_STAR);

    const char* SU = "Informacje";
    CardTitle("Poziom umiejętności");
    for (auto& t : kTal) {
        const Tal* tp = &t;
        T.Add(MkRow(t.id, t.name, "przez skrypt nauczyciela (animacje walki jak w grze)", C, SU, K_CHOICE,
                    (float)t.maxLvl, 0, (float)t.maxLvl, [tp](Row& r) { SetTalent(*tp, (int)r.val); T.Toast(r.name + ": ustawiono"); }));
        Opts(t.opts); Icon(ICN_STAR);
        St(ST_OK, "build 3: działa (poziomy)");
    }
    CardTitle("Szansa trafienia (%)");
    struct Hc { const char* id; const char* name; const char* konst; };
    static const Hc kHc[] = { { "hc1h", "Broń jednoręczna", "NPC_TALENT_1H" }, { "hc2h", "Broń dwuręczna", "NPC_TALENT_2H" },
                              { "hcbow", "Łuk", "NPC_TALENT_BOW" }, { "hcxbow", "Kusza", "NPC_TALENT_CROSSBOW" } };
    for (auto& h : kHc) {
        const Hc* hp = &h;
        T.Add(MkRow(h.id, h.name, "procent trafienia / opanowania broni", C, SU, K_ACTION, 100, 0, 100, [hp](Row& r) {
            CallExt("Npc_SetTalentValue", { Arg::Npc(Subject()), Arg::Int(ConstInt(hp->konst, 1)), Arg::Int((int)r.val) });
            T.Toast(r.name + ": " + std::to_string((int)r.val) + "%"); }));
        Icon(ICN_CROSSHAIR); St(ST_OK, "build 5: działa");
    }
    struct Th { const char* id; const char* name; const char* konst; };
    static const Th kTh[] = { { "hclock", "Otwieranie zamków", "NPC_TALENT_PICKLOCK" }, { "hcpocket", "Kradzież kieszonkowa", "NPC_TALENT_PICKPOCKET" },
                              { "hcsneak", "Skradanie", "NPC_TALENT_SNEAK" } };
    for (auto& h : kTh) {
        const Th* hp = &h;
        T.Add(MkRow(h.id, h.name, "procent umiejętności (jak w statystykach postaci)", C, SU, K_ACTION, 100, 0, 100, [hp](Row& r) {
            CallExt("Npc_SetTalentValue", { Arg::Npc(Subject()), Arg::Int(ConstInt(hp->konst, 1)), Arg::Int((int)r.val) }); }));
        St(ST_NEW, "nowe w build 6 (wymaksowanie ustawiało 50%)");
    }
    T.Add(MkRow("alltal", "Wymaksuj wszystko", "wszystkie umiejętności na max, 6. krąg, 100% trafienia", C, SU, K_ACTION, 0, 0, 0, [](Row&) {
        for (auto& t : kTal) SetTalent(t, t.maxLvl);
        for (auto& h : kHc) CallExt("Npc_SetTalentValue", { Arg::Npc(Subject()), Arg::Int(ConstInt(h.konst, 1)), Arg::Int(100) });
        for (const char* k : { "NPC_TALENT_PICKLOCK", "NPC_TALENT_PICKPOCKET", "NPC_TALENT_SNEAK" })
            CallExt("Npc_SetTalentValue", { Arg::Npc(Subject()), Arg::Int(ConstInt(k, 1)), Arg::Int(100) });
        T.Toast("Wszystkie umiejętności na maksa"); }));
    NoVal(); Icon(ICN_STAR); Hidden(); St(ST_OK, "build 3: działa; build 4: + 100% trafienia; build 14: przycisk w nagłówku karty Poziom umiejętności");

    const char* SW = "Wygląd";
    CardTitle("Twarz i ciało");
    T.Add(MkRow("vhead", "Głowa", "", C, SW, K_VALUE, 0, 0, 5));
    Opts({ { 0, "Wojownik" }, { 1, "Łysa" }, { 2, "Złodziej" }, { 3, "Kucyk" }, { 4, "Nowicjusz (Bractwo)" }, { 5, "Gruba łysa" } }); St(ST_NEW, "nowe w build 5");
    T.Add(MkRow("vface", "Twarz", "numer tekstury twarzy", C, SW, K_VALUE, 0, 0, 160)); St(ST_NEW, "nowe w build 5");
    T.Add(MkRow("vbody", "Ciało", "tekstura ciała", C, SW, K_VALUE, 0, 0, 3)); St(ST_NEW, "nowe w build 5");
    T.Add(MkRow("vskin", "Kolor skóry", "", C, SW, K_VALUE, 1, 0, 3)); St(ST_NEW, "nowe w build 5");
    T.Add(MkRow("pvauto", "Odświeżaj podgląd przy każdej zmianie", "", C, SW, K_TOGGLE)); Hidden(); St(ST_NEW, "nowe w build 13");
    CardTitle("Przynależność");
    T.Add(MkRow("pguild", "Gildia", "", C, "Informacje", K_CHOICE, 3, 1, 15, [](Row& r) {
        void* h = Subject();
        CallExt("Npc_SetTrueGuild", { Arg::Npc(h), Arg::Int((int)r.val) });
        fld<int>(h, K.offGuild) = (int)r.val;
        T.Toast("Gildia: " + GuildName((int)r.val)); }));
    { std::vector<std::pair<int, std::string>> o; for (int g = 1; g <= 15; g++) o.push_back({ g, GuildName(g) }); Opts(o); }
    St(ST_OK, "działa");
    CardTitle("Sylwetka");
    T.Add(MkRow("pscale", "Skala", "1 = normalnie", C, SW, K_ACTION, 1, 0.1f, 5, [](Row& r) {
        CallExt("Mdl_SetModelScale", { Arg::Npc(Subject()), Arg::Flt(r.val), Arg::Flt(r.val), Arg::Flt(r.val) }); }, true));
    St(ST_OK, "build 3: działa"); Icon(ICN_EXPAND);
    T.Add(MkRow("pfat", "Grubość", "0 = normalnie", C, SW, K_ACTION, 0, -2, 5, [](Row& r) {
        CallExt("Mdl_SetModelFatness", { Arg::Npc(Subject()), Arg::Flt(r.val) }); }, true));
    St(ST_OK, "build 5: działa");

    const char* SV = "Cheaty";
    CardTitle("Ochrona");
    T.Add(MkRow("god", "Nieśmiertelność", "HP nie spada; chroni też główne ciało podczas zamiany", C, SV, K_TOGGLE));
    St(ST_OK, "build 2-3: działa"); Icon(ICN_HEART);
    T.Add(MkRow("nohit", "Bez zachwiania od ciosów", "obrażenia wchodzą normalnie, ale postać nie reaguje na trafienia (z nieśmiertelnością: zero obrażeń i zero reakcji)", C, SV, K_TOGGLE));
    St(ST_NEW, "build 5: dawało pełną nieśmiertelność; build 6: tylko brak animacji trafienia, obrażenia wchodzą"); Icon(ICN_SHIELD);
    CardTitle("Ukrycie");
    T.Add(MkRow("ghost", "Tryb ducha", "nikt cię nie widzi ani nie słyszy; kradzież i broń nie wywołują reakcji", C, SV, K_TOGGLE, 0, 0, 0,
        [](Row& r) { if (r.on) NpcsInRadius(3000.f, [](void* n) { Peace(n); }); }));
    St(ST_OK, "build 5: działa"); Icon(ICN_GHOST); Excl({ "noweapon" });
    T.Add(MkRow("noweapon", "Ignorują wyciągniętą broń", "widzą cię normalnie, tylko nie reagują na broń w ręku (tryb ducha już to obejmuje)", C, SV, K_TOGGLE));
    St(ST_NEW, "build 5: nie działało; build 6: skrypty postaci nie widzą twojej broni"); Icon(ICN_EYE); Excl({ "ghost" });

    const char* SC = "Cheaty";
    CardTitle("Walka i magia");
    T.Add(MkRow("dmg", "Mnożnik obrażeń", "dla NPC, w którego celujesz", C, SC, K_SLIDER, 5, 1, 50, nullptr, true));
    St(ST_OK, "build 2-3: działa"); Icon(ICN_BOLT);
    T.Add(MkRow("mana", "Nieskończona mana", "czary nic nie kosztują", C, SC, K_TOGGLE));
    St(ST_NEW, "jeszcze nie testowane"); Icon(ICN_FLASK);

    const char* SR = "Ruch";
    CardTitle("No-clip");
    T.Add(MkRow("noclip", "No-clip (latanie)", "przez ściany; WASD, mysz obraca, Spacja/Ctrl góra/dół, Shift x4; Esc wyłącza", C, SR, K_TOGGLE));
    St(ST_OK, "build 12: działa (raport build 12)"); Icon(ICN_ROCKET); Excl({ "glide" });
    T.Add(MkRow("ncspeed", "Prędkość", "m/s (Shift x4)", C, SR, K_VALUE, 9, 1, 100)); St(ST_OK, "build 3: działa");
    T.Add(MkRow("ncturn", "Obrót Q/E", "stopnie na sekundę", C, SR, K_VALUE, 130, 20, 720)); St(ST_OK, "build 3: działa");
    T.Add(MkRow("ncmouse", "Mysz obraca postać", "ruch myszą w bok obraca postać, w górę/dół pochyla kamerę", C, SR, K_TOGGLE));
    T.rows.back().on = true; St(ST_OK, "build 12: działa");
    T.Add(MkRow("ncmsens", "Czułość myszy", "", C, SR, K_VALUE, 1.0f, 0.2f, 4.f, nullptr, true)); St(ST_NEW, "nowe w build 9");
    T.Add(MkRow("ncignore", "Wrogowie cię ignorują", "w no-clipie potwory i przeciwnicy cię nie zauważają", C, SR, K_TOGGLE));
    T.rows.back().on = true; St(ST_OK, "działa");
    CardTitle("Ślizg");
    T.Add(MkRow("glide", "Ślizg (po ziemi)", "chodzi po podłożu bez fizyki i animacji; WASD, Shift x4; Esc wyłącza", C, SR, K_TOGGLE));
    Excl({ "noclip" }); St(ST_NEW, "build 13: częściowo - wjeżdżał pod ziemię, ściana wyrzucała w górę; build 14: nowy ruch krokami + kolizja z obiektami"); Icon(ICN_ARROWS);
    T.Add(MkRow("glcoll", "Kolizja z obiektami", "drzewa, skały, mosty, kraty, płoty - wchodzisz na nie zamiast przez nie przelatywać", C, SR, K_TOGGLE));
    T.rows.back().on = true; St(ST_NEW, "nowe w build 14");
    T.Add(MkRow("glideslope", "Maks. nachylenie", "stopnie; strome ściany do tego kąta pokonuje (90 = wchodzi pionowo na każdą ścianę)", C, SR, K_VALUE, 50, 0, 90)); St(ST_NEW, "nowe w build 13");
    T.Add(MkRow("glspeed", "Prędkość", "m/s (Shift x4)", C, SR, K_VALUE, 6, 1, 60)); St(ST_NEW, "nowe w build 14 (osobno od no-clipa)");
    T.Add(MkRow("glturn", "Obrót Q/E", "stopnie na sekundę", C, SR, K_VALUE, 130, 20, 720)); St(ST_NEW, "nowe w build 14");
    T.Add(MkRow("glmouse", "Mysz obraca postać", "", C, SR, K_TOGGLE)); T.rows.back().on = true; St(ST_NEW, "nowe w build 14");
    T.Add(MkRow("glmsens", "Czułość myszy", "", C, SR, K_VALUE, 1.0f, 0.2f, 4.f, nullptr, true)); St(ST_NEW, "nowe w build 14");
    T.Add(MkRow("glignore", "Wrogowie cię ignorują", "", C, SR, K_TOGGLE)); T.rows.back().on = true; St(ST_NEW, "nowe w build 14");
    CardTitle("Bohater");
    T.Add(MkRow("pspeed", "Szybkość bohatera", "tempo animacji = bieg, ataki, obrót", C, SR, K_SLIDER, 1.5f, 0.2f, 5, nullptr, true));
    St(ST_OK, "build 3: działa"); Icon(ICN_BOLT);
    T.Add(MkRow("sprint", "Sprint", "overlay biegu z mikstury szybkości", C, SR, K_TOGGLE, 0, 0, 0, [](Row& r) {
        CallExt(r.on ? "Mdl_ApplyOverlayMds" : "Mdl_RemoveOverlayMds", { Arg::Npc(Player()), Arg::Str("HUMANS_SPRINT.MDS") }); }));
    St(ST_BUG, "build 6: dalej się zacina (jak ESP); build 7: usunięte skoki klatek trenera (zapis ustawień, szkielet ESP, lista NPC) - sprawdź ponownie");

    CardTitle("Ruda");
    T.Add(MkRow("ore", "Dodaj rudę", "bryłki rudy", "Ekwipunek", "Spawner", K_ACTION, 1000, 1, 100000, [](Row& r) {
        if (K.nugget >= 0) CreateItems(Player(), K.nugget, (int)r.val);
        T.Toast("+" + std::to_string((int)r.val) + " rudy"); }));
    St(ST_OK, "działa"); Icon(ICN_BAG); Hidden();
}
