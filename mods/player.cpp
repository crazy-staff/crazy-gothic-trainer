// Player module: survival, combat, stats, talents, movement (noclip/glide), hero speed.
#include "mods.h"
#include "MinHook.h"
#include <algorithm>
#include <cmath>
#include <cstring>

using namespace g1;
using namespace gx;

namespace {
uint32_t g_ncSaved = 0; void* g_ncNpc = nullptr; int g_ncMode = -1;

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
    // the camera AI eases towards the hero, so fast movement made it lag behind: carry it along
    if (On("nccam")) if (void* cv = fld<void*>(Game(), OG_CAMVOB)) {
        zVEC3 cp = Pos(cv);
        SafeSetPos(cv, { cp.x + np.x - p.x, cp.y + np.y - p.y, cp.z + np.z - p.z });
    }
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
// sight + hearing + smell: covers guards noticing a drawn weapon, gate dialogs etc.
using fnCanSense = int(__fastcall*)(void*, void*, void*);
fnCanSense oCanSense = nullptr;
int __fastcall hkCanSense(void* self, void* edx, void* vob) {
    void* h = Player();
    if (vob && vob == h && self != h) {
        if (On("ghost")) return 0;
        if (On("allynotalk") && IsAlly(self)) return 0;   // allies stop interrupting the player with dialogs
    }
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
    int n = std::min(33, fld<int>(self, NPC_PERCCOUNT));
    int* list = &fld<int>(self, NPC_PERCLIST);
    int saved[33]; bool any = false;
    for (int i = 0; i < n; i++) { saved[i] = list[i * 2]; if ((list[i * 2] == pFight || list[i * 2] == pDraw) && list[i * 2] > 0) { list[i * 2] = 0; any = true; } }
    oPercCheck(self, edx);
    if (any) { int m = std::min(n, fld<int>(self, NPC_PERCCOUNT)); for (int i = 0; i < m; i++) if (list[i * 2] == 0 && saved[i] > 0) list[i * 2] = saved[i]; }
}
// the real hit path (melee, ranged, magic) - build 3 hooked a legacy wrapper that melee never used
using fnDamage = void(__fastcall*)(void*, void*, void*);
fnDamage oDamage = nullptr;
void __fastcall hkDamage(void* self, void* edx, void* desc) {
    if (self && On("nohit") && (self == Player() || self == MainHero())) return;   // attacks pass through
    if (self && desc && (self == Player() || self == MainHero())) {
        void* att = fld<void*>(desc, 8);                  // oSDamageDescriptor::pNpcAttacker
        if (IsNpc(att) && IsAlly(att)) { Party_AllyHitPlayer(att); return; }   // party never hurts you
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
}
void Player_World() { g_ncNpc = nullptr; g_ncMode = -1; }

void Player_Tick(TickCtx& c) {
    void* h = c.hero;
    at<int>(GODMODE) = On("god") ? 1 : 0;
    if (On("mana")) SetAttr(h, 2, Attr(h, 3));
    if (void* sb = Subject()) {
        if (On("str")) SetAttr(sb, 4, (int)Val("str"));
        if (On("dex")) SetAttr(sb, 5, (int)Val("dex"));
        if (On("hpmax")) SetAttr(sb, 1, (int)Val("hpmax"));
        if (On("manamax")) SetAttr(sb, 3, (int)Val("manamax"));
        c.s.subjectName = U(NpcName(sb)); c.s.subjectPtr = (uintptr_t)sb;
    }
    if (On("noclip")) NoclipFrame(c.paused ? 0.f : c.dt); else if (g_ncNpc) NoclipRestore();

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
    T.Queue("row:look", [] {
        void* sb = Subject();
        if (!sb || IsMonster(sb)) { T.Toast("Wygląd da się zmienić tylko ludziom"); return; }
        int head = std::max(0, std::min(5, (int)Val("vhead")));
        CallExt("Mdl_SetVisualBody", { Arg::Npc(sb), Arg::Str("HUM_BODY_NAKED0"), Arg::Int((int)Val("vbody")), Arg::Int((int)Val("vskin")),
                                       Arg::Str(kHeads[head]), Arg::Int((int)Val("vface")), Arg::Int(0), Arg::Int(-1) });
        CallExt("AI_EquipBestArmor", { Arg::Npc(sb) });   // the body swap drops the armour visual
        Schedule(0.6, [sb] { if (NpcExists(sb)) Photo_Request(sb); });
        T.Toast("Zmieniono wygląd");
    });
}

void Player_Register() {
    const char* C = "Bohater";
    T.Add(MkRow("subject", "Kogo edytujesz", "", C, "Statystyki", K_VALUE, 0, 0, 2)); Hidden();
    Opts({ { 0, "Sterowana postać" }, { 1, "Bezimienny" }, { 2, "Cel" } }); St(ST_NEW, "nowe w build 5");

    const char* SS = "Statystyki";
    CardTitle("Atrybuty (włączone = nadpisuje grę)");
    T.Add(MkRow("str", "Siła", "", C, SS, K_SET, 200, 1, 1000)); St(ST_OK, "działa");
    T.Add(MkRow("dex", "Zręczność", "", C, SS, K_SET, 200, 1, 1000)); St(ST_OK, "działa");
    T.Add(MkRow("hpmax", "Max HP", "", C, SS, K_SET, 1000, 1, 10000)); St(ST_OK, "działa");
    T.Add(MkRow("manamax", "Max mana", "", C, SS, K_SET, 500, 1, 10000)); St(ST_OK, "działa");
    CardTitle("Doświadczenie");
    T.Add(MkRow("xp", "Dodaj doświadczenie", "przez B_GiveXP (awanse działają) - zawsze sterowana postać", C, SS, K_ACTION, 1000, 1, 1000000, [](Row& r) {
        bool ok; CallScript("B_GiveXP", { Arg::Int((int)r.val) }, &ok);
        T.Toast(ok ? "+" + std::to_string((int)r.val) + " PD" : std::string("Brak B_GiveXP")); }));
    St(ST_OK, "działa"); Icon(ICN_STAR);
    T.Add(MkRow("lp", "Dodaj punkty nauki", "PN", C, SS, K_ACTION, 50, 1, 1000, [](Row& r) {
        fld<int>(Subject(), FieldOff("C_NPC.LP", 0x308)) += (int)r.val; T.Toast("+" + std::to_string((int)r.val) + " PN"); }));
    St(ST_OK, "działa"); Icon(ICN_STAR);

    const char* SU = "Umiejętności";
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
        Icon(ICN_CROSSHAIR); St(ST_NEW, "nowe w build 4");
    }
    CardTitle("Szybko");
    T.Add(MkRow("alltal", "Wymaksuj wszystko", "wszystkie umiejętności na max, 6. krąg, 100% trafienia", C, SU, K_ACTION, 0, 0, 0, [](Row&) {
        for (auto& t : kTal) SetTalent(t, t.maxLvl);
        for (auto& h : kHc) CallExt("Npc_SetTalentValue", { Arg::Npc(Subject()), Arg::Int(ConstInt(h.konst, 1)), Arg::Int(100) });
        T.Toast("Wszystkie umiejętności na maksa"); }));
    NoVal(); Icon(ICN_STAR); St(ST_OK, "build 3: działa; build 4: + 100% trafienia");

    const char* SW = "Wygląd";
    CardTitle("Twarz i ciało");
    T.Add(MkRow("vhead", "Głowa", "", C, SW, K_VALUE, 0, 0, 5));
    Opts({ { 0, "Wojownik" }, { 1, "Łysa" }, { 2, "Złodziej" }, { 3, "Kucyk" }, { 4, "Nowicjusz (Bractwo)" }, { 5, "Gruba łysa" } }); St(ST_NEW, "nowe w build 5");
    T.Add(MkRow("vface", "Twarz", "numer tekstury twarzy", C, SW, K_VALUE, 0, 0, 160)); St(ST_NEW, "nowe w build 5");
    T.Add(MkRow("vbody", "Ciało", "tekstura ciała", C, SW, K_VALUE, 0, 0, 3)); St(ST_NEW, "nowe w build 5");
    T.Add(MkRow("vskin", "Kolor skóry", "", C, SW, K_VALUE, 1, 0, 3)); St(ST_NEW, "nowe w build 5");
    CardTitle("Przynależność i sylwetka");
    T.Add(MkRow("pguild", "Gildia", "", C, SW, K_CHOICE, 3, 1, 15, [](Row& r) {
        void* h = Subject();
        CallExt("Npc_SetTrueGuild", { Arg::Npc(h), Arg::Int((int)r.val) });
        fld<int>(h, K.offGuild) = (int)r.val;
        T.Toast("Gildia: " + GuildName((int)r.val)); }));
    { std::vector<std::pair<int, std::string>> o; for (int g = 1; g <= 15; g++) o.push_back({ g, GuildName(g) }); Opts(o); }
    St(ST_OK, "działa");
    T.Add(MkRow("pscale", "Skala", "1 = normalnie", C, SW, K_ACTION, 1, 0.1f, 5, [](Row& r) {
        CallExt("Mdl_SetModelScale", { Arg::Npc(Subject()), Arg::Flt(r.val), Arg::Flt(r.val), Arg::Flt(r.val) }); }, true));
    St(ST_OK, "build 3: działa"); Icon(ICN_EXPAND);
    T.Add(MkRow("pfat", "Grubość", "0 = normalnie", C, SW, K_ACTION, 0, -2, 5, [](Row& r) {
        CallExt("Mdl_SetModelFatness", { Arg::Npc(Subject()), Arg::Flt(r.val) }); }, true));
    St(ST_NEW, "nowe w build 4");

    const char* SV = "Ochrona";
    CardTitle("Ochrona");
    T.Add(MkRow("god", "Nieśmiertelność", "HP nie spada; chroni też główne ciało podczas zamiany", C, SV, K_TOGGLE));
    St(ST_OK, "build 2-3: działa"); Icon(ICN_HEART);
    T.Add(MkRow("nohit", "Ciosy przechodzą", "ataki w ciebie nie trafiają: bez obrażeń, bez zachwiania, bez odrzutu", C, SV, K_TOGGLE));
    St(ST_NEW, "build 3: nic nie robiło; build 4: hak na prawdziwej ścieżce trafienia"); Icon(ICN_SHIELD);
    CardTitle("Niewidzialność");
    T.Add(MkRow("ghost", "Tryb ducha", "nikt cię nie widzi ani nie słyszy; kradzież i broń nie wywołują reakcji", C, SV, K_TOGGLE, 0, 0, 0,
        [](Row& r) { if (r.on) NpcsInRadius(3000.f, [](void* n) { Peace(n); }); }));
    St(ST_BUG, "build 3: częściowo; build 4: blokada słuchu i percepcji pasywnych"); Icon(ICN_GHOST);
    T.Add(MkRow("noweapon", "Ignorują wyciągniętą broń", "ludzie nie reagują na broń w ręku (reszta zachowania normalna)", C, SV, K_TOGGLE));
    St(ST_NEW, "nowe w build 5"); Icon(ICN_EYE);

    const char* SC = "Walka";
    CardTitle("Walka i magia");
    T.Add(MkRow("dmg", "Mnożnik obrażeń", "dla NPC, w którego celujesz", C, SC, K_SLIDER, 5, 1, 50, nullptr, true));
    St(ST_OK, "build 2-3: działa"); Icon(ICN_BOLT);
    T.Add(MkRow("mana", "Nieskończona mana", "czary nic nie kosztują", C, SC, K_TOGGLE));
    St(ST_NEW, "jeszcze nie testowane"); Icon(ICN_FLASK);

    const char* SR = "Ruch";
    CardTitle("No-clip");
    T.Add(MkRow("noclip", "No-clip", "WASD, Q/E obrót, Spacja/Ctrl góra/dół, Shift x4; Esc wyłącza (wtedy menu gry i zapis są bezpieczne)", C, SR, K_TOGGLE));
    St(ST_BUG, "build 3: kamera zostawała w tyle; build 4: kamera przesuwana razem z bohaterem"); Icon(ICN_ROCKET);
    T.Add(MkRow("ncmode", "Tryb", "", C, SR, K_VALUE, 0, 0, 1));
    Opts({ { 0, "Latanie (przez ściany)" }, { 1, "Ślizg (po ziemi)" } }); St(ST_OK, "build 3: działa");
    T.Add(MkRow("ncspeed", "Prędkość", "m/s (Shift x4)", C, SR, K_VALUE, 9, 1, 100)); St(ST_OK, "build 3: działa");
    T.Add(MkRow("ncturn", "Obrót Q/E", "stopnie na sekundę", C, SR, K_VALUE, 130, 20, 720)); St(ST_OK, "build 3: działa");
    T.Add(MkRow("nccam", "Kamera bez opóźnienia", "kamera przesuwa się razem z bohaterem", C, SR, K_TOGGLE));
    T.rows.back().on = true; St(ST_NEW, "nowe w build 4");
    CardTitle("Bohater");
    T.Add(MkRow("pspeed", "Szybkość bohatera", "tempo animacji = bieg, ataki, obrót", C, SR, K_SLIDER, 1.5f, 0.2f, 5, nullptr, true));
    St(ST_OK, "build 3: działa"); Icon(ICN_BOLT);
    T.Add(MkRow("sprint", "Sprint", "overlay biegu z mikstury szybkości", C, SR, K_TOGGLE, 0, 0, 0, [](Row& r) {
        CallExt(r.on ? "Mdl_ApplyOverlayMds" : "Mdl_RemoveOverlayMds", { Arg::Npc(Player()), Arg::Str("HUMANS_SPRINT.MDS") }); }));
    St(ST_BUG, "build 3: działa, ale się zacina (to samo zacinanie jest w no-clipie - szukam wspólnej przyczyny)");

    CardTitle("Ruda");
    T.Add(MkRow("ore", "Dodaj rudę", "bryłki rudy", "Ekwipunek", "Spawner", K_ACTION, 1000, 1, 100000, [](Row& r) {
        if (K.nugget >= 0) CreateItems(Player(), K.nugget, (int)r.val);
        T.Toast("+" + std::to_string((int)r.val) + " rudy"); }));
    St(ST_OK, "działa"); Icon(ICN_BAG); Hidden();
}
