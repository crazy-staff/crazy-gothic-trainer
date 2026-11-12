// Gothic 1 (1.08k_mod, GothicMod.exe) engine bindings.
// All addresses were verified by disassembling the user's GothicMod.exe
// (md5 aff73a93a00bb0f350b8ebeef0cd6244). Script-visible fields are resolved
// at runtime through the Daedalus symbol table, so mods that change scripts still work.
#pragma once
#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace g1 {

template <class T> inline T& at(uintptr_t a) { return *reinterpret_cast<T*>(a); }
template <class T> inline T& fld(const void* base, uintptr_t off) { return *reinterpret_cast<T*>((uintptr_t)base + off); }

// ---- globals -------------------------------------------------------------
constexpr uintptr_t OGAME_PTR      = 0x8DA6BC; // oCGame*
constexpr uintptr_t PARSER         = 0x8DCE08; // zCParser (object)
constexpr uintptr_t CUR_PARSER     = 0x8DDFD4; // zCParser* used by externals
constexpr uintptr_t PLAYER_PTR     = 0x8DBBB0; // oCNpc::player
constexpr uintptr_t GODMODE        = 0x8DBB98; // oCNpc::godmode (checked in ChangeAttribute)
constexpr uintptr_t ZTIMER         = 0x8CF1E8; // zCTimer (factorMotion @+0)
constexpr uintptr_t ZINPUT_PTR     = 0x86CCA0; // zCInput*
constexpr uintptr_t DI_MOUSE       = 0x86D2FC; // IDirectInputDevice7*
constexpr uintptr_t DI_KBD         = 0x86D2F0;
constexpr uintptr_t ACTIVE_CAM_PTR = 0x873240; // zCCamera::activeCam

// ---- functions -----------------------------------------------------------
constexpr uintptr_t F_ZSTRING_CTOR      = 0x4013A0; // std::string(const char*) on zSTRING+4
constexpr uintptr_t F_ZSTRING_DTOR      = 0x401260; // zSTRING::~zSTRING
constexpr uintptr_t F_OCGAME_RENDER     = 0x63DBE0;
constexpr uintptr_t F_CONSOLE_EVAL      = 0x641420; // static oCGame::ConsoleEvalFunc(zSTRING const&, zSTRING&)
constexpr uintptr_t F_NPC_CHANGEATTR    = 0x68D7C0; // oCNpc::ChangeAttribute(int,int)
constexpr uintptr_t F_NPC_CREATEITEMS   = 0x68E050; // oCNpc::CreateItems(int inst,int amount)
constexpr uintptr_t F_NPC_DODIE         = 0x6934A0; // oCNpc::DoDie(oCNpc* killer)
constexpr uintptr_t F_NPC_SETASPLAYER   = 0x69EAE0; // oCNpc::SetAsPlayer()
constexpr uintptr_t F_SPAWN_NPC_POS     = 0x6D0710; // oCSpawnManager::SpawnNpc(oCNpc*, zVEC3 const&, float)
constexpr uintptr_t F_VOB_SETPOSWORLD   = 0x5EE650; // zCVob::SetPositionWorld(zVEC3 const&)
constexpr uintptr_t F_VOB_SETSLEEPING   = 0x5D7250; // zCVob::SetSleeping(int)
constexpr uintptr_t F_WORLD_ADDVOB      = 0x5F6340; // zCWorld::AddVob(zCVob*)
constexpr uintptr_t F_INPUT_SETDEVICE   = 0x4C8710; // zCInput_Win32::SetDeviceEnabled(int,int)
constexpr uintptr_t F_INPUT_CLEARKEYBUF = 0x4C8AE0;
constexpr uintptr_t F_PARSER_DOSTACK    = 0x6E8600; // zCParser::DoStack(int pc)

// ---- vtable slots ------------------------------------------------------------
constexpr int VT_GAME_GETGAMEWORLD = 0x8C / 4; // oCGame::GetGameWorld
constexpr int VT_WORLD_CREATEVOB   = 0x64 / 4; // oCWorld::CreateVob(type, inst)
constexpr int VT_WORLD_REMOVEVOB   = 0x30 / 4; // oCWorld::RemoveVob(zCVob*)
constexpr int VT_OBJ_DELETE        = 0x0C / 4; // scalar deleting dtor

constexpr uintptr_t VTBL_OCNPC          = 0x7DDF34;
constexpr uintptr_t VTBL_OCITEM         = 0x7DD0CC;
constexpr uintptr_t VTBL_OCMOBCONTAINER = 0x7DD3FC;

