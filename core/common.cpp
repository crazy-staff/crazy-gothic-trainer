#include "common.h"
#include "catalog.h"
#include "../trainer.h"
#include <algorithm>
#include <cmath>

extern HWND g_gameHwnd;

namespace gx {
Consts K;
std::mt19937 rng{ (unsigned)GetTickCount() };
std::vector<void*> g_savedTargets;
int g_activeSaved = -1;
std::vector<void*> g_allies;
static void* g_mainHero = nullptr;
struct Sched { double t; std::function<void()> f; };
static std::vector<Sched> g_sched;

void Consts::Refresh() {
    sepHum = ConstInt("GIL_SEPERATOR_HUM", 16); sepOrc = ConstInt("GIL_SEPERATOR_ORC", 37); gilMax = ConstInt("GIL_MAX", 42);
    attHostile = ConstInt("ATT_HOSTILE", 0); attNeutral = ConstInt("ATT_NEUTRAL", 2); attFriendly = ConstInt("ATT_FRIENDLY", 3);
    damFire = ConstInt("DAM_FIRE", 8); damMagic = ConstInt("DAM_MAGIC", 32);
    aivParty = ConstInt("AIV_PARTYMEMBER", 36);
    stAttack = SymIndex("ZS_ATTACK"); stMmAttack = SymIndex("ZS_MM_ATTACK"); stFollow = SymIndex("ZS_FOLLOWPC");
    stSummoned = SymIndex("ZS_MM_SUMMONEDBYPC"); stMmAll = SymIndex("ZS_MM_ALLSCHEDULER");
    instHero = SymIndex("PC_HERO"); nugget = SymIndex("ITMINUGGET");
    offGuild = FieldOff("C_NPC.GUILD", 0x1E8); offLevel = FieldOff("C_NPC.LEVEL", 0x1EC); offStartAi = FieldOff("C_NPC.START_AISTATE", -1);
    offItemName = FieldOff("C_ITEM.NAME", -1); offItemValue = FieldOff("C_ITEM.VALUE", -1); offItemAmount = FieldOff("C_ITEM.AMOUNT", -1);
}

std::string U(const std::string& s) { return Cp1250ToUtf8(s); }
bool IsMonster(void* npc) { return fld<int>(npc, K.offGuild) > K.sepHum; }
bool NpcExists(void* n) {
    bool found = false;
    if (n) ForEachNpc([&](void* x) { if (x == n) found = true; });
    return found;
}
bool NpcAlive(void* n) { return NpcExists(n) && !NpcDead(n); }

void* MainHero() { return NpcExists(g_mainHero) ? g_mainHero : nullptr; }
bool Swapped() { void* m = MainHero(); return m && Player() != m; }
void TrackMainHero() {
    void* p = Player();
    if (!p) return;
    static double lastScan = 0;
    if (!g_mainHero || !NpcExists(g_mainHero)) {
        g_mainHero = nullptr;
        if (NpcInstance(p) == K.instHero) { g_mainHero = p; return; }
        if (NowSec() - lastScan < 1.0) return;
        lastScan = NowSec();
        ForEachNpc([&](void* n) { if (!g_mainHero && NpcInstance(n) == K.instHero) g_mainHero = n; });
    }
}

void* SavedTarget() {
    // drop dead references
    g_savedTargets.erase(std::remove_if(g_savedTargets.begin(), g_savedTargets.end(), [](void* n) { return !NpcExists(n); }),
                         g_savedTargets.end());
    if (g_activeSaved >= (int)g_savedTargets.size()) g_activeSaved = (int)g_savedTargets.size() - 1;
    return g_activeSaved >= 0 ? g_savedTargets[g_activeSaved] : nullptr;
}
void* Target() {
    if (void* s = SavedTarget()) return s;
    return FocusNpc();
}

bool IsAlly(void* n) { return std::find(g_allies.begin(), g_allies.end(), n) != g_allies.end(); }
void MakeAlly(void* npc) {
    if (!npc || NpcDead(npc)) return;
    CallExt("Npc_SetAttitude", { Arg::Npc(npc), Arg::Int(K.attFriendly) });
    CallExt("Npc_SetTempAttitude", { Arg::Npc(npc), Arg::Int(K.attFriendly) });
    CallExt("Npc_ClearAIQueue", { Arg::Npc(npc) });
    if (IsMonster(npc)) {
        SetNpcAivar(npc, 20, 300); SetNpcAivar(npc, 21, 5); SetNpcAivar(npc, 23, 1);
        if (K.stSummoned >= 0) CallExt("AI_StartState", { Arg::Npc(npc), Arg::Int(K.stSummoned), Arg::Int(1), Arg::Str("") });
    } else {
        SetNpcAivar(npc, K.aivParty, 1);
        if (K.stFollow >= 0) CallExt("AI_StartState", { Arg::Npc(npc), Arg::Int(K.stFollow), Arg::Int(1), Arg::Str("") });
    }
    if (!IsAlly(npc)) g_allies.push_back(npc);
}
void Aggro(void* npc, void* victim) {
    if (!npc || !victim || NpcDead(npc)) return;
    CallExt("Npc_SetAttitude", { Arg::Npc(npc), Arg::Int(K.attHostile) });
    CallExt("Npc_SetTempAttitude", { Arg::Npc(npc), Arg::Int(K.attHostile) });
    CallExt("Npc_ClearAIQueue", { Arg::Npc(npc) });
    CallExt("Npc_SetTarget", { Arg::Npc(npc), Arg::Npc(victim) });
    int st = IsMonster(npc) ? K.stMmAttack : K.stAttack;
    if (st >= 0) CallExt("AI_StartState", { Arg::Npc(npc), Arg::Int(st), Arg::Int(0), Arg::Str("") });
}
void Peace(void* npc) {
    if (!npc || NpcDead(npc) || IsAlly(npc) || npc == Player()) return;
    void* h = Player();
    CallExt("Npc_ClearAIQueue", { Arg::Npc(npc) });
    int perm = CallExt("Npc_GetPermAttitude", { Arg::Npc(npc), Arg::Npc(h) });
    CallExt("Npc_SetTempAttitude", { Arg::Npc(npc), Arg::Int(perm) });
    if (IsMonster(npc)) {
        int st = K.offStartAi >= 0 ? fld<int>(npc, K.offStartAi) : -1;
        if (st <= 0) st = K.stMmAll;
        if (st > 0) CallExt("AI_StartState", { Arg::Npc(npc), Arg::Int(st), Arg::Int(0), Arg::Str("") });
    } else {
        CallExt("AI_ContinueRoutine", { Arg::Npc(npc) });
    }
}
void PeaceAll(float radius) { NpcsInRadius(radius, [](void* n) { Peace(n); }); }

void Effect(const char* fx, void* origin, void* target, int lvl, int dmg, int dtype, bool proj) {
    CallExt("Wld_PlayEffect", { Arg::Str(fx), Arg::Npc(origin), Arg::Npc(target), Arg::Int(lvl), Arg::Int(dmg),
                                Arg::Int(dtype), Arg::Int(proj ? 1 : 0) });
}
zVEC3 InFront(float dist) {
    void* h = Player();
    zVEC3 p = Pos(h), a = AtVec(h);
    return { p.x + a.x * dist, p.y + 30.f, p.z + a.z * dist };
}
zVEC3 Grounded(zVEC3 p, bool* water) {
    zVEC3 g;
    if (FindGround(p, 400.f, 3000.f, g, water)) return { g.x, g.y + 30.f, g.z };
    if (water) *water = false;
    return p;
}
zVEC3 SpawnPointNear(float minR, float maxR, float ang) {
    zVEC3 hp = Pos(Player());
    float r = minR + (maxR - minR) * (float)(rng() % 1000) / 1000.f;
    for (int tries = 0; tries < 8; tries++) {
        zVEC3 p{ hp.x + std::cos(ang) * r, hp.y + 50.f, hp.z + std::sin(ang) * r };
        bool water = false;
        zVEC3 g;
        if (!FindGround(p, 600.f, 2500.f, g, &water)) { if (r > 250) { r -= 150; continue; } return p; }
        if (!water) return { g.x, g.y + 40.f, g.z };
        r -= 150.f;
        if (r < 150.f) break;
    }
    return { hp.x + std::cos(ang) * 150.f, hp.y + 50.f, hp.z + std::sin(ang) * 150.f };
}

void Schedule(double d, std::function<void()> f) { g_sched.push_back({ NowSec() + d, std::move(f) }); }
void RunSchedule() {
    double now = NowSec();
    std::vector<Sched> due;
    for (auto it = g_sched.begin(); it != g_sched.end();) {
        if (it->t <= now) { due.push_back(std::move(*it)); it = g_sched.erase(it); } else ++it;
    }
    for (auto& d : due) d.f();
}
void ClearWorldState() {
    g_savedTargets.clear(); g_activeSaved = -1; g_allies.clear(); g_sched.clear(); g_mainHero = nullptr;
}

bool GameFocused() { return GetForegroundWindow() == g_gameHwnd; }
bool On(const char* id) { Row* r = T.Get(id); return r && r->on; }
float Val(const char* id) { Row* r = T.Get(id); return r ? r->val : 0.f; }
}
