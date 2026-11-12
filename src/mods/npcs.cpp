// NPC module: spawner, arena waves, mass ally/kill.
#include "mods.h"
#include "MinHook.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <unordered_map>
#include <unordered_set>
#include "../tester.h"

using namespace g1;
using namespace gx;
std::string Npcs_Kind(void* n);
bool Target_IsFrozen(void* n);
std::string ToCp1250(const std::string& u);   // player.cpp

namespace {
std::vector<void*> g_arena;
std::set<void*> g_frozenSpawn;
int g_wave = 0; double g_next = 0; bool g_rewarded = true; bool g_lost = false;

void ArenaClear() { for (void* n : g_arena) if (NpcExists(n)) RemoveVob(n); g_arena.clear(); }
// after a cleared wave: ore that grows with the wave + 1-2 random items whose value fits the wave
void ArenaReward() {
    void* h = Player(); if (!h) return;
    int ore = 15 * g_wave + (int)(rng() % (10 * g_wave + 1));
    if (K.nugget >= 0) CreateItems(h, K.nugget, ore);
    std::vector<const ItemEntry*> pool;
    int lo = 20 * g_wave, hi = 150 * g_wave + 100;
    for (auto& e : g_cat.items) if (e.value >= lo && e.value <= hi && e.sym != K.nugget && !T.Crashed("item:" + e.id)) pool.push_back(&e);
    std::string got = std::to_string(ore) + " rudy";
    int n = pool.empty() ? 0 : 1 + (int)(rng() % 2);
    for (int i = 0; i < n; i++) { const ItemEntry* e = pool[rng() % pool.size()]; CreateItems(h, e->sym, 1); got += ", " + U(e->name); }
    T.Toast("Nagroda za falę " + std::to_string(g_wave) + ": " + got);
}

// "Arena: Ork wojownik" - readable name (script name, or the id made readable); testers also see the id
std::string PrettyId(const std::string& id) {
    std::string o; bool up = true;
    for (char c : id) {
        if (c == '_') { o += ' '; up = true; continue; }
        if (isdigit((unsigned char)c)) continue;
        o += up ? (char)toupper((unsigned char)c) : (char)tolower((unsigned char)c); up = false;
    }
    while (!o.empty() && o.back() == ' ') o.pop_back();
    return o;
}
void ArenaName(void* n) {
    static int nameOff = FieldOff("C_NPC.NAME", 0x104);
    std::string id = InstName(n), nm = U(NpcName(n));
    bool idLike = nm.empty() || nm == id || std::all_of(nm.begin(), nm.end(), [](char c) { return isupper((unsigned char)c) || isdigit((unsigned char)c) || c == '_'; });
    std::string full = "Arena: " + (idLike ? PrettyId(id) : nm);
    if (T.testerMode) full += " (" + id + ")";
    ZAssign(&fld<zSTRING>(n, nameOff), ToCp1250(full).c_str());
}
void ArenaFrame() {
    double now = NowSec();
    g_arena.erase(std::remove_if(g_arena.begin(), g_arena.end(), [](void* n) { return !NpcAlive(n); }), g_arena.end());
    static double lastAggro = 0;
    if (now - lastAggro > 3.0) { lastAggro = now; for (void* n : g_arena) CallExt("Npc_SetTarget", { Arg::Npc(n), Arg::Npc(Player()) }); }
    if (!g_arena.empty()) return;
    if (!g_rewarded && g_wave > 0 && On("arenareward")) { g_rewarded = true; Guard("row:arena:reward", [] { ArenaReward(); }); }
    g_rewarded = true;
    if (now < g_next) return;
    if (g_next == 0) { g_next = now + 3; return; }
    // build 13: start wave + number of waves (0 = endless)
    int total = (int)Val("arenawaves"), first = std::max(1, (int)Val("arena"));
    if (total > 0 && g_wave >= first + total - 1) {
        if (Row* r = T.Get("arena")) r->on = false;
        T.Toast("ARENA: wygrana! Ukończone fale: " + std::to_string(total));
        g_wave = 0; g_next = 0;
        return;
    }
    g_wave++;
    int cnt = std::min(2 + g_wave, 14);
    std::vector<const NpcEntry*> pool;
    int lo = std::max(0, g_wave * 4 - 8), hi = g_wave * 12 + 10;
    for (auto& e : g_cat.npcs) {
        if (!e.monster || e.id.find("SUMMON") != std::string::npos || e.level <= 0 || T.Crashed("npc:" + e.id)) continue;
        // build 13: test / story-scripted monsters (immortal or invulnerable on purpose)
        if (e.id.find("TEST") != std::string::npos || e.id.find("ROCKEFELLER") != std::string::npos || e.id.find("KEEPER") != std::string::npos ||
            e.id.find("DANCE") != std::string::npos || e.id == "SLEEPER" || e.id.find("XARDAS") != std::string::npos) continue;
        if (e.level >= lo && e.level <= hi) pool.push_back(&e);
    }
    if (pool.empty()) for (auto& e : g_cat.npcs) if (e.monster && e.level > 0 && !T.Crashed("npc:" + e.id)) pool.push_back(&e);
    if (pool.empty()) return;
    for (int i = 0; i < cnt; i++) {
        zVEC3 p = SpawnPointNear(600.f, 1000.f, (float)i / cnt * 6.2831f);
        void* n = nullptr;
        for (int tries = 0; tries < 4 && !n; tries++) {   // build 10: a failed spawn takes another monster (waves came 1-2 short)
            const NpcEntry* e = pool[rng() % pool.size()];
            Guard("npc:" + e->id, [&] { n = SpawnNpc(e->sym, p, true); });
            if (n && (Dist(Pos(n), p) > 400.f || !VobInWorld(n))) EnsureNpcAt(n, p);
        }
        if (n) {
            MakeMortal(n);
            if (Attr(n, 1) > 4000) { SetAttr(n, 1, 4000); SetAttr(n, 0, 4000); }   // a few have absurd HP on purpose
            ArenaName(n);
            Guard("row:arena:aggro", [&] { Aggro(n, Player()); }); g_arena.push_back(n);
        }
    }
    g_next = now + 4; g_rewarded = false;
    char b[96]; snprintf(b, sizeof b, "ARENA: fala %d (%d wrogów)", g_wave, cnt);
    T.Toast(b);
}
}

