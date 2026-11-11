#pragma once
// Shared game helpers used by several feature modules. Changes here affect many
// features, so they are covered by the start-up self test (Core_SelfTest).
#include <functional>
#include <random>
#include <set>
#include <string>
#include <vector>
#include "g1.h"

namespace gx {
using namespace g1;

struct Consts {           // script constants cached per world (cheap per-frame use)
    int sepHum = 16, sepOrc = 37, gilMax = 42;
    int attHostile = 0, attNeutral = 2, attFriendly = 3;
    int damFire = 8, damMagic = 32;
    int aivParty = 36;
    int stAttack = -1, stMmAttack = -1, stFollow = -1, stSummoned = -1, stMmAll = -1;
    int instHero = -1, nugget = -1;
    int offGuild = 0x1E8, offLevel = 0x1EC, offStartAi = -1, offItemName = -1, offItemValue = -1, offItemAmount = -1;
    void Refresh();
};
extern Consts K;
extern std::mt19937 rng;

std::string U(const std::string& cp1250);
bool IsMonster(void* npc);
bool NpcExists(void* n);
bool NpcAlive(void* n);
template <class F> void NpcsInRadius(float r, F f) {
    void* h = Player(); zVEC3 hp = Pos(h);
    // only npcs that are really in the world (model + animation controller); others crash AI/death calls
    ForEachNpc([&](void* n) { if (n != h && Dist(Pos(n), hp) <= r && NpcReady(n)) f(n); });
}

// main hero (the real PC_HERO body, even while body-swapped)
void* MainHero();
bool Swapped();
void TrackMainHero();

// targets
void* Target();                   // active saved target or focus npc
void* SavedTarget();
extern std::vector<void*> g_savedTargets;
extern int g_activeSaved;

// allies
extern std::vector<void*> g_allies;
bool IsAlly(void* n);
void MakeAlly(void* npc);
void Aggro(void* npc, void* victim);
void Peace(void* npc);
void PeaceAll(float radius = 6000.f);

void Effect(const char* fx, void* origin, void* target, int lvl, int dmg, int dtype, bool proj);
zVEC3 InFront(float dist);
zVEC3 Grounded(zVEC3 p, bool* water = nullptr);
zVEC3 SpawnPointNear(float minR, float maxR, float ang);

// delayed actions (game thread)
void Schedule(double delaySec, std::function<void()> f);
void RunSchedule();
void ClearWorldState();   // on world change

void DealDamage(void* npc, int dmg, void* killer);   // direct HP damage (no aggro on the player when killer == null)
bool Project(const zVEC3& world, float& sx, float& sy);   // -> normalized screen coords
std::string PlaceName(const zVEC3& p);                     // nearest waypoint -> human location name
std::string RegionOfWp(const std::string& wpName);         // waypoint prefix -> region ("" if unknown)
std::string InstName(void* npc);                           // script instance id (e.g. GRD_212_TORWACHE)
bool GameFocused();
bool On(const char* id);
float Val(const char* id);
void Guard(const std::string& key, const std::function<void()>& f);
}
