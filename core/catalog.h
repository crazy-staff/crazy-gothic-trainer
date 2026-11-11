#pragma once
#include <string>
#include <vector>

// Lists of every item / NPC instance and every waypoint, read from the running
// game (Daedalus symbol table + bytecode, world waynet). Built on the game thread.
struct ItemEntry {
    int sym = 0; std::string id, name; int mainflag = 0, value = 0; int cat = 0; int flags = 0; std::string sub;
    int damage = 0, damageArr[8] = {}, range = 0, prot[8] = {}, condAtr[3] = {}, condVal[3] = {};
    std::string text[6]; int count[6] = {};
    std::string info;   // short human summary (damage/protection/requirements/effect)
};
struct NpcEntry {
    int sym; std::string id, name; int guild, level; bool monster; int group = 0;
};
enum NpcGroup { NG_OLDCAMP, NG_NEWCAMP, NG_SWAMP, NG_MAGES, NG_OTHERHUM, NG_ANIMALS, NG_BEASTS, NG_UNDEAD, NG_ORCS,
                NG_MAGIC, NG_COUNT };
extern const char* kNpcGroupNames[NG_COUNT];
struct WpEntry { std::string name; float x, y, z; };

enum ItemCat { IC_MELEE, IC_RANGED, IC_AMMO, IC_ARMOR, IC_RUNE, IC_SCROLL, IC_POTION, IC_FOOD,
               IC_MAGIC, IC_KEY, IC_DOCS, IC_MISC, IC_COUNT };
extern const char* kItemCatNames[IC_COUNT];

struct Catalog {
    std::vector<ItemEntry> items;
    std::vector<NpcEntry> npcs;
    std::vector<WpEntry> wps;
    std::vector<std::string> vfx;
    bool built = false;
    void* builtForWorld = nullptr;
    void Build();
    void BuildWaypoints();
};
extern Catalog g_cat;
std::string Cp1250ToUtf8(const std::string& s);
std::string GuildName(int g);
int NpcGroupOfGuild(int guild);
