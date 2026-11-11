// NPC module: spawner, arena waves, mass ally/kill.
#include "mods.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <unordered_map>

using namespace g1;
using namespace gx;

namespace {
std::vector<void*> g_arena;
std::set<void*> g_frozenSpawn;
int g_wave = 0; double g_next = 0;

void ArenaFrame() {
    double now = NowSec();
    g_arena.erase(std::remove_if(g_arena.begin(), g_arena.end(), [](void* n) { return !NpcAlive(n); }), g_arena.end());
    static double lastAggro = 0;
    if (now - lastAggro > 3.0) { lastAggro = now; for (void* n : g_arena) CallExt("Npc_SetTarget", { Arg::Npc(n), Arg::Npc(Player()) }); }
    if (!g_arena.empty() || now < g_next) return;
    if (g_next == 0) { g_next = now + 3; return; }
    g_wave++;
    int cnt = std::min(2 + g_wave, 14);
    std::vector<const NpcEntry*> pool;
    int lo = std::max(0, g_wave * 4 - 8), hi = g_wave * 12 + 10;
    for (auto& e : g_cat.npcs) {
        if (!e.monster || e.id.find("SUMMON") != std::string::npos || e.level <= 0 || T.Crashed("npc:" + e.id)) continue;
        if (e.level >= lo && e.level <= hi) pool.push_back(&e);
    }
    if (pool.empty()) for (auto& e : g_cat.npcs) if (e.monster && e.level > 0 && !T.Crashed("npc:" + e.id)) pool.push_back(&e);
    if (pool.empty()) return;
    for (int i = 0; i < cnt; i++) {
        const NpcEntry* e = pool[rng() % pool.size()];
        zVEC3 p = SpawnPointNear(600.f, 1000.f, (float)i / cnt * 6.2831f);
        void* n = nullptr;
        Guard("npc:" + e->id, [&] { n = SpawnNpc(e->sym, p); });
        if (n) { MakeMortal(n); Guard("row:arena:aggro", [&] { Aggro(n, Player()); }); g_arena.push_back(n); }
    }
    g_next = now + 4;
    char b[96]; snprintf(b, sizeof b, "ARENA: fala %d (%d wrogów)", g_wave, cnt);
    T.Toast(b);
}
}

