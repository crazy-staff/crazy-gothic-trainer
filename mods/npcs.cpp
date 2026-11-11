// NPC module: spawner, arena waves, mass ally/kill.
#include "mods.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

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
        if (n) { Guard("row:arena:aggro", [&] { Aggro(n, Player()); }); g_arena.push_back(n); }
    }
    g_next = now + 4;
    char b[96]; snprintf(b, sizeof b, "ARENA: fala %d (%d wrogów)", g_wave, cnt);
    T.Toast(b);
}
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
            if (mode == 0) Guard("spawnmode:wrogi", [&] { Aggro(npc, h); });
            else if (mode == 1) Guard("spawnmode:sojusznik", [&] { MakeAlly(npc); });
            else if (mode == 2) { if (NpcReady(npc)) CallExt("Npc_ClearAIQueue", { Arg::Npc(npc) }); SetSleeping(npc, true); g_frozenSpawn.insert(npc); }
        }
        static const char* mn[] = { "wrogi", "sojusznik", "zamrożony", "neutralny" };
        T.Toast("Spawn: " + std::to_string(ok) + "x " + id + " (" + mn[mode & 3] + ")");
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
void RefreshNearby(void* h) {
    if (NowSec() - g_nearAt < 0.5) return;
    g_nearAt = NowSec();
    g_near.clear();
    zVEC3 hp = Pos(h);
    float r = std::max(5.f, Val("allyradius")) * 100.f;
    ForEachNpc([&](void* n) {
        if (n == h || n == MainHero()) return;
        float d = Dist(Pos(n), hp);
        if (d > r) return;
        NearNpc e;
        e.ptr = (uintptr_t)n; e.name = U(NpcName(n)); e.inst = InstName(n);
        int g = fld<int>(n, K.offGuild);
        e.guild = GuildName(g); e.group = NpcGroupOfGuild(g); e.hp = Attr(n, 0); e.hpmax = Attr(n, 1);
        e.level = fld<int>(n, K.offLevel); e.dist = d / 100.f; e.ally = IsAlly(n); e.dead = NpcDead(n); e.monster = IsMonster(n);
        e.hostile = !e.dead && CallExt("Npc_GetAttitude", { Arg::Npc(n), Arg::Npc(h) }) == K.attHostile;
        g_near.push_back(std::move(e));
    });
    std::sort(g_near.begin(), g_near.end(), [](const NearNpc& a, const NearNpc& b) { return a.dist < b.dist; });
}
// allies that wandered off into their routine/dialog get put back into follow mode
void KeepParty() {
    if (T.gameClock - g_keepAt < 3.0) return;
    g_keepAt = T.gameClock;
    g_allies.erase(std::remove_if(g_allies.begin(), g_allies.end(), [](void* n) { return !NpcAlive(n); }), g_allies.end());
    for (void* n : g_allies) {
        if (IsMonster(n) || !NpcReady(n) || K.stFollow < 0) continue;
        if (CallExt("Npc_IsInState", { Arg::Npc(n), Arg::Int(K.stFollow) })) continue;
        if (K.stAttack >= 0 && CallExt("Npc_IsInState", { Arg::Npc(n), Arg::Int(K.stAttack) })) continue;
        SetNpcAivar(n, K.aivParty, 1);
        CallExt("AI_StartState", { Arg::Npc(n), Arg::Int(K.stFollow), Arg::Int(1), Arg::Str("") });
    }
}
}