// ---- object offsets --------------------------------------------------------
constexpr uintptr_t OG_TESTMODE   = 0xB0;  // oCGame::game_testmode (marvin)
constexpr uintptr_t OG_SHOWWAYNET = 0xAC;
constexpr uintptr_t OG_FRAMEINFO  = 0xA4;
constexpr uintptr_t OG_WLDTIMER   = 0x110; // oCWorldTimer* {float worldTime, int day}
constexpr uintptr_t OG_SPAWNMAN   = 0x134;
constexpr uintptr_t OG_INSCRIPTSTARTUP = 0x24;
constexpr uintptr_t OG_INLOADSAVE      = 0x28;

constexpr uintptr_t W_WAYNET     = 0x78;
constexpr uintptr_t W_VOBLIST    = 0x6268; // zCListSort<zCVob>*
constexpr uintptr_t W_NPCLIST    = 0x626C; // zCListSort<oCNpc>*
constexpr uintptr_t W_ITEMLIST   = 0x6270;
constexpr uintptr_t WN_WPLIST    = 0x28;   // zCListSort<zCWaypoint> (embedded)
constexpr uintptr_t WP_POS       = 0x44;
constexpr uintptr_t WP_NAME      = 0x5C;

constexpr uintptr_t VOB_TRAFO    = 0x3C;   // zMAT4 row-major, pos at [3][7][11]
constexpr uintptr_t VOB_FLAGS    = 0xE4;
constexpr uint32_t  VOBF_PHYSICS = 0x08, VOBF_COLLSTAT = 0x40, VOBF_COLLDYN = 0x80;
constexpr uintptr_t NPC_FLAGS    = 0x180;
constexpr uintptr_t NPC_ATTR     = 0x184;  // int[8]
constexpr uintptr_t NPC_FOCUSVOB = 0x9E4;

constexpr uintptr_t CAM_XCENTER  = 0x98, CAM_YCENTER = 0x9C, CAM_XDIM = 0x70, CAM_YDIM = 0x74;
constexpr uintptr_t CAM_MATRIX   = 0xA4;   // world->camera
constexpr uintptr_t CAM_VDISTX   = 0x8E8, CAM_VDISTY = 0x8EC;

constexpr float TICKS_PER_HOUR = 250000.f;

struct zVEC3 { float x, y, z; };

// MSVC6 zSTRING layout: vtbl + std::string{allocator byte, ptr, len, res}
struct zSTRING {
    uint32_t vt; uint8_t al; char* ptr; int len; int res;
};
struct ZStr {  // RAII wrapper constructed by the engine
    zSTRING s;
    explicit ZStr(const char* c);
    ~ZStr();
    ZStr(const ZStr&) = delete; ZStr& operator=(const ZStr&) = delete;
};
inline std::string zs(const zSTRING& s) { return s.ptr ? std::string(s.ptr, s.len) : std::string(); }

// ---- engine state helpers ---------------------------------------------------
void* Game();
void* Player();
void* GameWorld();
bool  InGame();             // world loaded, player present, not loading
zVEC3 Pos(const void* vob);
zVEC3 AtVec(const void* vob); // forward (z axis)
void  SetPos(void* vob, const zVEC3& p);
bool  IsNpc(const void* v);
bool  IsItem(const void* v);
void  Release(void* obj);
float Dist(const zVEC3& a, const zVEC3& b);

template <class F> void ForEachList(void* list, F f) {
    if (!list) return;
    void* node = fld<void*>(list, 8);
    int guard = 0;
    while (node && guard++ < 200000) {
        void* data = fld<void*>(node, 4);
        void* next = fld<void*>(node, 8);
        if (data) f(data);
        node = next;
    }
}
template <class F> void ForEachNpc(F f) { void* w = GameWorld(); if (w) ForEachList(fld<void*>(w, W_NPCLIST), f); }

// ---- parser / scripts -------------------------------------------------------
struct Sym {
    zSTRING name; void* next; int content; int offset; uint32_t bits;
    int filenr, line, line_anz, pos_beg, pos_anz; Sym* parent;
    int ele()  const { return bits & 0xfff; }
    int type() const { return (bits >> 12) & 0xf; }
    int flags()const { return (bits >> 16) & 0x3f; }
};
enum { T_VOID, T_FLOAT, T_INT, T_STRING, T_CLASS, T_FUNC, T_PROTO, T_INST };
enum { F_CONST = 1, F_RETURN = 2, F_CLASSVAR = 4, F_EXTERNAL = 8 };

int  SymCount();
Sym* SymAt(int i);
int  SymIndex(const std::string& upperName);   // -1 if missing
Sym* SymByName(const std::string& upperName);
int  ConstInt(const std::string& name, int def = 0);
int  FieldOff(const std::string& name, int def = -1); // "C_NPC.ATTRIBUTE" etc.
const uint8_t* Bytecode();
void RebuildSymbols();

