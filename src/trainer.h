#pragma once
// Trainer core: feature rows, action queue, snapshot shared with the UI, persistence.
#include <functional>
#include <mutex>
#include <string>
#include <vector>
#include "core/g1.h"

#define TRAINER_BUILD 9

enum Kind {
    K_TOGGLE,   // [ ON ]/[ OFF ]
    K_SLIDER,   // slider + ON/OFF
    K_SET,      // value + [ Ustaw ], override stays active (x / right click = off)
    K_ACTION,   // value (optional) + [ Wykonaj ] one shot
    K_CHOICE,   // dropdown + [ Ustaw ] one shot
    K_VALUE,    // setting only (number or dropdown), no button
};
enum Status { ST_NEW, ST_OK, ST_BUG, ST_CRASH };
enum { HK_CTRL = 1 << 16, HK_ALT = 2 << 16, HK_SHIFT = 4 << 16 };

struct Row {
    std::string id, name, note, cat, sub, group;   // group = card title inside the subsection
    Kind kind;
    float val = 0, vmin = 0, vmax = 0, def = 0;     // def = default value (reset icon)
    bool isFloat = false;
    bool on = false;
    int hotkey = 0;                              // vk | HK_CTRL/HK_ALT/HK_SHIFT
    std::function<void(Row&)> act;              // game thread
    bool noValue = false;
    bool hidden = false;                         // drawn by a custom panel, not as a row
    std::vector<std::pair<int, std::string>> options;
    Status status = ST_NEW;
    std::string hist;
    std::string icon;                            // Fork Awesome glyph (utf8)
    bool live = false;                           // value applies immediately (no button)
    std::vector<std::string> excl;               // rows that cannot be on together with this one
};

struct Label {
    int type;                 // 0 npc 1 item 2 chest
    float x, y;
    int box;                  // 0 none 1 rect 2 3d
    float bx0, by0, bx1, by1;
    float corners[16];        // 3d box: 8 projected points (x,y)
    std::vector<float> bones; // skeleton segments x0,y0,x1,y1 (normalized)
    bool anchor = false; float ax = 0, ay = 0;   // where the ground line ends (feet of the skeleton)
    std::string text, sub;
    std::vector<std::string> lines;   // equipment / chest contents
    unsigned color;
    float hp;
};

struct DiagLine { std::string text; bool ok; };
struct NearNpc {                  // npc list for pickers (ally picker etc.)
    uintptr_t ptr; std::string name, guild, inst; int group, hp, hpmax, level; float dist; bool ally, dead, monster, hostile;
    int pstate = 0;   // party: 0 follows, 1 waits, 2 back at post
};
struct SavedInfo {                // saved target details for the target panel
    uintptr_t ptr; std::string name, inst, guild, group, place; float x, y, z, dist; int hp, hpmax, level; bool dead, monster, frozen;
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
    std::vector<NearNpc> nearby, party, spawned;
    std::string subjectName; uintptr_t subjectPtr = 0;
    bool quicksilver = false;
    std::string subjectInst, tInst2, heroInst;
    bool photoBusy = false, photoBatch = false; int photoDone = 0, photoTotal = 0; std::string photoLabel;
    int espHidden = 0;
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
    volatile double lastTick = 0;        // real time of the last game frame (game menu open = stale)
    volatile bool inGameMenu = false;    // the game's own Esc/main menu is running
    bool dirty = false; double lastSave = 0;
    void SaveNow();

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
extern volatile bool g_stealMouse;
extern volatile long g_mouseDx, g_mouseDy;

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
void Act_PartyCmd(uintptr_t npc, int cmd);   // 0 follow 1 wait 2 to post 3 summon 4 tp to 5 kick
void Act_Photo(uintptr_t npc, bool force = false);   // portrait request (0 = current subject)
void Act_PhotoItem(int sym);                          // item icon request
void Act_PhotoBatch(int what);                        // 0 all npcs in world, 1 all items
void Act_PhotoCancel();
void Act_LookPreview();
void Act_RemoveSpawned(uintptr_t p);   // 0 = all
void Act_SpawnByName(const std::string& id, int count, int mode);
void Act_SaveTargetPtr(uintptr_t p);
void Act_PhotoKey(const std::string& key);
void Arena_PlayerDown();
bool Arena_IsEnemy(void* n);
void Act_CastOnTarget();
void Act_SelectTargetFor(int savedIdx);              // -1 = npc under crosshair
void Act_SetName(const std::string& utf8);
void Act_ApplyLook();
void Act_UpdatePos(int idx, const SavedPos& p);
void Act_SelectSavedPtr(uintptr_t p);
void CardTitle(const char* t);       // following rows of the module go into this card

void InstallGameHooks();
