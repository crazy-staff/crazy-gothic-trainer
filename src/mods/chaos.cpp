// Fun module (was "Chaos"): war, destruction, spell rain, invasions, physics, Quicksilver.
#include "mods.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>

using namespace g1;
using namespace gx;

const char* kRainFx[RAIN_FX_COUNT] = { "SPELLFX_FIREBOLT", "SPELLFX_FIREBALL", "SPELLFX_FIRESTORM", "SPELLFX_FIRERAIN", "SPELLFX_THUNDERBOLT",
                                       "SPELLFX_THUNDERBALL", "SPELLFX_ICECUBE", "SPELLFX_ICEWAVE", "SPELLFX_WINDFIST" };
const char* kRainNames[RAIN_FX_COUNT] = { "Ognisty pocisk", "Ognista kula", "Burza ognia", "Deszcz ognia", "Piorun",
                                          "Kula błyskawic", "Bryła lodu", "Fala lodu", "Pięść wiatru" };

namespace {
std::vector<int> g_savedAtt;
bool g_chaosActive = false;
std::set<void*> g_frozen;
std::map<void*, std::pair<int, int>> g_aggroed;   // npc -> (perm, temp) attitude before 'all vs me'
double g_lastRain = 0, g_lastAllVsMe = 0;

void SetChaos(bool on) {
    int gmax = K.gilMax;
    if (on && !g_chaosActive) {
        g_savedAtt.assign(gmax * gmax, -1);
        for (int a = 1; a < gmax; a++)
            for (int b = 1; b < gmax; b++) {
                g_savedAtt[a * gmax + b] = CallExt("Wld_GetGuildAttitude", { Arg::Int(a), Arg::Int(b) });
                CallExt("Wld_SetGuildAttitude", { Arg::Int(a), Arg::Int(K.attHostile), Arg::Int(b) });
            }
        g_chaosActive = true;
    } else if (!on && g_chaosActive) {
        for (int a = 1; a < gmax; a++)
            for (int b = 1; b < gmax; b++) {
                int v = g_savedAtt[a * gmax + b];
                if (v >= 0) CallExt("Wld_SetGuildAttitude", { Arg::Int(a), Arg::Int(v), Arg::Int(b) });
            }
        g_chaosActive = false;
        Guard("row:peace(chaos-off)", [] { PeaceAll(6000.f); });
        T.Toast("Pokój - wszyscy wracają do swoich zajęć");
    }
}
void SetFreeze(bool on) {
    if (on) {
        void* h = Player();
        ForEachNpc([&](void* n) { if (n != h && n != MainHero() && !IsAlly(n) && !g_frozen.count(n)) { SetSleeping(n, true); g_frozen.insert(n); } });
    } else {
        ForEachNpc([&](void* n) { if (g_frozen.count(n)) SetSleeping(n, false); });
        g_frozen.clear();
    }
}
bool Victim(void* n) { return !NpcDead(n) && !IsAlly(n) && n != MainHero() && n != Player(); }
// "kogo": 0 wszyscy, 1 wrogowie, 2 ludzie, 3 potwory
bool PassFilter(void* n, int f) {
    switch (f) {
    case 1: return CallExt("Npc_GetAttitude", { Arg::Npc(n), Arg::Npc(Player()) }) == K.attHostile;
    case 2: return !IsMonster(n);
    case 3: return IsMonster(n);
    default: return true;
    }
}
}

// Spell visual played on the target itself (not cast by the hero -> nobody blames the player),
// damage applied directly a moment later.
void SpellOn(void* t, const char* fx, int dmg, void* killer) {
    if (!NpcAlive(t)) return;
    bool fire = strstr(fx, "FIRE") != nullptr;
    Effect(fx, t, t, 2, 0, fire ? K.damFire : K.damMagic, false);
    if (strstr(fx, "ICE")) {
        if (NpcReady(t)) { SetSleeping(t, true); Schedule(4.0, [t] { if (NpcExists(t)) SetSleeping(t, false); }); }
    }
    if (dmg > 0) Schedule(0.5, [t, dmg, killer] { if (NpcAlive(t)) DealDamage(t, dmg, NpcExists(killer) ? killer : nullptr); });
}