void MakeMortal(void* n) {
    fld<int>(n, NPC_FLAGS) &= ~2;
    static int protOff = -1; if (protOff < 0) protOff = FieldOff("C_NPC.PROTECTION", 0x1A4);
    for (int k = 0; k < 8; k++) { int& p = fld<int>(n, protOff + 4 * k); if (p < 0 || p >= 999) p = 50; }   // -1 / 999 = immune
}
// every NPC created by the spawner (copies only - originals of story characters are never touched)
std::vector<std::pair<void*, double>> g_spawnedNpcs;
void Act_SpawnNpc(int sym, int count, int mode) {
    Sym* s = SymAt(sym); std::string id = s ? zs(s->name) : "?";
    T.Queue("npc:" + id, [=] {
        if (!InGame()) return;
        int n = std::max(1, std::min(count, 30));
        void* h = Player(); zVEC3 a = AtVec(h);
        float base = std::atan2(a.z, a.x);
        int ok = 0;
        // build 12: the script instance symbol points at the existing character - with "teleport original"
        // a story NPC (Aidan, Diego...) is brought here instead of creating a second one
        if (!T.spawnClone) {
            Sym* is = SymAt(sym);
            void* orig = is ? (void*)(uintptr_t)is->offset : nullptr;
            // build 14: the symbol can point at a deleted NPC (arena wolf removed, far monster deleted by the spawn
            // manager) - that crashed "spawn WOLF". Only a living NPC from the world's list, and only humans (story
            // characters); monsters are always created new.
            if (NpcExists(orig) && IsNpc(orig) && !NpcDead(orig) && !IsMonster(orig) && orig != Player() && orig != MainHero() && NpcInstance(orig) == sym) {
                zVEC3 p = SpawnPointNear(350.f, 450.f, base);
                CallExt("Npc_ClearAIQueue", { Arg::Npc(orig) });
                EnsureNpcAt(orig, p);
                if (mode == 0) Guard("spawnmode:wrogi", [&] { Aggro(orig, h); });
                else if (mode == 1) Guard("spawnmode:sojusznik", [&] { MakeAlly(orig); });
                T.Toast("Przeniesiono oryginał: " + U(NpcName(orig)));
                return;
            }
        }
        for (int i = 0; i < n; i++) {
            float ang = base + (n > 1 ? ((float)i - (n - 1) / 2.f) * 0.3f : 0.f);
            zVEC3 p = SpawnPointNear(350.f + (i / 8) * 150.f, 450.f + (i / 8) * 150.f, ang);
            Sym* is = SymAt(sym);
            int savedOff = is ? is->offset : 0;   // scripts that name e.g. DIEGO must keep the real Diego
            void* npc = SpawnNpc(sym, p, true);
            if (is) is->offset = savedOff;
            if (!npc) continue;
            ok++;
            g_spawnedNpcs.push_back({ npc, T.gameClock });
            Schedule(0.8, [npc, id] {   // still no model -> the engine disabled it; say so instead of leaving a ghost
                if (NpcExists(npc) && !VobModel(npc)) { Log("spawn %s: brak modelu po wstawieniu", id.c_str()); T.Toast(id + ": gra nie wczytała modelu postaci"); }
            });
            MakeMortal(npc);   // script immortality / immune protections (some monsters) - spawned ones can die
            // named npcs get beamed to their daily-routine waypoint on insertion: bring them back
            // build 10: put it back in the same frame (before the spawn manager takes a far NPC out of the world),
            // and re-insert it if the game did that anyway
            if (Dist(Pos(npc), p) > 400.f || !VobInWorld(npc)) EnsureNpcAt(npc, p);
            for (double t : { 0.15, 0.6, 1.5 })
                Schedule(t, [npc, p] { if (NpcExists(npc) && (Dist(Pos(npc), p) > 400.f || !VobInWorld(npc))) EnsureNpcAt(npc, p); });
            if (mode == 0) Guard("spawnmode:wrogi", [&] { Aggro(npc, h); });
            else if (mode == 1) Guard("spawnmode:sojusznik", [&] { MakeAlly(npc); });
            else if (mode == 2) { if (NpcReady(npc)) CallExt("Npc_ClearAIQueue", { Arg::Npc(npc) }); SetSleeping(npc, true); g_frozenSpawn.insert(npc); }
        }
        static const char* mn[] = { "wrogi", "sojusznik", "zamrożony", "neutralny" };
        T.Toast(ok ? "Spawn: " + std::to_string(ok) + "x " + id + " (" + mn[mode & 3] + ")" : "Gra odmówiła stworzenia " + id);
    });
}