void Npcs_Hooks() {}
void Npcs_World() { g_arena.clear(); g_frozenSpawn.clear(); g_wave = 0; g_next = 0; g_near.clear(); g_nearAt = -10; }
void Npcs_Tick(TickCtx& c) {
    if (T.menuOpen) RefreshNearby(c.hero);
    c.s.nearby = g_near;
    if (On("allykeep") && !c.paused) Guard("row:allykeep", [] { KeepParty(); });
    for (auto it = g_frozenSpawn.begin(); it != g_frozenSpawn.end();) {
        if (!NpcExists(*it)) { it = g_frozenSpawn.erase(it); continue; }
        if ((fld<uint8_t>(*it, VOB_SLEEPMODE) & 3) != 0) SetSleeping(*it, true);
        ++it;
    }
    if (On("arena")) { if (g_wave == 0 && g_next == 0) g_wave = std::max(0, (int)Val("arena") - 1); if (!c.paused) ArenaFrame(); }
    else { g_wave = 0; g_next = 0; g_arena.clear(); }
    c.s.arena = On("arena"); c.s.arenaWave = g_wave; c.s.arenaAlive = (int)g_arena.size();
}

void Npcs_Register() {
    const char* C = "Postacie";
    CardTitle("Arena");
    T.Add(MkRow("arena", "Tryb Areny", "fale wrogów 6-10 m od ciebie, na lądzie; liczba = fala startowa", C, "Arena", K_SLIDER, 1, 1, 20));
    St(ST_NEW, "build 3: CRASH (atak na postać bez załadowanego modelu); build 4: atak dopiero, gdy wróg jest gotowy"); Icon(ICN_CROSSHAIR);
    CardTitle("Masowo");
    T.Add(MkRow("killall", "Zabij wrogów w promieniu", "metry", C, "Arena", K_ACTION, 20, 1, 200, [](Row& r) {
        int c = 0;
        NpcsInRadius(r.val * 100.f, [&](void* n) {
            if (NpcDead(n) || IsAlly(n) || n == MainHero()) return;
            if (CallExt("Npc_GetAttitude", { Arg::Npc(n), Arg::Npc(Player()) }) != K.attHostile) return;
            Kill(n, Player()); c++;
        });
        T.Toast("Zabito " + std::to_string(c) + " wrogów"); }));
    St(ST_OK, "build 3: działa"); Icon(ICN_KILL);

    const char* D = "Drużyna";
    CardTitle("Drużyna");
    T.Add(MkRow("allyall", "Wszyscy w promieniu do drużyny", "metry, idą za tobą", C, D, K_ACTION, 10, 1, 100, [](Row& r) {
        int c = 0; NpcsInRadius(r.val * 100.f, [&](void* n) { if (!NpcDead(n) && n != MainHero()) { MakeAlly(n); c++; } });
        T.Toast(std::to_string(c) + " nowych sojuszników"); }));
    St(ST_OK, "build 3: działa (ale postacie z dialogiem odchodziły)"); Icon(ICN_USERS);
    T.Add(MkRow("dismiss", "Rozwiąż drużynę", "sojusznicy wracają do swoich zajęć", C, D, K_ACTION, 0, 0, 0, [](Row&) {
        std::vector<void*> a = g_allies; g_allies.clear();
        for (void* n : a) if (NpcAlive(n)) { SetNpcAivar(n, K.aivParty, 0); Peace(n); }
        T.Toast("Drużyna rozwiązana (" + std::to_string(a.size()) + ")"); }));
    NoVal(); St(ST_OK, "build 3: działa"); Icon(ICN_TIMES);
    CardTitle("Zachowanie drużyny");
    T.Add(MkRow("allynotalk", "Bez zaczepiania", "drużyna nie zaczyna z tobą dialogów (to je wyrywało z drużyny)", C, D, K_TOGGLE));
    T.rows.back().on = true; St(ST_NEW, "nowe w build 4");
    T.Add(MkRow("allykeep", "Pilnuj drużyny", "co 3 s kto wypadł z podążania, wraca za tobą", C, D, K_TOGGLE));
    T.rows.back().on = true; St(ST_NEW, "nowe w build 4");
    T.Add(MkRow("allyradius", "Promień listy wyboru", "m", C, D, K_VALUE, 30, 5, 150)); St(ST_NEW, "nowe w build 4");
}