namespace {
void RainFrame() {
    double now = T.gameClock;
    // build 7: the engine crashed when many projectiles hit (and killed) at once -> hard limits:
    // >= 0.35 s between volleys, <= 6 per volley, <= 12 in flight, one projectile per victim every 2.5 s
    if (now - g_lastRain < std::max(0.35f, Val("rainint"))) return;
    g_lastRain = now;
    static std::map<void*, double> recent;
    for (auto it = recent.begin(); it != recent.end();) { if (now - it->second > 2.5) it = recent.erase(it); else ++it; }
    int inFlight = (int)recent.size();
    if (inFlight >= 12) return;
    int mask = (int)Val("rainmask");
    std::vector<int> fx;
    for (int i = 0; i < RAIN_FX_COUNT; i++) if (mask & (1 << i)) fx.push_back(i);
    if (fx.empty()) fx.push_back(1);
    std::vector<void*> c;
    int filt = (int)Val("rainfilter");
    NpcsInRadius(Val("rainradius") * 100.f, [&](void* n) {
        if (!Victim(n) || !PassFilter(n, filt) || recent.count(n) || !NpcReady(n)) return;
        if (IsImmortal(n)) { if (!On("rainimm")) return; fld<int>(n, NPC_FLAGS) &= ~2; }
        c.push_back(n); });
    if (c.empty()) return;
    std::shuffle(c.begin(), c.end(), rng);
    int cnt = std::min({ (int)c.size(), std::max(1, (int)Val("raincount")), 6, 12 - inFlight });
    for (int i = 0; i < cnt; i++) {
        recent[c[i]] = now;
        const char* f = kRainFx[fx[rng() % fx.size()]];
        bool fire = strstr(f, "FIRE") != nullptr;
        Guard(std::string("row:rain:") + f, [&] { Effect(f, Player(), c[i], 2, (int)Val("raindmg"), fire ? K.damFire : K.damMagic, true); });
    }
}
void AllVsMeFrame() {
    double now = T.gameClock;
    if (now - g_lastAllVsMe < 1.5) return;
    g_lastAllVsMe = now;
    void* h = Player();
    NpcsInRadius(Val("allvsmer") * 100.f, [&](void* n) {
        if (!Victim(n) || g_aggroed.count(n)) return;
        int perm = CallExt("Npc_GetPermAttitude", { Arg::Npc(n), Arg::Npc(h) });
        int temp = CallExt("Npc_GetAttitude", { Arg::Npc(n), Arg::Npc(h) });
        g_aggroed[n] = { perm, temp };
        Aggro(n, h);
    });
}
}

void Chaos_Hooks() {}
void Chaos_World() { g_chaosActive = false; g_frozen.clear(); g_aggroed.clear(); g_lastRain = g_lastAllVsMe = 0; }
void Chaos_Tick(TickCtx& c) {
    SetChaos(On("chaos"));
    if (On("freezenpc")) SetFreeze(true); else if (!g_frozen.empty()) SetFreeze(false);
    if (c.paused) return;
    if (On("rain")) RainFrame();
    if (On("allvsme")) AllVsMeFrame();
    else if (!g_aggroed.empty()) {
        std::map<void*, std::pair<int, int>> a; a.swap(g_aggroed);
        Guard("row:allvsme-off", [&] {   // everyone goes back to exactly how they felt about you before
            for (auto& e : a) {
                void* n = e.first;
                if (!NpcAlive(n) || !NpcReady(n)) continue;
                CallExt("Npc_SetAttitude", { Arg::Npc(n), Arg::Int(e.second.first) });
                CallExt("Npc_SetTempAttitude", { Arg::Npc(n), Arg::Int(e.second.second) });
                CallExt("Npc_ClearAIQueue", { Arg::Npc(n) });
                CallExt("AI_StandupQuick", { Arg::Npc(n) });
                if (IsMonster(n)) { int st = K.stMmAll; if (st > 0) CallExt("AI_StartState", { Arg::Npc(n), Arg::Int(st), Arg::Int(1), Arg::Str("") }); }
                else CallExt("AI_ContinueRoutine", { Arg::Npc(n) });
            }
        });
    }
}