void Act_RemoveSpawned(uintptr_t p) {   // 0 = all spawned
    T.Queue("row:spawnremove", [p] {
        int c = 0;
        for (auto it = g_spawnedNpcs.begin(); it != g_spawnedNpcs.end();) {
            if (p && (uintptr_t)it->first != p) { ++it; continue; }
            void* n = it->first;
            if (NpcExists(n) && n != Player() && n != MainHero()) {
                g_allies.erase(std::remove(g_allies.begin(), g_allies.end(), n), g_allies.end());
                g_savedTargets.erase(std::remove(g_savedTargets.begin(), g_savedTargets.end(), n), g_savedTargets.end());
                RemoveVob(n); c++;
            }
            it = g_spawnedNpcs.erase(it);
        }
        T.Toast("Usunięto zrespionych: " + std::to_string(c));
    });
}
void Act_SpawnByName(const std::string& id, int count, int mode) {
    int sym = SymIndex(id);
    if (sym < 0) { T.Toast("Nie ma takiej instancji: " + id); return; }
    Act_SpawnNpc(sym, count, mode);
}
void Act_SaveTargetPtr(uintptr_t p) {
    T.Queue("row:tsave", [p] {
        void* n = (void*)p;
        if (!NpcExists(n)) return;
        auto it = std::find(g_savedTargets.begin(), g_savedTargets.end(), n);
        if (it == g_savedTargets.end()) { g_savedTargets.push_back(n); it = g_savedTargets.end() - 1; }
        g_activeSaved = (int)(it - g_savedTargets.begin());
        T.Toast("Zapisano cel: " + U(NpcName(n)));
    });
}
void Act_MakeAllies(const std::vector<uintptr_t>& v) {
    T.Queue("row:allypick", [v] {
        int c = 0;
        for (uintptr_t p : v) { void* n = (void*)p; if (NpcAlive(n) && n != Player() && n != MainHero()) { MakeAlly(n); c++; } }
        T.Toast(std::to_string(c) + " nowych w drużynie");
    });
}

