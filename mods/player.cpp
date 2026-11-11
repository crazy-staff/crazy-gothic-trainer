// Player module: survival, combat, stats, talents, movement (noclip/glide), hero speed.
#include "mods.h"
#include "MinHook.h"
#include <algorithm>
#include <cmath>

using namespace g1;
using namespace gx;

namespace {
uint32_t g_ncSaved = 0; void* g_ncNpc = nullptr; int g_ncMode = -1;
bool g_modelTouched = false;

struct Tal { const char* id; const char* name; const char* konst; int maxLvl; std::vector<std::pair<int, std::string>> opts; };
const std::vector<std::pair<int, std::string>> kLvl3 = { { 0, "Brak" }, { 1, "Uczeń" }, { 2, "Mistrz" } };
const std::vector<std::pair<int, std::string>> kLvl2 = { { 0, "Brak" }, { 1, "Opanowane" } };
const std::vector<std::pair<int, std::string>> kCircle = { { 0, "Brak" }, { 1, "1. krąg" }, { 2, "2. krąg" }, { 3, "3. krąg" },
                                                         { 4, "4. krąg" }, { 5, "5. krąg" }, { 6, "6. krąg" } };
const Tal kTal[] = {
    { "tal1h", "Broń jednoręczna", "NPC_TALENT_1H", 2, kLvl3 }, { "tal2h", "Broń dwuręczna", "NPC_TALENT_2H", 2, kLvl3 },
    { "talbow", "Łuk", "NPC_TALENT_BOW", 2, kLvl3 }, { "talxbow", "Kusza", "NPC_TALENT_CROSSBOW", 2, kLvl3 },
    { "talmage", "Krąg magii", "NPC_TALENT_MAGE", 6, kCircle }, { "tallock", "Otwieranie zamków", "NPC_TALENT_PICKLOCK", 1, kLvl2 },
    { "talpocket", "Kradzież kieszonkowa", "NPC_TALENT_PICKPOCKET", 1, kLvl2 }, { "talsneak", "Skradanie", "NPC_TALENT_SNEAK", 1, kLvl2 },
    { "talacro", "Akrobatyka", "NPC_TALENT_ACROBAT", 1, kLvl2 },
};

// Raise/lower a talent through the game's own teaching script (gives overlays, hit chance, UI).
void SetTalent(const Tal& t, int target) {
    void* h = Player();
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
void NoclipRestore() {
    if (!g_ncNpc) return;
    void* n = g_ncNpc; int mode = g_ncMode;
    g_ncNpc = nullptr; g_ncMode = -1;
    if (!NpcExists(n)) return;
    if (mode == 0) {
        SetSleeping(n, false);
        zVEC3 p = Pos(n);
        TraceHit h1, h2;
        const int fl = TR_NEAREST | TR_VOB_IGNORE | TR_STAT_POLY | TR_POLY_IGNORE_TRANSP;
        bool b = TraceRay({ p.x, p.y + 20, p.z }, { 0, -200000, 0 }, fl, h1);
        bool a = TraceRay({ p.x, p.y + 200000, p.z }, { 0, -199990, 0 }, fl, h2);
        zVEC3 dst = p; bool any = false;
        if (b && a) { dst = (std::fabs(h1.pos.y - p.y) <= std::fabs(h2.pos.y - p.y)) ? h1.pos : h2.pos; any = true; }
        else if (b) { dst = h1.pos; any = true; }
        else if (a) { dst = h2.pos; any = true; }
        if (any) SetPos(n, { dst.x, dst.y + 40.f, dst.z });
        uint32_t& f = fld<uint32_t>(n, VOB_FLAGS);
        f = (f & ~NC_MASK) | (g_ncSaved & NC_MASK);
    }
}
void NoclipFrame(float dt) {
    void* h = Player();
    if (!h) return;
    int mode = (int)Val("ncmode");
    if (h != g_ncNpc || mode != g_ncMode) {
        NoclipRestore();
        g_ncNpc = h; g_ncMode = mode;
        g_ncSaved = fld<uint32_t>(h, VOB_FLAGS);
    }
    if (mode == 0) {       // fly: no physics, no collision, AI asleep so it cannot snap us to the floor
        fld<uint32_t>(h, VOB_FLAGS) &= ~NC_MASK;
        if ((fld<uint8_t>(h, VOB_SLEEPMODE) & 3) != 0) SetSleeping(h, true);
    }
    if (T.menuOpen || !GameFocused()) return;
    auto k = [](int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; };
    float* m = &fld<float>(h, VOB_TRAFO);
    float yaw = 0;
    float turn = Val("ncturn") * 0.0174533f;
    if (k('Q') || k(VK_LEFT)) yaw -= turn * dt;
    if (k('E') || k(VK_RIGHT)) yaw += turn * dt;
    if (yaw != 0) {
        float c = std::cos(yaw), s = std::sin(yaw);
        for (int j = 0; j < 3; j++) {
            float a = m[0 * 4 + j], b = m[2 * 4 + j];
            m[0 * 4 + j] = c * a + s * b;
            m[2 * 4 + j] = -s * a + c * b;
        }
    }
    zVEC3 at{ m[2], 0, m[10] }, right{ m[0], 0, m[8] };
    float sp = Val("ncspeed") * 100.f * (k(VK_SHIFT) ? 4.f : 1.f) * dt;
    zVEC3 p = Pos(h), d{ 0, 0, 0 };
    if (k('W')) { d.x += at.x; d.z += at.z; }
    if (k('S')) { d.x -= at.x; d.z -= at.z; }
    if (k('D')) { d.x += right.x; d.z += right.z; }
    if (k('A')) { d.x -= right.x; d.z -= right.z; }
    if (mode == 0) { if (k(VK_SPACE)) d.y += 1; if (k(VK_CONTROL)) d.y -= 1; }
    float len = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
    if (len < 1e-4f) return;
    zVEC3 np{ p.x + d.x / len * sp, p.y + d.y / len * sp, p.z + d.z / len * sp };
    if (mode == 1) {
        // glide: stop in front of walls (ray at chest height), the game keeps us on the floor
        TraceHit hit;
        zVEC3 dir{ np.x - p.x, 0, np.z - p.z };
        if (TraceRay({ p.x, p.y + 60.f, p.z }, { dir.x * 1.6f, 0, dir.z * 1.6f }, TR_NEAREST | TR_VOB_IGNORE | TR_STAT_POLY | TR_POLY_IGNORE_TRANSP, hit))
            return;
    }
    SetPos(h, np);
}

// ---- hooks ---------------------------------------------------------------------------------
using fnChangeAttr = void(__fastcall*)(void*, void*, int, int);
fnChangeAttr oChangeAttr = nullptr;
void __fastcall hkChangeAttr(void* self, void* edx, int attr, int value) {
    void* hero = Player();
    if (value < 0 && hero) {
        if (self == hero && attr == 2 && On("mana")) value = 0;
        if (attr == 0 && On("god") && self == MainHero()) value = 0;   // main body stays immortal while swapped
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
    return oCanSee(self, edx, vob, freeLos);
}
using fnOnDamage = void(__fastcall*)(void*, void*, void*, void*, float, int, const zVEC3*);
fnOnDamage oOnDamage = nullptr;
void __fastcall hkOnDamage(void* self, void* edx, void* src, void* infl, float dmg, int type, const zVEC3* loc) {
    if (self && self == Player()) {
        if (On("nohit")) return;                                   // attacks pass through: no damage, no reaction
        if (On("nostagger")) {                                     // damage only, no hit reaction/knockback
            static int protOff = -1; if (protOff < 0) protOff = FieldOff("C_NPC.PROTECTION", 0x1A4);
            int idx = 0; for (int b = 0; b < 8; b++) if (type & (1 << b)) { idx = b; break; }
            int d = std::max(1, (int)dmg - fld<int>(self, protOff + 4 * idx));
            using fn = void(__thiscall*)(void*, int, int);
            reinterpret_cast<fn>(F_NPC_CHANGEATTR)(self, 0, -d);
            if (Attr(self, 0) <= 0) Kill(self, IsNpc(infl) ? infl : (IsNpc(src) ? src : nullptr));
            return;
        }
    }
    oOnDamage(self, edx, src, infl, dmg, type, loc);
}
}

void Player_Hooks() {
    MH_CreateHook((LPVOID)F_NPC_CHANGEATTR, (LPVOID)&hkChangeAttr, (LPVOID*)&oChangeAttr);
    MH_CreateHook((LPVOID)F_NPC_CANSEE, (LPVOID)&hkCanSee, (LPVOID*)&oCanSee);
    MH_CreateHook((LPVOID)F_NPC_ONDAMAGE, (LPVOID)&hkOnDamage, (LPVOID*)&oOnDamage);
}
void Player_World() { g_ncNpc = nullptr; g_ncMode = -1; g_modelTouched = false; }

void Player_Tick(TickCtx& c) {
    void* h = c.hero;
    at<int>(GODMODE) = On("god") ? 1 : 0;
    if (On("mana")) SetAttr(h, 2, Attr(h, 3));
    if (On("str")) SetAttr(h, 4, (int)Val("str"));
    if (On("dex")) SetAttr(h, 5, (int)Val("dex"));
    if (On("hpmax")) SetAttr(h, 1, (int)Val("hpmax"));
    if (On("manamax")) SetAttr(h, 3, (int)Val("manamax"));
    if (On("noclip")) NoclipFrame(c.paused ? 0.f : c.dt); else if (g_ncNpc) NoclipRestore();

    // hero animation speed: own multiplier, and optionally immune to the world-speed slider
    float want = On("pspeed") ? Val("pspeed") : 1.f;
    if (On("speed") && On("speedonlyworld")) want /= std::max(0.05f, Val("speed"));
    if (void* m = VobModel(h)) {
        if (want != 1.f) { fld<float>(m, MODEL_TIMESCALE) = want; g_modelTouched = true; }
        else if (g_modelTouched) { fld<float>(m, MODEL_TIMESCALE) = 1.f; g_modelTouched = false; }
    }

    Snapshot& s = c.s;
    s.hp = Attr(h, 0); s.hpmax = Attr(h, 1); s.mana = Attr(h, 2); s.manamax = Attr(h, 3);
    s.str = Attr(h, 4); s.dex = Attr(h, 5); s.guild = fld<int>(h, K.offGuild); s.level = fld<int>(h, K.offLevel);
    static int lpOff = -1, expOff = -1, expNOff = -1;
    if (lpOff < 0) { lpOff = FieldOff("C_NPC.LP", 0x308); expOff = FieldOff("C_NPC.EXP", 0x300); expNOff = FieldOff("C_NPC.EXP_NEXT", 0x304); }
    s.lp = fld<int>(h, lpOff); s.exp = fld<int>(h, expOff); s.expNext = fld<int>(h, expNOff);
    s.bodyName = U(NpcName(h)); s.guildName = GuildName(s.guild);
    s.ore = K.nugget >= 0 ? CallExt("Npc_HasItems", { Arg::Npc(h), Arg::Int(K.nugget) }) : 0;
    s.swapped = Swapped();
    s.ghost = On("ghost"); s.noclip = On("noclip");
}

void Player_Register() {
    const char* C = "Gracz";
    T.Add(MkRow("god", "Nieśmiertelność", "HP nie spada; działa też na główne ciało podczas zamiany", C, "Przetrwanie", K_TOGGLE));
    St(ST_OK, "działa (potwierdzone w build 2); build 3: chroni też główne ciało przy zamianie"); Icon(ICN_HEART);
    T.Add(MkRow("nohit", "Ciosy przechodzą", "ataki nie zadają obrażeń i nie wywołują reakcji", C, "Przetrwanie", K_TOGGLE));
    St(ST_NEW, "nowe w build 3"); Icon(ICN_SHIELD);
    T.Add(MkRow("nostagger", "Bez reakcji na ciosy", "obrażenia wchodzą, ale bez zachwiania/odrzutu", C, "Przetrwanie", K_TOGGLE));
    St(ST_NEW, "nowe w build 3"); Icon(ICN_SHIELD);
    T.Add(MkRow("mana", "Nieskończona mana", "czary nic nie kosztują", C, "Przetrwanie", K_TOGGLE));
    St(ST_NEW, "nie testowane jeszcze"); Icon(ICN_FLASK);
    T.Add(MkRow("ghost", "Tryb ducha", "NPC i potwory cię nie widzą; walczący odpuszczają", C, "Przetrwanie", K_TOGGLE, 0, 0, 0,
        [](Row& r) { if (r.on) NpcsInRadius(5000.f, [](void* n) { Peace(n); }); }));
    St(ST_NEW, "build 2: CRASH (pusty argument NPC) - przyczyna naprawiona w build 3"); Icon(ICN_GHOST);
    T.Add(MkRow("dmg", "Mnożnik obrażeń", "dla NPC, w którego celujesz", C, "Walka", K_SLIDER, 5, 1, 50, nullptr, true));
    St(ST_OK, "działa (potwierdzone w build 2)"); Icon(ICN_BOLT);

    T.Add(MkRow("str", "Siła", "x = przywróć wartość gry", C, "Statystyki", K_SET, 200, 1, 1000)); St(ST_OK, "działa");
    T.Add(MkRow("dex", "Zręczność", "x = przywróć wartość gry", C, "Statystyki", K_SET, 200, 1, 1000)); St(ST_OK, "działa");
    T.Add(MkRow("hpmax", "Max HP", "x = przywróć wartość gry", C, "Statystyki", K_SET, 1000, 1, 100000)); St(ST_OK, "działa");
    T.Add(MkRow("manamax", "Max mana", "x = przywróć wartość gry", C, "Statystyki", K_SET, 500, 1, 100000)); St(ST_OK, "działa");

    for (auto& t : kTal) {
        const Tal* tp = &t;
        T.Add(MkRow(t.id, t.name, "przez skrypt nauczyciela (animacje walki i trafienia jak w grze)", C, "Umiejętności", K_CHOICE,
                    (float)t.maxLvl, 0, (float)t.maxLvl, [tp](Row& r) { SetTalent(*tp, (int)r.val); T.Toast(r.name + ": ustawiono"); }));
        Opts(t.opts); Icon(ICN_STAR);
        St(ST_NEW, "build 2: tylko 'wszystkie naraz' i nie działało (pusty argument NPC); build 3: osobno + B_GiveSkill");
    }
    T.Add(MkRow("alltal", "Wymaksuj wszystko", "wszystkie umiejętności na max + 6. krąg", C, "Umiejętności", K_ACTION, 0, 0, 0, [](Row&) {
        for (auto& t : kTal) SetTalent(t, t.maxLvl);
        T.Toast("Wszystkie umiejętności na maksa"); }));
    NoVal(); Icon(ICN_STAR); St(ST_NEW, "build 2: częściowo (tylko zamki/kieszenie); build 3: przez B_GiveSkill");

    T.Add(MkRow("ore", "Dodaj rudę", "bryłki rudy", C, "Ekonomia", K_ACTION, 1000, 1, 100000, [](Row& r) {
        if (K.nugget >= 0) CreateItems(Player(), K.nugget, (int)r.val);
        T.Toast("+" + std::to_string((int)r.val) + " rudy"); }));
    St(ST_OK, "działa");
    T.Add(MkRow("lp", "Dodaj punkty nauki", "PN", C, "Ekonomia", K_ACTION, 50, 1, 1000, [](Row& r) {
        fld<int>(Player(), FieldOff("C_NPC.LP", 0x308)) += (int)r.val; T.Toast("+" + std::to_string((int)r.val) + " PN"); }));
    St(ST_OK, "działa");
    T.Add(MkRow("xp", "Dodaj doświadczenie", "przez B_GiveXP (awanse działają)", C, "Ekonomia", K_ACTION, 1000, 1, 1000000, [](Row& r) {
        bool ok; CallScript("B_GiveXP", { Arg::Int((int)r.val) }, &ok);
        T.Toast(ok ? "+" + std::to_string((int)r.val) + " PD" : std::string("Brak B_GiveXP")); }));
    St(ST_OK, "działa");
    T.Add(MkRow("pguild", "Gildia gracza", "", C, "Tożsamość", K_CHOICE, 3, 1, 15, [](Row& r) {
        void* h = Player();
        CallExt("Npc_SetTrueGuild", { Arg::Npc(h), Arg::Int((int)r.val) });
        fld<int>(h, K.offGuild) = (int)r.val;
        T.Toast("Gildia: " + GuildName((int)r.val)); }));
    { std::vector<std::pair<int, std::string>> o; for (int g = 1; g <= 15; g++) o.push_back({ g, GuildName(g) }); Opts(o); }
    St(ST_OK, "działa");

    T.Add(MkRow("noclip", "No-clip", "WASD, Q/E obrót, Spacja/Ctrl góra/dół (latanie), Shift szybciej", C, "Ruch", K_TOGGLE));
    St(ST_NEW, "build 2: przyklejone do ziemi; build 3: dwa tryby - Latanie (bez AI i fizyki) i Ślizg (po ziemi, ściany blokują)");
    Icon(ICN_ROCKET);
    T.Add(MkRow("ncmode", "Tryb no-clip", "", C, "Ruch", K_VALUE, 0, 0, 1));
    Opts({ { 0, "Latanie (przez ściany)" }, { 1, "Ślizg (po ziemi, ściany blokują)" } }); St(ST_NEW, "nowe w build 3");
    T.Add(MkRow("ncspeed", "Prędkość no-clip", "m/s (Shift x4)", C, "Ruch", K_VALUE, 9, 1, 100)); St(ST_NEW, "nowe w build 3");
    T.Add(MkRow("ncturn", "Szybkość obrotu Q/E", "stopnie na sekundę", C, "Ruch", K_VALUE, 130, 20, 720)); St(ST_NEW, "nowe w build 3");
    T.Add(MkRow("pspeed", "Szybkość bohatera", "tempo animacji = bieg, ataki, obrót", C, "Ruch", K_SLIDER, 1.5f, 0.2f, 5, nullptr, true));
    St(ST_NEW, "nowe w build 3"); Icon(ICN_BOLT);
    T.Add(MkRow("sprint", "Sprint", "overlay biegu z mikstury szybkości", C, "Ruch", K_TOGGLE, 0, 0, 0, [](Row& r) {
        CallExt(r.on ? "Mdl_ApplyOverlayMds" : "Mdl_RemoveOverlayMds", { Arg::Npc(Player()), Arg::Str("HUMANS_SPRINT.MDS") }); }));
    St(ST_NEW, "build 2: brak efektu (pusty argument NPC) - naprawione w build 3");
}