void MakeMortal(void* n) {
    fld<int>(n, NPC_FLAGS) &= ~2;
    static int protOff = -1; if (protOff < 0) protOff = FieldOff("C_NPC.PROTECTION", 0x1A4);
    for (int k = 0; k < 8; k++) { int& p = fld<int>(n, protOff + 4 * k); if (p < 0 || p >= 999) p = 50; }   // -1 / 999 = immune
}
void Act_SpawnNpc(int sym, int count, int mode) {
    Sym* s = SymAt(sym); std::string id = s ? zs(s->name) : "?";
    T.Queue("npc:" + id, [=] {
        if (!InGame()) return;
        int n = std::max(1, std::min(count, 30));
        void* h = Player(); zVEC3 a = AtVec(h);
        float base = std::atan2(a.z, a.x);
        int ok = 0;
        for (int i = 0; i < n; i++) {
            float ang = base + (n > 1 ? ((float)i - (n - 1) / 2.f) * 0.3f : 0.f);
            zVEC3 p = SpawnPointNear(350.f + (i / 8) * 150.f, 450.f + (i / 8) * 150.f, ang);
            void* npc = SpawnNpc(sym, p);
            if (!npc) continue;
            ok++;
            MakeMortal(npc);   // script immortality / immune protections (some monsters) - spawned ones can die
            // named npcs get beamed to their daily-routine waypoint on insertion: bring them back
            for (double t : { 0.15, 0.6, 1.5 })
                Schedule(t, [npc, p] { if (NpcExists(npc) && Dist(Pos(npc), p) > 400.f) SafeSetPos(npc, p); });
            if (mode == 0) Guard("spawnmode:wrogi", [&] { Aggro(npc, h); });
            else if (mode == 1) Guard("spawnmode:sojusznik", [&] { MakeAlly(npc); });
            else if (mode == 2) { if (NpcReady(npc)) CallExt("Npc_ClearAIQueue", { Arg::Npc(npc) }); SetSleeping(npc, true); g_frozenSpawn.insert(npc); }
        }
        static const char* mn[] = { "wrogi", "sojusznik", "zamrożony", "neutralny" };
        T.Toast(ok ? "Spawn: " + std::to_string(ok) + "x " + id + " (" + mn[mode & 3] + ")" : "Gra odmówiła stworzenia " + id);
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
void KeepParty() {
    if (T.gameClock - g_keepAt < 3.0) return;
    g_keepAt = T.gameClock;
    g_allies.erase(std::remove_if(g_allies.begin(), g_allies.end(), [](void* n) { return !NpcAlive(n); }), g_allies.end());
    for (void* n : g_allies) {
        if (g_pstate.count(n) && g_pstate[n] != 0) continue;   // waiting / at post on purpose
        if (!NpcReady(n)) continue;
        if (IsMonster(n)) {   // summoned-like monsters keep forgetting they are friends
            CallExt("Npc_SetTempAttitude", { Arg::Npc(n), Arg::Int(K.attFriendly) });
            continue;
        }
        if (K.stFollow < 0) continue;
        if (CallExt("Npc_IsInState", { Arg::Npc(n), Arg::Int(K.stFollow) })) continue;
        if (K.stAttack >= 0 && CallExt("Npc_IsInState", { Arg::Npc(n), Arg::Int(K.stAttack) })) continue;
        SetNpcAivar(n, K.aivParty, 1);
        CallExt("AI_StartState", { Arg::Npc(n), Arg::Int(K.stFollow), Arg::Int(1), Arg::Str("") });
    }
}
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
void Act_PartyCmd(uintptr_t p, int cmd) {
    T.Queue("row:party", [p, cmd] {
        void* n = (void*)p;
        if (!NpcExists(n)) return;
        void* h = Player();
        switch (cmd) {
        case 0: g_pstate[n] = 0;
            if (NpcReady(n)) { CallExt("Npc_ClearAIQueue", { Arg::Npc(n) }); CallExt("AI_StandupQuick", { Arg::Npc(n) }); }
            MakeAlly(n); T.Toast(U(NpcName(n)) + " idzie za tobą"); break;
        case 1: g_pstate[n] = 1; Wait(n); T.Toast(U(NpcName(n)) + " czeka tutaj"); break;
        case 2: g_pstate[n] = 2; SetNpcAivar(n, K.aivParty, 0); g_allies.erase(std::remove(g_allies.begin(), g_allies.end(), n), g_allies.end());
                ToRoutine(n); g_allies.push_back(n); T.Toast(U(NpcName(n)) + " wraca do swojej rutyny"); break;
        case 3: SafeSetPos(n, Grounded(InFront(180.f))); g_pstate[n] = 0; MakeAlly(n); T.Toast("Przywołano: " + U(NpcName(n))); break;
        case 4: { zVEC3 q = Pos(n); SafeSetPos(h, { q.x, q.y + 60, q.z }); } break;
        case 5: g_pstate.erase(n); g_allies.erase(std::remove(g_allies.begin(), g_allies.end(), n), g_allies.end());
                SetNpcAivar(n, K.aivParty, 0); ToRoutine(n); T.Toast(U(NpcName(n)) + " opuszcza drużynę"); break;
        }
    });
}

void Npcs_Hooks() {}
void Npcs_World() { g_arena.clear(); g_frozenSpawn.clear(); g_wave = 0; g_next = 0; g_near.clear(); g_nearAt = -10; g_pstate.clear(); }
void Npcs_Tick(TickCtx& c) {
    if (T.menuOpen) RefreshNearby(c.hero);
    c.s.nearby = g_near;
    if (T.menuOpen) for (void* n : g_allies) if (NpcExists(n)) c.s.party.push_back(Describe(n, c.hero));
    if (On("allykeep") && !c.paused) Guard("row:allykeep", [] { KeepParty(); });
    for (auto it = g_frozenSpawn.begin(); it != g_frozenSpawn.end();) {
        if (!NpcExists(*it)) { it = g_frozenSpawn.erase(it); continue; }
        if ((fld<uint8_t>(*it, VOB_SLEEPMODE) & 3) != 0) SetSleeping(*it, true);
        ++it;
    }
    if (On("arena")) { if (g_wave == 0 && g_next == 0) g_wave = std::max(0, (int)Val("arena") - 1); if (!c.paused) ArenaFrame(); }
    else {
        if (!g_arena.empty()) {   // arena switched off: its monsters disappear
            for (void* n : g_arena) if (NpcExists(n)) RemoveVob(n);
            T.Toast("Arena zakończona - przeciwnicy usunięci");
        }
        g_wave = 0; g_next = 0; g_arena.clear();
    }
    c.s.arena = On("arena"); c.s.arenaWave = g_wave; c.s.arenaAlive = (int)g_arena.size();
}

void Npcs_Register() {
    const char* C = "Postacie";
    CardTitle("Arena");
    T.Add(MkRow("arena", "Tryb Areny", "fale wrogów 6-10 m od ciebie, na lądzie; liczba = fala startowa", C, "Spawner", K_SLIDER, 1, 1, 20));
    St(ST_NEW, "build 3: CRASH (atak na postać bez załadowanego modelu); build 4: atak dopiero, gdy wróg jest gotowy"); Icon(ICN_CROSSHAIR);
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
    T.Add(MkRow("allykeep", "Pilnuj drużyny", "co 3 s kto wypadł z podążania, wraca za tobą", C, D, K_TOGGLE));
    T.rows.back().on = true; St(ST_NEW, "nowe w build 4");
    T.Add(MkRow("allyradius", "Promień listy wyboru", "m", C, D, K_VALUE, 30, 5, 150)); St(ST_NEW, "nowe w build 4");
}