namespace {
std::vector<NearNpc> g_near; double g_nearAt = -10, g_keepAt = 0;
std::unordered_map<void*, int> g_pstate;   // 0 follows, 1 waits, 2 back at post
// build 9 (#16): "wait" = stay exactly here. The script stand states end after a while and the daily routine
// walks the NPC back to its post, so the spot is remembered and enforced every second.
struct WaitSpot { zVEC3 pos; float trafo[12]; };
std::unordered_map<void*, WaitSpot> g_waitAt;
// build 12: while waiting the daily routine is switched off (that is what walked them away), and restored on
// "back to post" / "leave party" - no more teleporting every second
std::unordered_map<void*, int> g_savedRtn;
int RtnOff() { static int o = FieldOff("C_NPC.DAILY_ROUTINE", -1); return o; }
void RoutineOff(void* n) { int o = RtnOff(); if (o < 0 || g_savedRtn.count(n)) return; g_savedRtn[n] = fld<int>(n, o); fld<int>(n, o) = 0; }
void RoutineOn(void* n) { int o = RtnOff(); auto it = g_savedRtn.find(n); if (o < 0 || it == g_savedRtn.end()) return; if (NpcExists(n)) fld<int>(n, o) = it->second; g_savedRtn.erase(it); }
int StateSym(std::initializer_list<const char*> names) { for (auto n : names) { int i = SymIndex(n); if (i >= 0) return i; } return -1; }
void Wait(void* n) {
    if (!NpcReady(n)) return;
    SetNpcAivar(n, K.aivParty, 0);
    CallExt("Npc_ClearAIQueue", { Arg::Npc(n) });
    CallExt("AI_StandupQuick", { Arg::Npc(n) });
    int st = IsMonster(n) ? StateSym({ "ZS_MM_RTN_REST", "ZS_MM_RTN_ROAM" }) : StateSym({ "ZS_STAND", "ZS_STANDAROUND", "ZS_GUARD" });
    if (st >= 0) CallExt("AI_StartState", { Arg::Npc(n), Arg::Int(st), Arg::Int(1), Arg::Str("") });
}
// back to the daily routine: stop the current action, weapon away, attitude back, walk to where the
// routine says it should be right now
void ToRoutine(void* n) {
    if (!NpcReady(n)) return;
    CallExt("Npc_ClearAIQueue", { Arg::Npc(n) });
    CallExt("AI_StandupQuick", { Arg::Npc(n) });
    CallExt("AI_RemoveWeapon", { Arg::Npc(n) });
    int perm = CallExt("Npc_GetPermAttitude", { Arg::Npc(n), Arg::Npc(Player()) });
    CallExt("Npc_SetTempAttitude", { Arg::Npc(n), Arg::Int(perm) });
    if (IsMonster(n)) {
        int st = K.offStartAi >= 0 ? fld<int>(n, K.offStartAi) : -1;
        if (st <= 0) st = K.stMmAll;
        if (st > 0) CallExt("AI_StartState", { Arg::Npc(n), Arg::Int(st), Arg::Int(1), Arg::Str("") });
    } else CallExt("AI_ContinueRoutine", { Arg::Npc(n) });
}
NearNpc Describe(void* n, void* h) {
    NearNpc e;
    e.ptr = (uintptr_t)n; e.name = U(NpcName(n)); e.inst = InstName(n);
    int g = fld<int>(n, K.offGuild);
    e.guild = GuildName(g); e.group = NpcGroupOfGuild(g); e.hp = Attr(n, 0); e.hpmax = Attr(n, 1);
    e.level = fld<int>(n, K.offLevel); e.dist = Dist(Pos(n), Pos(h)) / 100.f; e.ally = IsAlly(n); e.dead = NpcDead(n); e.monster = IsMonster(n);
    e.hostile = !e.dead && !e.ally && CallExt("Npc_GetAttitude", { Arg::Npc(n), Arg::Npc(h) }) == K.attHostile;
    auto it = g_pstate.find(n); e.pstate = it == g_pstate.end() ? 0 : it->second;
    e.kind = Npcs_Kind(n); e.frozen = Target_IsFrozen(n) || g_frozenSpawn.count(n);
    return e;
}
void RefreshNearby(void* h) {
    if (NowSec() - g_nearAt < 0.5) return;
    g_nearAt = NowSec();
    g_near.clear();
    zVEC3 hp = Pos(h);
    float r = std::max(5.f, Val("allyradius")) * 100.f;
    ForEachNpc([&](void* n) {
        if (n == h || n == MainHero() || IsAlly(n)) return;
        if (Dist(Pos(n), hp) > r) return;
        g_near.push_back(Describe(n, h));
    });
    std::sort(g_near.begin(), g_near.end(), [](const NearNpc& a, const NearNpc& b) { return a.dist < b.dist; });
}
// allies that wandered off into their routine/dialog get put back into follow mode
// build 13: never touch the party while a dialogue is open - restarting their state inside a dialogue locked it
bool InDialog() { bool ok = false; int r = CallExt("InfoManager_HasFinished", {}, &ok); return ok && r == 0; }
void KeepParty() {
    if (T.gameClock - g_keepAt < 1.0) return;   // build 12: 1 s - after a dialogue they walked back to their routine
    g_keepAt = T.gameClock;
    if (InDialog()) return;
    g_allies.erase(std::remove_if(g_allies.begin(), g_allies.end(), [](void* n) { return !NpcAlive(n); }), g_allies.end());
    for (void* n : g_allies) {
        if (g_pstate.count(n) && g_pstate[n] != 0) continue;   // waiting / at post on purpose
        if (void* h = Player()) {   // left far behind (ladder, teleport, routine) -> put it right behind you
            zVEC3 hp = Pos(h);
            if (Dist(Pos(n), hp) > Val("allykeepdist") * 100.f || !VobInWorld(n)) {
                const float* t = &fld<float>(h, VOB_TRAFO);
                zVEC3 b = hp; b.x -= t[2] * 250.f; b.z -= t[10] * 250.f;
                bool wasReady = NpcReady(n) && VobInWorld(n);
                if (wasReady && On("allyfightbreak")) CallExt("Npc_ClearAIQueue", { Arg::Npc(n) });
                EnsureNpcAt(n, b);   // build 10: far allies are taken out of the world by the game - bring the real one back
                // build 14: break the fight for monsters too (bloodfly flew back to its opponent): new target = you,
                // follow state restarted, and for 8 s it does not notice anyone else (the old opponent included)
                if (On("allyfightbreak")) Party_BreakFight(n);
            }
        }
        if (!NpcReady(n) || !VobInWorld(n)) continue;
        if (IsMonster(n)) {   // summoned-like monsters keep forgetting they are friends
            CallExt("Npc_SetTempAttitude", { Arg::Npc(n), Arg::Int(K.attFriendly) });
            // build 13: wolves & co. walked behind you - run mode when they fall behind (at most every 4 s)
            static std::unordered_map<void*, double> runAt;
            if (Player() && Dist(Pos(n), Pos(Player())) > 600.f && T.gameClock - runAt[n] > 4.0) {
                runAt[n] = T.gameClock;
                CallExt("AI_SetWalkMode", { Arg::Npc(n), Arg::Int(ConstInt("NPC_RUN", 0)) });
            }
            continue;
        }
        if (K.stFollow < 0) continue;
        SetNpcAivar(n, K.aivParty, 1);   // dialogues reset it - re-assert every check
        if (CallExt("Npc_IsInState", { Arg::Npc(n), Arg::Int(K.stFollow) })) continue;
        if (K.stAttack >= 0 && CallExt("Npc_IsInState", { Arg::Npc(n), Arg::Int(K.stAttack) })) continue;
        SetNpcAivar(n, K.aivParty, 1);
        CallExt("AI_StartState", { Arg::Npc(n), Arg::Int(K.stFollow), Arg::Int(1), Arg::Str("") });
    }
}
}

