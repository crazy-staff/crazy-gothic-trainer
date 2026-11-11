// Chaos module: guild war, peace, mass destruction, invasions, physics fun.
#include "mods.h"
#include <algorithm>
#include <cmath>

using namespace g1;
using namespace gx;

namespace {
std::vector<int> g_savedAtt;
bool g_chaosActive = false;
std::set<void*> g_frozen;
double g_lastRain = 0;

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
        Guard("row:peace(chaos-off)", [] { PeaceAll(1e9f); });
        T.Toast("Pokój - wszyscy wracają do swoich zajęć");
    }
}
void SetFreeze(bool on) {
    if (on) {
        void* h = Player();
        ForEachNpc([&](void* n) { if (n != h && n != MainHero() && !IsAlly(n)) { SetSleeping(n, true); g_frozen.insert(n); } });
    } else {
        ForEachNpc([&](void* n) { if (g_frozen.count(n)) SetSleeping(n, false); });
        g_frozen.clear();
    }
}
struct RainKind { const char* fx; bool fire; };
const RainKind kRain[] = { { "SPELLFX_FIREBALL", true }, { "SPELLFX_THUNDERBOLT", false }, { "SPELLFX_ICECUBE", false }, { "SPELLFX_FIRERAIN", true } };
void RainFrame(float dmg) {
    double now = NowSec();
    if (now - g_lastRain < 0.6) return;
    g_lastRain = now;
    std::vector<void*> c;
    NpcsInRadius(3000.f, [&](void* n) { if (!NpcDead(n) && !IsAlly(n) && n != MainHero()) c.push_back(n); });
    if (c.empty()) return;
    void* t = c[rng() % c.size()];
    const RainKind& k = kRain[std::max(0, std::min(3, (int)Val("raintype")))];
    Guard(std::string("row:rain:") + k.fx, [&] { Effect(k.fx, Player(), t, 2, (int)dmg, k.fire ? K.damFire : K.damMagic, true); });
}
}

void Chaos_Hooks() {}
void Chaos_World() { g_chaosActive = false; g_frozen.clear(); }
void Chaos_Tick(TickCtx& c) {
    SetChaos(On("chaos"));
    if (On("freezenpc")) SetFreeze(true); else if (!g_frozen.empty()) SetFreeze(false);
    if (On("rain") && !c.paused) RainFrame(Val("rain"));
}

