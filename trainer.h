#pragma once
// Trainer core: feature rows, action queue, snapshot shared with the UI, persistence.
#include <functional>
#include <mutex>
#include <string>
#include <vector>
#include "core/g1.h"

#define TRAINER_BUILD 4

enum Kind {
    K_TOGGLE,   // [ ON ]/[ OFF ]
    K_SLIDER,   // slider + ON/OFF
    K_SET,      // value + [ Ustaw ], override stays active (x / right click = off)
    K_ACTION,   // value (optional) + [ Wykonaj ] one shot
    K_CHOICE,   // dropdown + [ Ustaw ] one shot
    K_VALUE,    // setting only (number or dropdown), no button
};
enum Status { ST_NEW, ST_OK, ST_BUG, ST_CRASH };

struct Row {
    std::string id, name, note, cat, sub, group;   // group = card title inside the subsection
    Kind kind;
    float val = 0, vmin = 0, vmax = 0;
    bool isFloat = false;
    bool on = false;
    int hotkey = 0;
    std::function<void(Row&)> act;              // game thread
    bool noValue = false;
    bool hidden = false;                         // drawn by a custom panel, not as a row
    std::vector<std::pair<int, std::string>> options;
    Status status = ST_NEW;
    std::string hist;
    std::string icon;                            // Fork Awesome glyph (utf8)
};

struct Label {
    int type;                 // 0 npc 1 item 2 chest
    float x, y;
    int box;                  // 0 none 1 rect 2 3d
    float bx0, by0, bx1, by1;
    float corners[16];        // 3d box: 8 projected points (x,y)
    std::vector<float> bones; // skeleton segments x0,y0,x1,y1 (normalized)
    std::string text, sub;
    std::vector<std::string> lines;   // equipment / chest contents
    unsigned color;
    float hp;
};

struct DiagLine { std::string text; bool ok; };
struct NearNpc {                  // npc list for pickers (ally picker etc.)
    uintptr_t ptr; std::string name, guild, inst; int group, hp, hpmax, level; float dist; bool ally, dead, monster, hostile;
};
struct SavedInfo {                // saved target details for the target panel
    uintptr_t ptr; std::string name, inst, guild, group, place; float x, y, z, dist; int hp, hpmax, level; bool dead, monster;
    bool onScreen; float headX, headY, headR;   // normalized screen pos/size of the head (portrait capture)
};

struct Snapshot {
    bool inGame = false;
    int hp = 0, hpmax = 0, mana = 0, manamax = 0, str = 0, dex = 0, level = 0, guild = 0, lp = 0, exp = 0, expNext = 0;
    int ore = 0, day = 0, hour = 0, minute = 0;
    float px = 0, py = 0, pz = 0;
    std::string world, bodyName, guildName;
    bool tValid = false, tSaved = false; std::string tName, tGuild, tGroup; int tHp = 0, tHpMax = 0, tLevel = 0; bool tDead = false, tMonster = false;
    std::vector<std::string> savedTargets; int activeSaved = -1;
    std::vector<SavedInfo> saved;
    std::string tInst, tPlace; float tx = 0, ty = 0, tz = 0; uintptr_t tPtr = 0;
    std::vector<NearNpc> nearby;
    bool quicksilver = false;
    bool swapped = false;
    bool arena = false; int arenaWave = 0, arenaAlive = 0;
    std::vector<Label> labels;
    bool espLines = false;
    bool speed = false; float speedVal = 1;
    bool ghost = false, noclip = false, paused = false;
    std::string marvinOut;
    std::vector<DiagLine> diag;
};

struct SavedPos { std::string name; float x, y, z; };
struct ItemSet { std::string name; std::vector<std::pair<std::string, int>> items; bool preset = false; };
struct Macro { std::string name; std::string cmds; int repeat = 1; };

struct Trainer {
    std::vector<Row> rows;
    std::mutex mtx;
    std::vector<std::pair<std::string, std::function<void()>>> queue;
    Snapshot snap;
    std::string toast; double toastUntil = 0;
    bool menuOpen = false;

    int spawnMode = 0, spawnCount = 1, itemAmount = 1;
    std::vector<SavedPos> positions;
    std::vector<std::string> crashKeys;
    std::vector<ItemSet> sets;          // user + preset item sets (ui thread owns, game thread reads copies)
    ItemSet editSet;                     // set being built in the UI
    std::vector<Macro> macros;
    float accent[3] = { 0.66f, 0.55f, 1.00f };
    bool testerMode = false;
    double gameClock = 0;                // seconds of unpaused game time (pause-safe scheduling)

    void Init();
    Row* Get(const std::string& id);
    void Add(Row r);
    void Queue(const std::string& label, std::function<void()> f);
    void Toast(const std::string& s);
    void Tick();
    void Hotkey(int vk);
    void Load();
    void Save();
    void RestoreToggles();
    bool Crashed(const std::string& key);
    void LoadPositions(const std::string& world);
    void SavePositions(const std::string& world);
    void LoadSets(); void SaveSets();
    void LoadMacros(); void SaveMacros();
};
extern Trainer T;
double NowSec();
std::string IniPath();
std::string DataPath(const char* file);
void Log(const char* fmt, ...);
void CrashGuardInit();
void ApplyRow(Row& r);

// UI -> game thread actions
void Act_GiveItem(int sym, int amount);
void Act_DropItem(int sym, int amount);
void Act_GiveItemTarget(int sym, int amount);
void Act_GiveSet(const ItemSet& s);
void Act_SpawnNpc(int sym, int count, int mode);
void Act_Teleport(float x, float y, float z, const std::string& name);
void Act_SavePos(const std::string& name);
void Act_DeletePos(int idx);
void Act_Console(const std::string& cmd);
void Act_RunMacro(const Macro& m);
void Act_RunScript(const std::string& fn);
void Act_SelectSavedTarget(int idx);
void Act_RemoveSavedTarget(int idx);
void Act_Unswap();
void Act_MakeAllies(const std::vector<uintptr_t>& npcs);
void Act_SelectSavedPtr(uintptr_t p);
void CardTitle(const char* t);       // following rows of the module go into this card

void InstallGameHooks();