// ---- build 14: the party never fights itself; neutral people leave a party monster alone ----------------
std::unordered_map<void*, double> g_calmUntil;      // ally -> ignores everyone but you until then (fight break)
std::unordered_set<void*> g_hostile;                 // npcs hostile to you (refreshed every 0.5 s while you have a party)
std::unordered_map<void*, double> g_aggr;            // who hit you or a party member recently
std::unordered_map<void*, double> g_calmAt;
bool PartyEnemy(void* n) {
    if (!n) return false;
    if (g_hostile.count(n) || Arena_IsEnemy(n)) return true;
    auto it = g_aggr.find(n); return it != g_aggr.end() && T.gameClock - it->second < 20.0;
}
void Party_BreakFight(void* n) {
    g_calmUntil[n] = T.gameClock + 8.0;
    Schedule(0.05, [n] {
        if (!NpcAlive(n) || !NpcReady(n)) return;
        void* h = Player();
        CallExt("Npc_ClearAIQueue", { Arg::Npc(n) });
        if (h) CallExt("Npc_SetTarget", { Arg::Npc(n), Arg::Npc(h) });
        CallExt("Npc_SetTempAttitude", { Arg::Npc(n), Arg::Int(K.attFriendly) });
        if (IsMonster(n)) { if (K.stSummoned >= 0) CallExt("AI_StartState", { Arg::Npc(n), Arg::Int(K.stSummoned), Arg::Int(1), Arg::Str("") }); }
        else if (K.stFollow >= 0) CallExt("AI_StartState", { Arg::Npc(n), Arg::Int(K.stFollow), Arg::Int(1), Arg::Str("") });
    });
}
static void Calm(void* n) {
    auto& t = g_calmAt[n];
    if (T.gameClock - t < 0.5) return;
    t = T.gameClock;
    if (IsAlly(n)) { Party_BreakFight(n); return; }
    Schedule(0.0, [n] { if (NpcAlive(n) && NpcReady(n)) Peace(n); });
}
bool Party_BlockDamage(void* victim, void* att) {
    if (!IsNpc(att) || !IsNpc(victim) || att == victim || g_allies.empty()) return false;
    void* h = Player(); void* mh = MainHero();
    bool vA = IsAlly(victim), aA = IsAlly(att), vHero = victim == h || victim == mh, aHero = att == h || att == mh;
    if (aHero) return false;                                      // your own hits always land
    if (vA && aA) { Calm(att); return true; }                     // party members never fight each other
    if (aA && !vHero && !PartyEnemy(victim)) { Calm(att); return true; }   // your wolf does not bite neutral people
    if (vA && !aA && !PartyEnemy(att)) { Calm(att); return true; }        // neutral people leave your party alone
    if ((vA || vHero) && !aA) g_aggr[att] = T.gameClock;          // whoever attacks you or the party is fair game
    return false;
}
bool Party_IgnoreSense(void* self, void* vob) {
    if (g_allies.empty() || !self || !vob || vob == self) return false;
    void* h = Player();
    if (vob == h || vob == MainHero()) return false;
    if (!IsNpc(vob)) return false;
    bool sA = IsAlly(self), vA = IsAlly(vob);
    if (sA) {
        auto it = g_calmUntil.find(self);
        if (it != g_calmUntil.end() && T.gameClock < it->second) return true;   // fight break: sees only you
        if (vA) return true;                                                     // party members do not notice each other
        if (IsMonster(self) && !PartyEnemy(vob)) return true;                    // party monster ignores neutral people
        return false;
    }
    if (vA && IsMonster(vob) && self != h && !PartyEnemy(self)) return true;    // neutral people ignore your monster
    return false;
}
void Party_AllyHitPlayer(void* n) {
    static double last = 0;
    if (T.gameClock - last < 0.5) return;
    last = T.gameClock;
    Schedule(0.0, [n] {
        if (!NpcAlive(n) || !NpcReady(n)) return;
        CallExt("Npc_ClearAIQueue", { Arg::Npc(n) });
        CallExt("Npc_SetTempAttitude", { Arg::Npc(n), Arg::Int(K.attFriendly) });
        CallExt("Npc_SetAttitude", { Arg::Npc(n), Arg::Int(K.attFriendly) });
        auto it = g_pstate.find(n);
        if (it == g_pstate.end() || it->second == 0) MakeAlly(n); else Wait(n);
    });
}
void Act_EnvCmd(uintptr_t p, int cmd) {
    static const char* rowFor[] = { nullptr, "tpto", "tpull", nullptr, nullptr, nullptr, "tfreeze", "tkill", "trevive", "tremove" };
    T.Queue("env:" + std::to_string(cmd), [p, cmd] {
        void* n = (void*)p;
        if (!NpcExists(n)) { T.Toast("Tej postaci już nie ma"); return; }
        if (cmd >= 0 && cmd < 10 && rowFor[cmd]) {   // the Cel actions, run on this NPC instead of the target
            if (Row* r = T.Get(rowFor[cmd])) if (r->act) { g_targetOverride = n; Guard("env:" + r->id, [&] { r->act(*r); }); g_targetOverride = nullptr; }
            tst::Event("env", std::string(rowFor[cmd]) + " na " + InstName(n));
            return;
        }
        switch (cmd) {
        case 0: Act_SaveTargetPtr(p); break;
        case 3: if (IsAlly(n)) { Act_PartyCmd(p, 5); } else if (!NpcDead(n)) { g_pstate[n] = 0; MakeAlly(n); T.Toast(U(NpcName(n)) + " dołącza do drużyny"); } break;
        case 4: Peace(n); CallExt("Npc_SetAttitude", { Arg::Npc(n), Arg::Int(K.attNeutral) }); CallExt("Npc_SetTempAttitude", { Arg::Npc(n), Arg::Int(K.attNeutral) });
                T.Toast(U(NpcName(n)) + ": spokój"); break;
        case 5: if (void* h = Player()) { Aggro(n, h); T.Toast(U(NpcName(n)) + " atakuje ciebie"); } break;
        }
        tst::Event("env", "akcja " + std::to_string(cmd) + " na " + InstName(n));
    });
}
void Act_PartyCmd(uintptr_t p, int cmd) {
    T.Queue("row:party", [p, cmd] {
        void* n = (void*)p;
        if (!NpcExists(n)) return;
        void* h = Player();
        if (cmd != 1) g_waitAt.erase(n);
        switch (cmd) {
        case 0: g_pstate[n] = 0;
            if (NpcReady(n)) { CallExt("Npc_ClearAIQueue", { Arg::Npc(n) }); CallExt("AI_StandupQuick", { Arg::Npc(n) }); }
            MakeAlly(n); T.Toast(U(NpcName(n)) + " idzie za tobą"); break;
        case 1: {
            g_pstate[n] = 1; RoutineOff(n); Wait(n);
            if (NpcReady(n)) CallExt("AI_Wait", { Arg::Npc(n), Arg::Flt(100000.f) });
            WaitSpot w; w.pos = Pos(n); const float* t = &fld<float>(n, VOB_TRAFO); std::copy(t, t + 12, w.trafo);
            g_waitAt[n] = w;
            T.Toast(U(NpcName(n)) + " czeka tutaj"); break;
        }
        case 2: g_pstate[n] = 2; RoutineOn(n); SetNpcAivar(n, K.aivParty, 0); g_allies.erase(std::remove(g_allies.begin(), g_allies.end(), n), g_allies.end());
                ToRoutine(n); g_allies.push_back(n); T.Toast(U(NpcName(n)) + " wraca do swojej rutyny"); break;
        case 3: EnsureNpcAt(n, Grounded(InFront(180.f))); g_pstate[n] = 0; MakeAlly(n); T.Toast("Przywołano: " + U(NpcName(n))); break;
        case 4: { zVEC3 q = Pos(n); SafeSetPos(h, { q.x, q.y + 60, q.z }); } break;
        case 5: RoutineOn(n); g_pstate.erase(n); g_allies.erase(std::remove(g_allies.begin(), g_allies.end(), n), g_allies.end());
                SetNpcAivar(n, K.aivParty, 0); ToRoutine(n); T.Toast(U(NpcName(n)) + " opuszcza drużynę"); break;
        }
    });
}

