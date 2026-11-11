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
            else if (mode == 2) { CallExt("Npc_ClearAIQueue", { Arg::Npc(npc) }); SetSleeping(npc, true); g_frozenSpawn.insert(npc); }
        }
        static const char* mn[] = { "wrogi", "sojusznik", "zamrożony", "neutralny" };
        T.Toast("Spawn: " + std::to_string(ok) + "x " + id + " (" + mn[mode & 3] + ")");
    });
}

void Npcs_Hooks() {}
void Npcs_World() { g_arena.clear(); g_frozenSpawn.clear(); g_wave = 0; g_next = 0; }
void Npcs_Tick(TickCtx& c) {
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
    const char* C = "Potwory i NPC";
    T.Add(MkRow("arena", "Tryb Areny", "fale wrogów 6-10 m od ciebie, na lądzie; liczba = fala startowa", C, "Arena", K_SLIDER, 1, 1, 20));
    St(ST_NEW, "build 2: CRASH po chwili (pusty argument NPC w ataku) - naprawione w build 3"); Icon(ICN_CROSSHAIR);
    T.Add(MkRow("killall", "Zabij wrogów w promieniu", "metry", C, "Masowe", K_ACTION, 20, 1, 200, [](Row& r) {
        int c = 0;
        NpcsInRadius(r.val * 100.f, [&](void* n) {
            if (NpcDead(n) || IsAlly(n)) return;
            if (CallExt("Npc_GetAttitude", { Arg::Npc(n), Arg::Npc(Player()) }) != K.attHostile) return;
            Kill(n, Player()); c++;
        });
        T.Toast("Zabito " + std::to_string(c) + " wrogów"); }));
    Icon(ICN_KILL);
    T.Add(MkRow("allyall", "Wszyscy w promieniu = sojusznicy", "metry, idą za tobą", C, "Masowe", K_ACTION, 10, 1, 100, [](Row& r) {
        int c = 0; NpcsInRadius(r.val * 100.f, [&](void* n) { if (!NpcDead(n)) { MakeAlly(n); c++; } });
        T.Toast(std::to_string(c) + " nowych sojuszników"); }));
    St(ST_NEW, "używa tej samej logiki co kontrola umysłu (build 2: crash, naprawione)"); Icon(ICN_USERS);
    T.Add(MkRow("dismiss", "Rozwiąż drużynę", "sojusznicy wracają do swoich zajęć", C, "Masowe", K_ACTION, 0, 0, 0, [](Row&) {
        std::vector<void*> a = g_allies; g_allies.clear();
        for (void* n : a) if (NpcAlive(n)) { SetNpcAivar(n, K.aivParty, 0); Peace(n); }
        T.Toast("Drużyna rozwiązana (" + std::to_string(a.size()) + ")"); }));
    NoVal();
}