void Chaos_Register() {
    const char* C = "Chaos";
    T.Add(MkRow("chaos", "Wszyscy na wszystkich", "każda gildia wroga każdej; wyłączenie = pokój", C, "Wojna", K_TOGGLE));
    St(ST_OK, "działa; wyłączenie uspokaja (build 2: Pokój crashował - naprawione)"); Icon(ICN_FIRE);
    T.Add(MkRow("peace", "Pokój na świecie", "wszyscy przestają walczyć (bez sojuszników)", C, "Wojna", K_ACTION, 0, 0, 0, [](Row&) {
        PeaceAll(1e9f); T.Toast("Pokój"); })); NoVal(); Icon(ICN_HEART);
    St(ST_NEW, "build 2: CRASH (pusty argument NPC) - naprawione w build 3");
    T.Add(MkRow("allvsme", "Wszyscy przeciw tobie", "promień w metrach", C, "Wojna", K_ACTION, 30, 5, 300, [](Row& r) {
        int c = 0; NpcsInRadius(r.val * 100.f, [&](void* n) { if (!IsAlly(n) && !NpcDead(n) && n != MainHero()) { Aggro(n, Player()); c++; } });
        T.Toast(std::to_string(c) + " wrogów rusza na ciebie"); }));
    St(ST_NEW, "build 2: CRASH (pusty argument NPC) - naprawione w build 3"); Icon(ICN_CROSSHAIR);
    T.Add(MkRow("randguild", "Losowe gildie", "ludzie w promieniu (m) dostają losową gildię", C, "Wojna", K_ACTION, 30, 1, 300, [](Row& r) {
        int c = 0;
        NpcsInRadius(r.val * 100.f, [&](void* n) {
            if (IsMonster(n) || NpcDead(n)) return;
            int g = 1 + (int)(rng() % 14);
            CallExt("Npc_SetTrueGuild", { Arg::Npc(n), Arg::Int(g) }); fld<int>(n, K.offGuild) = g; c++;
        });
        T.Toast(std::to_string(c) + " zmian gildii"); }));
    St(ST_OK, "chyba działa (build 2)");

    T.Add(MkRow("thanos", "Pstryknięcie Thanosa", "połowa NPC w promieniu (m) rozpada się w pył", C, "Zagłada", K_ACTION, 50, 5, 500, [](Row& r) {
        std::vector<void*> v;
        NpcsInRadius(r.val * 100.f, [&](void* n) { if (!NpcDead(n) && !IsAlly(n) && n != MainHero()) v.push_back(n); });
        std::shuffle(v.begin(), v.end(), rng);
        size_t k = v.size() / 2;
        for (size_t i = 0; i < k; i++) {
            void* n = v[i];
            double t0 = 0.4 + 2.6 * (double)i / std::max<size_t>(1, k);
            Schedule(t0, [n] { if (NpcAlive(n)) Effect("SPELLFX_TRANSFORM", n, n, 0, 0, 0, false); });
            Schedule(t0 + 0.7, [n] {
                if (!NpcExists(n)) return;
                Kill(n, Player());
                fld<uint32_t>(n, VOB_FLAGS) &= ~(1u | VOBF_COLLSTAT | VOBF_COLLDYN);
            });
        }
        T.Toast("*pstryk*  " + std::to_string(k) + " z " + std::to_string(v.size()) + " obróci się w pył"); }));
    St(ST_NEW, "build 2: działało, ale pomijało nieśmiertelnych (np. straż bramy) - build 3: zdejmuje nieśmiertelność; efekt pyłu był pusty (naprawione)");
    Icon(ICN_MAGIC);
    T.Add(MkRow("meteor", "Meteoryt", "deszcz ognia na wszystkich w promieniu (m)", C, "Zagłada", K_ACTION, 25, 5, 200, [](Row& r) {
        int c = 0;
        NpcsInRadius(r.val * 100.f, [&](void* n) {
            if (!NpcDead(n) && !IsAlly(n) && n != MainHero()) { Effect("SPELLFX_FIRERAIN", n, n, 3, 120, K.damFire, false); c++; } });
        T.Toast("Meteoryt! (" + std::to_string(c) + " celów)"); }));
    St(ST_NEW, "build 2: nic się nie działo (pusty argument NPC) - naprawione w build 3"); Icon(ICN_FIRE);
    T.Add(MkRow("rain", "Deszcz zaklęć", "co 0.6 s zaklęcie w losowego NPC (30 m); wartość = obrażenia", C, "Zagłada", K_SLIDER, 80, 1, 2000));
    St(ST_NEW, "build 2: nic się nie działo - naprawione w build 3"); Icon(ICN_BOLT);
    T.Add(MkRow("raintype", "Rodzaj deszczu", "", C, "Zagłada", K_VALUE, 0, 0, 3));
    Opts({ { 0, "Ogniste kule" }, { 1, "Pioruny" }, { 2, "Bryły lodu" }, { 3, "Deszcz ognia" } });

    T.Add(MkRow("zombies", "Apokalipsa zombie", "ile zombie wokół ciebie; słabsze i śmiertelne", C, "Inwazja", K_ACTION, 8, 1, 40, [](Row& r) {
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
            Guard("npc:" + e->id, [&] { z = SpawnNpc(e->sym, SpawnPointNear(800.f, 1500.f, (float)i / n * 6.2831f)); });
            if (!z) continue;
            fld<int>(z, NPC_FLAGS) &= ~2;                       // not immortal
            for (int k = 0; k < 8; k++) fld<int>(z, protOff + 4 * k) = 10 + (int)(rng() % 15);
            int hp = 80 + (int)(rng() % 120);
            SetAttr(z, 1, hp); SetAttr(z, 0, hp);
            ZAssign(&fld<zSTRING>(z, nameOff), "Zombie apokalipsy");
            CallExt("Mdl_SetModelFatness", { Arg::Npc(z), Arg::Flt((float)(rng() % 300) / 100.f - 0.5f) });
        }
        T.Toast("Umarli powstają..."); }));
    St(ST_NEW, "build 2: działało, ale zombie były nieśmiertelne; build 3: słabsze, śmiertelne, własna nazwa"); Icon(ICN_USERS);
    T.Add(MkRow("polymorph", "Polimorf", "ludzie w promieniu (m) zmieniają się w zwierzęta - UWAGA: usuwa NPC (questy!)", C, "Inwazja", K_ACTION, 10, 1, 100, [](Row& r) {
        std::vector<const NpcEntry*> pool;
        for (auto& e : g_cat.npcs) if (e.group == NG_ANIMALS && e.level > 0 && !T.Crashed("npc:" + e.id)) pool.push_back(&e);
        if (pool.empty()) return;
        std::vector<void*> v;
        NpcsInRadius(r.val * 100.f, [&](void* n) { if (!NpcDead(n) && !IsMonster(n) && !IsAlly(n) && n != MainHero()) v.push_back(n); });
        for (void* n : v) {
            zVEC3 p = Pos(n);
            const NpcEntry* e = pool[rng() % pool.size()];
            g_savedTargets.erase(std::remove(g_savedTargets.begin(), g_savedTargets.end(), n), g_savedTargets.end());
            RemoveVob(n);
            Guard("npc:" + e->id, [&] { void* a = SpawnNpc(e->sym, p); if (a) Effect("SPELLFX_TRANSFORM", a, a, 0, 0, 0, false); });
        }
        T.Toast("Polimorf: " + std::to_string(v.size()) + " przemian"); }));
    St(ST_OK, "działa"); Icon(ICN_PAW);

    T.Add(MkRow("launchall", "Wszyscy w powietrze", "promień (m); NPC lecą 15 m w górę", C, "Fizyka", K_ACTION, 20, 1, 200, [](Row& r) {
        int c = 0; NpcsInRadius(r.val * 100.f, [&](void* n) { if (n == MainHero()) return; zVEC3 p = Pos(n); p.y += 1500.f; SafeSetPos(n, p); c++; });
        T.Toast(std::to_string(c) + " w powietrzu"); }));
    St(ST_OK, "działa"); Icon(ICN_ROCKET);
    T.Add(MkRow("freezenpc", "Zamroź wszystkich NPC", "świat staje, tylko ty się ruszasz", C, "Fizyka", K_TOGGLE));
    Icon(ICN_SNOW);
}