// build 11: oCSpawnManager::CheckRemoveNpc (0x6D0A80, from GD3D11's G1 1.08k map) takes NPCs far from the player
// out of the world. Party members, our spawned NPCs and your own body while you control someone else stay in.
bool Target_KeepBody(void* npc);
using fnCheckRemove = int(__fastcall*)(void*, void*, void*);
fnCheckRemove oCheckRemove = nullptr;
int __fastcall hkCheckRemove(void* self, void* edx, void* npc) {
    if (npc) {
        if (std::find(g_allies.begin(), g_allies.end(), npc) != g_allies.end()) return 0;
        for (auto& e : g_spawnedNpcs) if (e.first == npc) return 0;
        if (Target_KeepBody(npc)) return 0;
    }
    return oCheckRemove(self, edx, npc);
}
// build 12: "insert <npc>" typed in the game's console (or run from our console) lands in the spawned list
using fnConsole = int(__cdecl*)(const zSTRING*, zSTRING*);
fnConsole oConsole = nullptr;
int __cdecl hkConsole(const zSTRING* in, zSTRING* out) {
    std::string cmd = in && in->ptr ? std::string(in->ptr, in->len) : std::string();
    for (auto& c : cmd) c = (char)tolower((unsigned char)c);
    if (cmd.rfind("insert", 0) != 0) return oConsole(in, out);
    std::set<void*> before; ForEachNpc([&](void* n) { before.insert(n); });
    int r = oConsole(in, out);
    ForEachNpc([&](void* n) { if (!before.count(n) && n != Player()) g_spawnedNpcs.push_back({ n, T.gameClock }); });
    RefreshNpcSet();
    return r;
}
void Npcs_Hooks() {
    MH_CreateHook((LPVOID)0x6D0A80, (LPVOID)&hkCheckRemove, (LPVOID*)&oCheckRemove);
    MH_CreateHook((LPVOID)F_CONSOLE_EVAL, (LPVOID)&hkConsole, (LPVOID*)&oConsole);
}
// build 12: changing the hour re-applies daily routines (party members and spawned NPCs walked off).
// Called around Wld_SetTime: remember where they stand, put them back right after.
std::vector<std::pair<void*, zVEC3>> g_timeKeep;
void Npcs_BeforeTimeJump() {
    g_timeKeep.clear();
    for (void* n : g_allies) if (NpcExists(n)) g_timeKeep.push_back({ n, Pos(n) });
    for (auto& e : g_spawnedNpcs) if (NpcExists(e.first)) g_timeKeep.push_back({ e.first, Pos(e.first) });
}
void Npcs_AfterTimeJump() {
    for (double t : { 0.05, 0.5, 1.5 })
        Schedule(t, [] {
            for (auto& k : g_timeKeep) {
                void* n = k.first;
                if (!NpcAlive(n)) continue;
                if (Dist(Pos(n), k.second) > 300.f || !VobInWorld(n)) { if (NpcReady(n)) CallExt("Npc_ClearAIQueue", { Arg::Npc(n) }); EnsureNpcAt(n, k.second); }
                auto ps = g_pstate.find(n);
                bool ally = std::find(g_allies.begin(), g_allies.end(), n) != g_allies.end();
                if (ally && (ps == g_pstate.end() || ps->second == 0)) MakeAlly(n);
            }
        });
}
// an arena enemy brought you to 0 HP (the hit was clamped to 1 HP) or you died during the arena
void Arena_PlayerDown() {
    if (g_lost || !On("arena")) return;
    g_lost = true;
    T.Queue("row:arena", [] {
        int w = g_wave;
        ArenaClear();
        if (Row* r = T.Get("arena")) r->on = false;
        g_wave = 0; g_next = 0; g_lost = false;
        T.Toast("ARENA: przegrana na fali " + std::to_string(w) + " - przeciwnicy usunięci");
    });
}
// build 13: what kind of character this is (ESP "typ")
std::string Npcs_Kind(void* n) {
    if (Arena_IsEnemy(n)) return "Arena";
    if (std::find(g_allies.begin(), g_allies.end(), n) != g_allies.end()) return "Drużyna";
    for (auto& e : g_spawnedNpcs) if (e.first == n) return "Zrespiony";
    return "";
}
bool Arena_IsEnemy(void* n) { return n && std::find(g_arena.begin(), g_arena.end(), n) != g_arena.end(); }