void Chaos_Register() {
    const char* C = "Fun";
    const char* W = "Walka";
    CardTitle("Wojna gildii");
    T.Add(MkRow("chaos", "Wszyscy na wszystkich", "każda gildia wroga każdej; wyłączenie = pokój w promieniu 60 m", C, W, K_TOGGLE));
    St(ST_OK, "build 3: działa"); Icon(ICN_FIRE);
    T.Add(MkRow("randguild", "Losowe gildie", "ludzie w promieniu (m) dostają losową gildię", C, W, K_ACTION, 30, 1, 300, [](Row& r) {
        int c = 0;
        NpcsInRadius(r.val * 100.f, [&](void* n) {
            if (IsMonster(n) || NpcDead(n)) return;
            int g = 1 + (int)(rng() % 14);
            CallExt("Npc_SetTrueGuild", { Arg::Npc(n), Arg::Int(g) }); fld<int>(n, K.offGuild) = g; c++;
        });
        T.Toast(std::to_string(c) + " zmian gildii"); }));
    St(ST_OK, "build 3: działa"); Icon(ICN_REFRESH);
    T.Add(MkRow("peace", "Pokój", "wszyscy w promieniu (m) przestają walczyć, także z tobą", C, W, K_ACTION, 50, 5, 300, [](Row& r) {
        void* h = Player();
        int c = 0;
        NpcsInRadius(r.val * 100.f, [&](void* n) {
            if (NpcDead(n) || IsAlly(n) || n == MainHero()) return;
            int perm = CallExt("Npc_GetPermAttitude", { Arg::Npc(n), Arg::Npc(h) });
            if (perm == K.attHostile) perm = K.attNeutral;   // hostile towards you -> neutral
            CallExt("Npc_SetAttitude", { Arg::Npc(n), Arg::Int(perm) });
            CallExt("Npc_SetTempAttitude", { Arg::Npc(n), Arg::Int(perm) });
            Peace(n); c++;
        });
        if (CallExt("Npc_IsInFightMode", { Arg::Npc(h), Arg::Int(0) }) == 0) CallExt("AI_RemoveWeapon", { Arg::Npc(h) });
        T.Toast("Pokój (" + std::to_string(c) + ")"); }));
    Icon(ICN_HEART);
    St(ST_BUG, "build 5: działa, ale nie uspokajało wrogów gracza; build 6: wrogość wobec ciebie zamieniana na neutralną");
    CardTitle("Masowo");
    T.Add(MkRow("killall", "Zabij wrogów w promieniu", "metry", C, W, K_ACTION, 20, 1, 200, [](Row& r) {
        int c = 0;
        NpcsInRadius(r.val * 100.f, [&](void* n) {
            if (NpcDead(n) || IsAlly(n) || n == MainHero()) return;
            if (CallExt("Npc_GetAttitude", { Arg::Npc(n), Arg::Npc(Player()) }) != K.attHostile) return;
            if (IsImmortal(n) && !On("killallimm")) return;
            Kill(n, Player()); c++;
        });
        T.Toast("Zabito " + std::to_string(c) + " wrogów"); }));
    St(ST_OK, "build 3: działa"); Icon(ICN_KILL);
    T.Add(MkRow("killallimm", "Też nieśmiertelnych", "", C, W, K_TOGGLE)); St(ST_NEW, "nowe w build 6");
    CardTitle("Przeciw tobie");
    T.Add(MkRow("allvsme", "Wszyscy przeciw tobie", "dopóki włączone, każdy w promieniu rusza na ciebie; wyłączenie = pokój", C, W, K_TOGGLE));
    St(ST_NEW, "build 3: działało jako jednorazowa akcja; build 4: przełącznik"); Icon(ICN_CROSSHAIR);
    T.Add(MkRow("allvsmer", "Promień", "m", C, W, K_VALUE, 30, 5, 200)); St(ST_NEW, "nowe w build 4");

    const char* Z = "Magia";
    CardTitle("Pstryknięcie");
    T.Add(MkRow("thanos", "Pstryknięcie Thanosa", "NPC w promieniu (m) rozpadają się w pył", C, Z, K_ACTION, 50, 5, 500, [](Row& r) {
        std::vector<void*> v;
        int who = (int)Val("thanoswho");
        NpcsInRadius(r.val * 100.f, [&](void* n) {
            if (!Victim(n) || !PassFilter(n, who)) return;
            if (IsImmortal(n) && !On("thanosimm")) return;
            v.push_back(n); });
        std::shuffle(v.begin(), v.end(), rng);
        size_t k = (size_t)std::lround(v.size() * std::max(0.f, std::min(100.f, Val("thanospct"))) / 100.f);
        for (size_t i = 0; i < k; i++) {
            void* n = v[i];
            double t0 = 0.4 + 2.6 * (double)i / std::max<size_t>(1, k);
            Schedule(t0, [n] { if (NpcAlive(n)) Effect("SPELLFX_TRANSFORM", n, n, 0, 0, 0, false); });
            Schedule(t0 + 0.7, [n] {
                if (!NpcExists(n)) return;
                Kill(n, Player());   // kill credited to you -> experience
                fld<uint32_t>(n, VOB_FLAGS) &= ~(1u | VOBF_COLLSTAT | VOBF_COLLDYN);
            });
        }
        T.Toast("*pstryk*  " + std::to_string(k) + " z " + std::to_string(v.size()) + " obróci się w pył"); }));
    St(ST_NEW, "build 3: działa; build 4: domyślnie wszyscy; build 5: zabójstwa dają PD"); Icon(ICN_MAGIC);
    T.Add(MkRow("thanoswho", "Kogo", "", C, Z, K_VALUE, 0, 0, 3));
    Opts({ { 0, "Wszystkich" }, { 1, "Tylko wrogów" }, { 2, "Tylko ludzi" }, { 3, "Tylko potwory" } }); St(ST_NEW, "nowe w build 6");
    T.Add(MkRow("thanospct", "Ile z nich", "%", C, Z, K_VALUE, 100, 1, 100)); St(ST_NEW, "build 6: procent zamiast wszyscy/połowa");
    T.Add(MkRow("thanosimm", "Też nieśmiertelnych", "", C, Z, K_TOGGLE)); T.rows.back().on = true; St(ST_NEW, "nowe w build 6");
    CardTitle("Meteoryt");
    T.Add(MkRow("meteor", "Meteoryt", "deszcz ognia na wszystkich w promieniu (m)", C, Z, K_ACTION, 25, 5, 200, [](Row& r) {
        int c = 0;
        NpcsInRadius(r.val * 100.f, [&](void* n) {
            if (!Victim(n)) return;
            if (IsImmortal(n)) { if (!On("meteorimm")) return; fld<int>(n, NPC_FLAGS) &= ~2; }
            SpellOn(n, "SPELLFX_FIRERAIN", (int)Val("meteordmg"), Player()); c++; });
        T.Toast("Meteoryt! (" + std::to_string(c) + " celów)"); }));
    St(ST_NEW, "build 3: bez obrażeń; build 4: obrażenia bezpośrednie; build 5: zabójstwa dają PD"); Icon(ICN_FIRE);
    T.Add(MkRow("meteordmg", "Obrażenia", "", C, Z, K_VALUE, 150, 1, 2000)); St(ST_OK, "build 5: działa");
    T.Add(MkRow("meteorimm", "Też nieśmiertelnych", "", C, Z, K_TOGGLE)); St(ST_NEW, "nowe w build 6");

    const char* M = "Magia";
    CardTitle("Deszcz zaklęć");
    T.Add(MkRow("rain", "Deszcz zaklęć", "zaklęcia spadają na losowe postacie wokół ciebie", C, M, K_TOGGLE));
    St(ST_NEW, "build 6: CRASH przy wielu zgonach naraz; build 7: limity (max 6 na salwę, 12 w locie, min 0,35 s)"); Icon(ICN_BOLT);
    T.Add(MkRow("raindmg", "Obrażenia", "", C, M, K_VALUE, 80, 0, 2000)); St(ST_NEW, "nowe w build 4");
    T.Add(MkRow("rainint", "Co ile sekund", "", C, M, K_VALUE, 0.6f, 0.35f, 5, nullptr, true)); St(ST_NEW, "nowe w build 4");
    T.Add(MkRow("raincount", "Celów naraz", "", C, M, K_VALUE, 1, 1, 6)); St(ST_NEW, "nowe w build 4");
    T.Add(MkRow("rainradius", "Promień", "m", C, M, K_VALUE, 30, 5, 150)); St(ST_NEW, "nowe w build 4");
    T.Add(MkRow("rainfilter", "W kogo", "", C, M, K_VALUE, 0, 0, 3));
    Opts({ { 0, "Wszystkich (bez drużyny)" }, { 1, "Tylko wrogów" }, { 2, "Tylko ludzi" }, { 3, "Tylko potwory" } }); St(ST_NEW, "nowe w build 4");
    T.Add(MkRow("rainimm", "Też nieśmiertelnych", "", C, M, K_TOGGLE)); St(ST_NEW, "nowe w build 6");
    T.Add(MkRow("rainmask", "Zaklęcia", "", C, M, K_VALUE, 2 | 16, 0, 511)); Hidden(); St(ST_NEW, "nowe w build 4");

    const char* I = "Walka";   // build 7: each transformation has its own card under Walka
    CardTitle("Zombie");
    T.Add(MkRow("zombies", "Apokalipsa zombie", "ile zombie wokół ciebie; słabsze i śmiertelne", C, I, K_ACTION, 8, 1, 200, [](Row& r) {
        std::vector<const NpcEntry*> pool;
        for (auto& e : g_cat.npcs) if (e.group == NG_UNDEAD && e.id.find("ZOMBIE") != std::string::npos && !T.Crashed("npc:" + e.id)) pool.push_back(&e);
        if (pool.empty()) for (auto& e : g_cat.npcs) if (e.group == NG_UNDEAD && !T.Crashed("npc:" + e.id)) pool.push_back(&e);
        if (pool.empty()) { T.Toast("Brak nieumarłych w skryptach"); return; }
        int n = (int)r.val;
        static int nameOff = -1, protOff = -1;
        if (nameOff < 0) { nameOff = FieldOff("C_NPC.NAME", 0x104); protOff = FieldOff("C_NPC.PROTECTION", 0x1A4); }
        for (int i = 0; i < n; i++) {
            const NpcEntry* e = pool[rng() % pool.size()];
            void* z = nullptr;
            float rr = 800.f + (float)(i / 40) * 300.f;
            Guard("npc:" + e->id, [&] { z = SpawnNpc(e->sym, SpawnPointNear(rr, rr + 700.f, (float)i / std::min(n, 40) * 6.2831f + (float)(i / 40) * 0.3f)); });
            if (!z) continue;
            MakeMortal(z);
            for (int k = 0; k < 8; k++) fld<int>(z, protOff + 4 * k) = 10 + (int)(rng() % 15);
            int hp = 80 + (int)(rng() % 120);
            SetAttr(z, 1, hp); SetAttr(z, 0, hp);
            ZAssign(&fld<zSTRING>(z, nameOff), "Zombie apokalipsy");
            CallExt("Mdl_SetModelFatness", { Arg::Npc(z), Arg::Flt((float)(rng() % 300) / 100.f - 0.5f) });
        }
        T.Toast("Umarli powstają..."); }));
    St(ST_OK, "build 3: działa"); Icon(ICN_USERS);
    CardTitle("Polimorfia");
    T.Add(MkRow("polymorph", "Polimorf", "ludzie w promieniu (m) zmieniają się w zwierzęta - UWAGA: usuwa NPC (questy!)", C, I, K_ACTION, 10, 1, 100, [](Row& r) {
        std::vector<const NpcEntry*> pool;
        for (auto& e : g_cat.npcs) if (e.group == NG_ANIMALS && e.level > 0 && !T.Crashed("npc:" + e.id)) pool.push_back(&e);
        if (pool.empty()) return;
        std::vector<void*> v;
        NpcsInRadius(r.val * 100.f, [&](void* n) { if (Victim(n) && !IsMonster(n)) v.push_back(n); });
        for (void* n : v) {
            zVEC3 p = Pos(n);
            const NpcEntry* e = pool[rng() % pool.size()];
            g_savedTargets.erase(std::remove(g_savedTargets.begin(), g_savedTargets.end(), n), g_savedTargets.end());
            RemoveVob(n);
            Guard("npc:" + e->id, [&] { void* a = SpawnNpc(e->sym, p); if (a) Effect("SPELLFX_TRANSFORM", a, a, 0, 0, 0, false); });
        }
        T.Toast("Polimorf: " + std::to_string(v.size()) + " przemian"); }));
    St(ST_OK, "działa"); Icon(ICN_PAW);

    const char* F = "Chaos";
    CardTitle("Fizyka");
    T.Add(MkRow("launchall", "Wszyscy w powietrze", "promień (m); NPC lecą 15 m w górę", C, F, K_ACTION, 20, 1, 200, [](Row& r) {
        int c = 0; NpcsInRadius(r.val * 100.f, [&](void* n) { if (n == MainHero()) return; zVEC3 p = Pos(n); p.y += 1500.f; SafeSetPos(n, p); c++; });
        T.Toast(std::to_string(c) + " w powietrzu"); }));
    St(ST_OK, "działa"); Icon(ICN_ROCKET);
    T.Add(MkRow("freezenpc", "Zamroź wszystkich NPC", "świat staje, tylko ty się ruszasz", C, F, K_TOGGLE));
    St(ST_OK, "build 3: działa"); Icon(ICN_SNOW);

    const char* Q = "Chaos";
    CardTitle("Quicksilver");
    T.Add(MkRow("quicksilver", "Tryb Quicksilver", "wszystko wokół prawie stoi, ty poruszasz się normalnie (lub szybciej)", C, Q, K_TOGGLE));
    St(ST_OK, "build 5: działa"); Icon(ICN_BOLT); Excl({ "speed" });
    T.Add(MkRow("qsfactor", "Tempo świata", "0.05 = prawie stop", C, Q, K_VALUE, 0.12f, 0.02f, 0.6f, nullptr, true)); St(ST_NEW, "nowe w build 4");
    T.Add(MkRow("qshero", "Twoje tempo", "1 = normalnie", C, Q, K_VALUE, 1.3f, 1, 3, nullptr, true)); St(ST_NEW, "nowe w build 4");
    T.Add(MkRow("qsallies", "Drużyna też szybka", "", C, Q, K_TOGGLE)); T.rows.back().on = true; St(ST_NEW, "nowe w build 4");
}