struct Arg {
    enum K { I, F, S, N } k; int i; float f; std::string s; void* p;
    static Arg Int(int v) { Arg a; a.k = I; a.i = v; return a; }
    static Arg Flt(float v) { Arg a; a.k = F; a.f = v; return a; }
    static Arg Str(const std::string& v) { Arg a; a.k = S; a.s = v; return a; }
    static Arg Npc(void* v) { Arg a; a.k = N; a.p = v; return a; }
};
// Calls a Daedalus external (by name) or script function. Returns int result (if any).
int CallExt(const char* name, const std::vector<Arg>& args, bool* ok = nullptr);
int CallScript(const char* name, const std::vector<Arg>& args, bool* ok = nullptr);
std::string ConsoleExec(const std::string& cmd);

// ---- npc helpers -------------------------------------------------------------
int  Attr(const void* npc, int i);
void SetAttr(void* npc, int i, int v);
int  NpcGuild(const void* npc);
int  NpcInstance(const void* npc);
std::string NpcName(const void* npc);
int  NpcAivar(const void* npc, int i);
void SetNpcAivar(void* npc, int i, int v);
bool NpcDead(const void* npc);
void* FocusNpc();
void Kill(void* npc, void* killer, bool evenImmortal = true);
bool IsImmortal(const void* npc);
void SetAsPlayer(void* npc);
void CreateItems(void* npc, int inst, int amount);
void* SpawnNpc(int inst, const zVEC3& pos, bool noRoutine = false);
void RespawnNpc(void* npc, const zVEC3& pos);
extern bool g_homeWorldOk;
bool VobInWorld(void* v);
void EnsureNpcAt(void* npc, const zVEC3& pos);   // teleport, re-inserting the npc if the game disabled it
void* SpawnItem(int inst, const zVEC3& pos);
void RemoveVob(void* vob);
void SetSleeping(void* vob, bool s);
void ZAssign(zSTRING* z, const char* c);       // zSTRING::operator=(const char*)
void* VobModel(void* vob);                     // zCModel* of an npc (or null)
constexpr uintptr_t MODEL_TIMESCALE = 0x1D0;   // zCModel animation time scale (float)
constexpr uintptr_t F_NPC_ONDAMAGE = 0x7476A0; // oCNpc::OnDamage(src, inflictor, float dmg, int type, zVEC3 const& loc)
constexpr uintptr_t MOBC_CONTAINLIST = 0x254;  // oCMobContainer zCListSort<oCItem>

// ---- input -------------------------------------------------------------------
void SetGameInput(bool enabled);
void SetGameInput2(bool keyboard, bool mouse);

// ---- raytrace ------------------------------------------------------------------
enum { TR_NEAREST = 1, TR_VOB_IGNORE_NO_CD_DYN = 2, TR_VOB_IGNORE = 8, TR_STAT_POLY = 0x20,
       TR_POLY_IGNORE_TRANSP = 0x100, TR_POLY_TEST_WATER = 0x200 };
struct TraceHit { bool hit; void* vob; void* poly; zVEC3 pos; bool water; };
bool TraceRay(const zVEC3& start, const zVEC3& ray, int flags, TraceHit& out);
bool FindGround(const zVEC3& p, float above, float below, zVEC3& out, bool* water = nullptr);
void SafeSetPos(void* vob, const zVEC3& p);   // teleport without collision sweep
std::string WorldName();
constexpr uintptr_t F_NPC_CANSEE = 0x69E010;  // oCNpc::CanSee(zCVob*, int)
constexpr uintptr_t VOB_BBOX = 0x7C;          // zTBBox3D mins[3], maxs[3] (world)
constexpr uintptr_t VOB_SLEEPMODE = 0xEC;
constexpr uintptr_t NPC_ANICTRL = 0x9B4;          // oCAniCtrl_Human* (null until the npc is fully set up)
constexpr uintptr_t NPC_PERCLIST = 0x834, NPC_PERCCOUNT = 0x93C; // {int type, int func}[33]
constexpr uintptr_t ITEM_AMOUNT = 0x2E4;          // oCItem::amount (stacked items)
constexpr uintptr_t F_NPC_ONDAMAGE_DESC = 0x730FE0; // oCNpc::OnDamage(oSDamageDescriptor&) - real hit path (vtbl 0xB8)
constexpr uintptr_t F_NPC_CANSENSE = 0x69D020;    // oCNpc::CanSense(zCVob*) - sight/hearing/smell
constexpr uintptr_t F_NPC_PASSIVEPERC = 0x6B49C0; // oCNpc::CreatePassivePerception(int, zCVob*, zCVob*)
constexpr uintptr_t OG_CAMVOB = 0x14;             // zCSession::camVob
constexpr uintptr_t MODEL_NODELIST = 0x64;        // zCArray<zCModelNodeInst*> {arr, alloc, num}
bool NpcReady(void* npc);                          // has animation controller + model (safe for AI calls)

} // namespace g1