void Npcs_World() { g_savedRtn.clear(); g_spawnedNpcs.clear(); g_waitAt.clear(); g_lost = false; g_rewarded = true; g_arena.clear(); g_frozenSpawn.clear(); g_wave = 0; g_next = 0; g_near.clear(); g_nearAt = -10; g_pstate.clear(); }
void Npcs_Tick(TickCtx& c) {
    if (T.menuOpen) RefreshNearby(c.hero);
    c.s.nearby = g_near;
    if (T.menuOpen) for (void* n : g_allies) if (NpcExists(n)) c.s.party.push_back(Describe(n, c.hero));
    g_spawnedNpcs.erase(std::remove_if(g_spawnedNpcs.begin(), g_spawnedNpcs.end(), [](const std::pair<void*, double>& e) { return !NpcExists(e.first); }), g_spawnedNpcs.end());
    if (T.menuOpen) for (auto& e : g_spawnedNpcs) c.s.spawned.push_back(Describe(e.first, c.hero));
    // build 14: who is hostile to you (party fights only them); party members that vanished are logged
    static double hostAt = 0;
    if (!g_allies.empty() && !c.paused && T.gameClock - hostAt > 0.5) {
        hostAt = T.gameClock;
        std::unordered_set<void*> hs; void* hh = c.hero;
        NpcsInRadius(6000.f, [&](void* n) { if (!IsAlly(n) && !NpcDead(n) && CallExt("Npc_GetAttitude", { Arg::Npc(n), Arg::Npc(hh) }) == K.attHostile) hs.insert(n); });
        g_hostile.swap(hs);
        for (auto it = g_aggr.begin(); it != g_aggr.end();) { if (T.gameClock - it->second > 30.0 || !NpcExists(it->first)) it = g_aggr.erase(it); else ++it; }
        for (auto it = g_calmUntil.begin(); it != g_calmUntil.end();) { if (T.gameClock > it->second) it = g_calmUntil.erase(it); else ++it; }
        static int sumAiv[4] = { -2, -2, -2, -2 };   // summoned-monster timers (names differ between script versions)
        if (sumAiv[0] == -2) { const char* nm[] = { "AIV_MM_SUMMONTIME", "AIV_SUMMONTIME", "AIV_MM_SUMMONEDTIME", "AIV_SUMMONEDTIME" }; for (int i = 0; i < 4; i++) sumAiv[i] = ConstInt(nm[i], -1); }
        for (void* n : g_allies) if (NpcExists(n) && IsMonster(n)) for (int a : sumAiv) if (a >= 0) SetNpcAivar(n, a, 0);
    }
    {   // log party members that disappeared (#build 14: wolf vanished when you flew far away)
        static std::unordered_map<void*, std::string> known;
        for (auto it = known.begin(); it != known.end();) {
            if (!IsAlly(it->first)) { it = known.erase(it); continue; }
            if (!NpcExists(it->first)) { Log("druzyna: %s znikna z listy postaci (usuniety przez gre)", it->second.c_str()); tst::Event("party", "zniknął: " + it->second); it = known.erase(it); continue; }
            if (NpcDead(it->first)) { Log("druzyna: %s zginal", it->second.c_str()); tst::Event("party", "zginął: " + it->second); it = known.erase(it); continue; }
            ++it;
        }
        for (void* n : g_allies) if (NpcAlive(n) && !known.count(n)) known[n] = InstName(n);
    }
    if (On("allykeep") && !c.paused) Guard("row:allykeep", [] { KeepParty(); });
    {   // build 14: Otoczenie - everybody around (party and dead included), only while that page is open
        static std::vector<NearNpc> env; static double envAt = -10;
        if (T.menuOpen && T.envWanted && NowSec() - envAt > 0.5) {
            envAt = NowSec(); env.clear();
            zVEC3 hp = Pos(c.hero); float r = std::max(5.f, Val("envradius")) * 100.f;
            std::vector<std::pair<float, void*>> v;
            ForEachNpc([&](void* n) { if (n != c.hero && n != MainHero()) { float d = Dist(Pos(n), hp); if (d <= r) v.push_back({ d, n }); } });
            std::sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.first < b.first; });
            if (v.size() > 80) v.resize(80);
            for (auto& e : v) env.push_back(Describe(e.second, c.hero));
        }
        T.envWanted = false;
        if (T.menuOpen) c.s.env = env;
    }
    // build 13: "wait" holds the exact spot. Routine is off; every 0.5 s: not fighting and moved > 1.2 m -> back
    // to the spot and facing; nothing queued -> a long AI_Wait. Never during dialogues, never frozen (can fight).
    static double waitCheck = 0;
    if (!c.paused && !g_waitAt.empty() && T.gameClock - waitCheck > 0.5 && !InDialog()) {
        waitCheck = T.gameClock;
        for (auto it = g_waitAt.begin(); it != g_waitAt.end();) {
            void* n = it->first;
            if (!NpcAlive(n) || !g_pstate.count(n) || g_pstate[n] != 1) { it = g_waitAt.erase(it); continue; }
            bool fighting = K.stAttack >= 0 && NpcReady(n) && CallExt("Npc_IsInState", { Arg::Npc(n), Arg::Int(K.stAttack) });
            if (!fighting && (Dist(Pos(n), it->second.pos) > 120.f || !VobInWorld(n))) {
                if (NpcReady(n) && VobInWorld(n)) CallExt("Npc_ClearAIQueue", { Arg::Npc(n) });
                EnsureNpcAt(n, it->second.pos);
                float* t = &fld<float>(n, VOB_TRAFO);
                for (int i = 0; i < 12; i++) if (i != 3 && i != 7 && i != 11) t[i] = it->second.trafo[i];
                if (NpcReady(n)) { CallExt("AI_StandupQuick", { Arg::Npc(n) }); CallExt("AI_Wait", { Arg::Npc(n), Arg::Flt(100000.f) }); }
            }
            ++it;
        }
    }
    for (auto it = g_frozenSpawn.begin(); it != g_frozenSpawn.end();) {
        if (!NpcExists(*it)) { it = g_frozenSpawn.erase(it); continue; }
        if ((fld<uint8_t>(*it, VOB_SLEEPMODE) & 3) != 0) SetSleeping(*it, true);
        ++it;
    }
    if (On("arena")) {
        if (g_wave == 0 && g_next == 0) g_wave = std::max(0, (int)Val("arena") - 1);
        if (Player() && NpcDead(Player())) Arena_PlayerDown();
        else if (!c.paused) ArenaFrame();
    } else {
        if (!g_arena.empty()) {   // arena switched off: its monsters disappear
            ArenaClear();
            T.Toast("Arena zakończona - przeciwnicy usunięci");
        }
        g_wave = 0; g_next = 0; g_arena.clear();
    }
    c.s.arena = On("arena"); c.s.arenaWave = g_wave; c.s.arenaAlive = (int)g_arena.size();
}

void Npcs_Register() {
    const char* C = "Postacie";
    CardTitle("Arena");
    T.Add(MkRow("arena", "Tryb Areny", "fala startowa; wrogowie 6-10 m od ciebie, tylko powalają - powalenie = przegrana", "Fun", "Walka", K_SLIDER, 1, 1, 20));
    St(ST_NEW, "build 7: powalenie zamiast śmierci, przegrana usuwa wrogów"); Icon(ICN_CROSSHAIR);
    T.Add(MkRow("arenawaves", "Liczba fal", "0 = bez końca; suwak areny = od której fali zaczynasz", "Fun", "Walka", K_VALUE, 0, 0, 50)); St(ST_NEW, "nowe w build 13");
    T.Add(MkRow("arenareward", "Nagrody po fali", "ruda + 1-2 losowe przedmioty", "Fun", "Walka", K_TOGGLE)); T.rows.back().on = true; St(ST_NEW, "nowe w build 7");
    const char* D = "Drużyna";
    CardTitle("Drużyna");
    T.Add(MkRow("allyall", "Wszyscy w promieniu do drużyny", "metry, idą za tobą", C, D, K_ACTION, 10, 1, 100, [](Row& r) {
        int c = 0; NpcsInRadius(r.val * 100.f, [&](void* n) { if (!NpcDead(n) && n != MainHero()) { MakeAlly(n); c++; } });
        T.Toast(std::to_string(c) + " nowych sojuszników"); }));
    St(ST_OK, "build 3: działa (ale postacie z dialogiem odchodziły)"); Icon(ICN_USERS);
    T.Add(MkRow("dismiss", "Rozwiąż drużynę", "sojusznicy wracają do swoich zajęć", C, D, K_ACTION, 0, 0, 0, [](Row&) {
        std::vector<void*> a = g_allies; g_allies.clear(); g_pstate.clear();
        for (void* n : a) if (NpcAlive(n)) { SetNpcAivar(n, K.aivParty, 0); ToRoutine(n); }
        T.Toast("Drużyna rozwiązana (" + std::to_string(a.size()) + ")"); }));
    NoVal(); St(ST_OK, "build 3: działa"); Icon(ICN_TIMES);
    CardTitle("Zachowanie drużyny");
    T.Add(MkRow("allynotalk", "Bez zaczepiania", "drużyna nie zaczyna z tobą dialogów (to je wyrywało z drużyny)", C, D, K_TOGGLE));
    T.rows.back().on = true; St(ST_NEW, "nowe w build 4");
    T.Add(MkRow("allykeep", "Pilnuj drużyny", "co 3 s kto wypadł z podążania wraca za tobą; kto jest za daleko, zostaje przeniesiony", C, D, K_TOGGLE));
    T.rows.back().on = true; St(ST_NEW, "build 7: przenosi tych, co zostali daleko");
    T.Add(MkRow("allyfightbreak", "Po teleporcie przerywa walkę", "towarzysz przeniesiony do ciebie idzie za tobą zamiast wracać do przeciwnika", C, D, K_TOGGLE));
    T.rows.back().on = true; St(ST_NEW, "nowe w build 13");
    T.Add(MkRow("allykeepdist", "Przenieś z odległości", "m", C, D, K_VALUE, 30, 10, 150)); St(ST_NEW, "nowe w build 7");
    CardTitle("Otoczenie");
    T.Add(MkRow("envradius", "Promień", "m - kogo pokazuje lista Otoczenie", C, "Otoczenie", K_VALUE, 30, 5, 150)); St(ST_NEW, "nowe w build 14");
    CardTitle("Zachowanie drużyny");
    T.Add(MkRow("allyradius", "Promień listy wyboru", "m", C, D, K_VALUE, 30, 5, 150)); St(ST_NEW, "nowe w build 4");
}
