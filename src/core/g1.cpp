#include "g1.h"
#include <cmath>
#include <algorithm>
#include <cstring>
#include <memory>

namespace g1 {

// ---- typedefs for engine calls ------------------------------------------------
using fn_strctor  = void(__thiscall*)(void*, const char*);
using fn_this     = void(__thiscall*)(void*);
using fn_this_i   = void(__thiscall*)(void*, int);
using fn_this_ii  = void(__thiscall*)(void*, int, int);
using fn_this_p   = void(__thiscall*)(void*, void*);
using fn_this_ret = int(__thiscall*)(void*);
using fn_vec      = void(__thiscall*)(void*, const zVEC3*);
using fn_spawn    = void(__thiscall*)(void*, void*, const zVEC3*, float);
using fn_create   = void*(__thiscall*)(void*, int, int);
using fn_ext      = int(__cdecl*)();
using fn_cons     = int(__cdecl*)(const zSTRING*, zSTRING*);

static inline void** vtbl(const void* o) { return *reinterpret_cast<void** const*>(o); }

ZStr::ZStr(const char* c) {
    memset(&s, 0, sizeof(s));
    s.vt = 0x7D06E8;
    reinterpret_cast<fn_strctor>(F_ZSTRING_CTOR)(&s, c ? c : "");
}
ZStr::~ZStr() { reinterpret_cast<fn_this>(F_ZSTRING_DTOR)(&s); }

void* Game() { return at<void*>(OGAME_PTR); }
void* Player() { return at<void*>(PLAYER_PTR); }
void* GameWorld() {
    void* g = Game();
    if (!g) return nullptr;
    using fn = void*(__thiscall*)(void*);
    return reinterpret_cast<fn>(vtbl(g)[VT_GAME_GETGAMEWORLD])(g);
}
bool InGame() {
    void* g = Game();
    if (!g || !Player() || !GameWorld()) return false;
    if (fld<int>(g, OG_INLOADSAVE) || fld<int>(g, OG_INSCRIPTSTARTUP)) return false;
    return SymCount() > 0;
}
zVEC3 Pos(const void* v) {
    const float* m = &fld<float>(v, VOB_TRAFO);
    return { m[3], m[7], m[11] };
}
zVEC3 AtVec(const void* v) {
    const float* m = &fld<float>(v, VOB_TRAFO);
    return { m[2], m[6], m[10] };
}
void SetPos(void* v, const zVEC3& p) { reinterpret_cast<fn_vec>(F_VOB_SETPOSWORLD)(v, &p); }
bool IsNpc(const void* v) { return v && *reinterpret_cast<const uintptr_t*>(v) == VTBL_OCNPC; }
bool IsItem(const void* v) { return v && *reinterpret_cast<const uintptr_t*>(v) == VTBL_OCITEM; }
void Release(void* o) {
    if (!o) return;
    int& rc = fld<int>(o, 4);
    if (--rc <= 0) reinterpret_cast<fn_this_i>(vtbl(o)[VT_OBJ_DELETE])(o, 1);
}
float Dist(const zVEC3& a, const zVEC3& b) {
    float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// ---- symbols -------------------------------------------------------------------
static std::unordered_map<std::string, int> g_symIdx;
static void* g_symArrayCached = nullptr;
static int g_symCountCached = 0;

int SymCount() { return at<int>(PARSER + 0x20); }
Sym* SymAt(int i) {
    if (i < 0 || i >= SymCount()) return nullptr;
    return at<Sym**>(PARSER + 0x18)[i];
}
void RebuildSymbols() {
    g_symIdx.clear();
    int n = SymCount();
    for (int i = 0; i < n; i++) {
        Sym* s = SymAt(i);
        if (!s || !s->name.ptr) continue;
        g_symIdx.emplace(zs(s->name), i);
    }
    g_symArrayCached = at<void*>(PARSER + 0x18);
    g_symCountCached = n;
}
static void EnsureSymbols() {
    if (at<void*>(PARSER + 0x18) != g_symArrayCached || SymCount() != g_symCountCached) RebuildSymbols();
}
int SymIndex(const std::string& n) {
    EnsureSymbols();
    auto it = g_symIdx.find(n);
    return it == g_symIdx.end() ? -1 : it->second;
}
Sym* SymByName(const std::string& n) { return SymAt(SymIndex(n)); }
int ConstInt(const std::string& n, int def) {
    Sym* s = SymByName(n);
    if (!s || s->type() != T_INT) return def;
    if (s->ele() > 1) return s->content ? *reinterpret_cast<int*>(s->content) : def;
    return s->content;
}
int FieldOff(const std::string& n, int def) {
    Sym* s = SymByName(n);
    if (!s || !(s->flags() & F_CLASSVAR)) return def;
    return s->offset;
}
const uint8_t* Bytecode() { return at<const uint8_t*>(PARSER + 0x48); }

// ---- calling scripts ---------------------------------------------------------------
static const char* kScratch[] = { "SELF", "OTHER", "VICTIM", "ITEM" };

static int Invoke(Sym* s, const std::vector<Arg>& args, bool isExt) {
    uintptr_t P = PARSER;
    int* ds = reinterpret_cast<int*>(P + 0x58);
    int& sp = at<int>(P + 0x1058);
    const int saveSp = sp;
    void* saveCur = at<void*>(CUR_PARSER);
    at<void*>(CUR_PARSER) = reinterpret_cast<void*>(P);

    std::vector<std::unique_ptr<ZStr>> strs;
    struct Saved { Sym* s; int off; };
    std::vector<Saved> saved;
    int scratchUsed = 0;
    for (const Arg& a : args) {
        if (sp > 1000) break;
        switch (a.k) {
        case Arg::I: ds[sp++] = a.i; ds[sp++] = 0x40; break;
        case Arg::F: { int b; memcpy(&b, &a.f, 4); ds[sp++] = b; ds[sp++] = 0x40; } break;
        case Arg::S: strs.emplace_back(new ZStr(a.s.c_str()));
                     ds[sp++] = (int)(uintptr_t)&strs.back()->s; ds[sp++] = 0x41; break;
        case Arg::N: {
            int idx = scratchUsed < 4 ? SymIndex(kScratch[scratchUsed]) : -1;
            scratchUsed++;
            Sym* ss = SymAt(idx);
            if (ss) { saved.push_back({ ss, ss->offset }); ss->offset = (int)(uintptr_t)a.p; }
            ds[sp++] = idx;
        } break;
        }
    }
    int ret = 0;
    if (isExt) {
        reinterpret_cast<fn_ext>(s->content)();
    } else {
        reinterpret_cast<fn_this_i>(F_PARSER_DOSTACK)(reinterpret_cast<void*>(P), s->content);
    }
    if (sp >= saveSp + 2) {
        int tok = ds[sp - 1], val = ds[sp - 2];
        ret = (tok == 0x41 && val) ? *reinterpret_cast<int*>(val) : val;
    } else if (sp == saveSp + 1) {
        ret = ds[sp - 1];
    }
    sp = saveSp;
    for (auto& v : saved) v.s->offset = v.off;
    at<void*>(CUR_PARSER) = saveCur;
    return ret;
}

int CallExt(const char* name, const std::vector<Arg>& args, bool* ok) {
    std::string n(name);
    std::transform(n.begin(), n.end(), n.begin(), ::toupper);
    Sym* s = SymByName(n);
    if (!s || s->type() != T_FUNC || !(s->flags() & F_EXTERNAL) || !s->content) { if (ok) *ok = false; return 0; }
    if (ok) *ok = true;
    return Invoke(s, args, true);
}
int CallScript(const char* name, const std::vector<Arg>& args, bool* ok) {
    std::string n(name);
    std::transform(n.begin(), n.end(), n.begin(), ::toupper);
    Sym* s = SymByName(n);
    if (!s || s->type() != T_FUNC) { if (ok) *ok = false; return 0; }
    if (ok) *ok = true;
    return Invoke(s, args, (s->flags() & F_EXTERNAL) != 0);
}
std::string ConsoleExec(const std::string& cmd) {
    ZStr in(cmd.c_str()), out("");
    reinterpret_cast<fn_cons>(F_CONSOLE_EVAL)(&in.s, &out.s);
    return zs(out.s);
}

// ---- npcs ------------------------------------------------------------------------
int Attr(const void* n, int i) { return fld<int>(n, NPC_ATTR + 4 * i); }
void SetAttr(void* n, int i, int v) { fld<int>(n, NPC_ATTR + 4 * i) = v; }
int NpcGuild(const void* n) {
    static int off = -1; if (off < 0) off = FieldOff("C_NPC.GUILD", 0x1E8);
    return fld<int>(n, off);
}
int NpcAivar(const void* n, int i) {
    static int off = -1; if (off < 0) off = FieldOff("C_NPC.AIVAR", 0x224);
    return fld<int>(n, off + 4 * i);
}
void SetNpcAivar(void* n, int i, int v) {
    static int off = -1; if (off < 0) off = FieldOff("C_NPC.AIVAR", 0x224);
    fld<int>(n, off + 4 * i) = v;
}
std::string NpcName(const void* n) {
    static int off = -1; if (off < 0) off = FieldOff("C_NPC.NAME", 0x104);
    return zs(fld<zSTRING>(n, off));
}
int NpcInstance(const void* n) {  // zCVob virtual GetScriptInstance(void*& sym, int& index) (slot 0x54), as Hlp_GetInstanceID
    if (!n) return -1;
    using fn = int(__thiscall*)(void*, void**, int*);
    void* a = nullptr; int idx = -1;
    reinterpret_cast<fn>(vtbl(n)[0x54 / 4])(const_cast<void*>(n), &a, &idx);
    return idx;
}
void ZAssign(zSTRING* z, const char* c) { reinterpret_cast<fn_strctor>(0x4C5820)(z, c); }
void* VobModel(void* vob) {
    void* v = fld<void*>(vob, 0xB8);
    return (v && *reinterpret_cast<uintptr_t*>(v) == 0x7D3FEC) ? v : nullptr;
}
bool NpcReady(void* n) {
    if (!IsNpc(n)) return false;
    return fld<void*>(n, NPC_ANICTRL) != nullptr && VobModel(n) != nullptr;
}
bool NpcDead(const void* n) { return reinterpret_cast<fn_this_ret>(0x693480)(const_cast<void*>(n)) != 0; }
void* FocusNpc() {
    void* p = Player();
    if (!p) return nullptr;
    void* f = fld<void*>(p, NPC_FOCUSVOB);
    return IsNpc(f) ? f : nullptr;
}
bool IsImmortal(const void* npc) { return (fld<int>(npc, NPC_FLAGS) & 2) != 0; }
void Kill(void* npc, void* killer, bool evenImmortal) {
    if (!npc || NpcDead(npc)) return;
    if (!evenImmortal && IsImmortal(npc)) return;
    if (!fld<void*>(npc, NPC_ANICTRL)) return;   // DoDie uses the animation controller -> null = engine crash
    fld<int>(npc, NPC_FLAGS) &= ~2;  // NPC_FLAG_IMMORTAL (gate guards etc.)
    SetAttr(npc, 0, 0);
    reinterpret_cast<fn_this_p>(F_NPC_DODIE)(npc, killer);
}
void SetAsPlayer(void* npc) { reinterpret_cast<fn_this>(F_NPC_SETASPLAYER)(npc); }
void CreateItems(void* npc, int inst, int amount) {
    reinterpret_cast<fn_this_ii>(F_NPC_CREATEITEMS)(npc, inst, amount);
}
void* SpawnNpc(int inst, const zVEC3& pos, bool noRoutine) {
    void* w = GameWorld(); void* g = Game();
    if (!w || !g) return nullptr;
    void* npc = reinterpret_cast<fn_create>(vtbl(w)[VT_WORLD_CREATEVOB])(w, 0x82, inst);
    if (!IsNpc(npc)) return nullptr;
    if (noRoutine) {
        // build 9 (#17): a named NPC with a daily routine got beamed to its routine waypoint on insertion; far
        // from the player the spawn manager disabled it at once (no model). Our teleport back then moved an
        // invisible, disabled NPC. Without a routine it stays where we put it, like monsters do.
        static int offRtn = FieldOff("C_NPC.DAILY_ROUTINE", -1);
        if (offRtn >= 0) fld<int>(npc, offRtn) = 0;
    }
    void* sm = fld<void*>(g, OG_SPAWNMAN);
    reinterpret_cast<fn_spawn>(F_SPAWN_NPC_POS)(sm, npc, &pos, 0.f);
    Release(npc);  // world holds its own reference
    return npc;
}
// put an existing (despawned) npc object back into the world
void RespawnNpc(void* npc, const zVEC3& pos) {
    void* g = Game(); if (!g || !npc) return;
    void* sm = fld<void*>(g, OG_SPAWNMAN);
    reinterpret_cast<fn_spawn>(F_SPAWN_NPC_POS)(sm, npc, &pos, 0.f);
}
void* SpawnItem(int inst, const zVEC3& pos) {
    void* w = GameWorld();
    if (!w) return nullptr;
    void* it = reinterpret_cast<fn_create>(vtbl(w)[VT_WORLD_CREATEVOB])(w, 0x81, inst);
    if (!it) return nullptr;
    SetPos(it, pos);
    reinterpret_cast<fn_this_p>(F_WORLD_ADDVOB)(w, it);
    Release(it);
    return it;
}
void RemoveVob(void* vob) {
    void* w = GameWorld();
    if (!w || !vob) return;
    reinterpret_cast<fn_this_p>(vtbl(w)[VT_WORLD_REMOVEVOB])(w, vob);
}
void SetSleeping(void* vob, bool s) { reinterpret_cast<fn_this_i>(F_VOB_SETSLEEPING)(vob, s ? 1 : 0); }

// ---- input -------------------------------------------------------------------------
struct IDIDev { void** vt; };
void SetGameInput(bool en) { SetGameInput2(en, en); }
void SetGameInput2(bool kbd, bool mouse) {
    void* in = at<void*>(ZINPUT_PTR);
    if (in) {
        reinterpret_cast<fn_this_ii>(F_INPUT_SETDEVICE)(in, 2, mouse ? 1 : 0);
        reinterpret_cast<fn_this_ii>(F_INPUT_SETDEVICE)(in, 1, kbd ? 1 : 0);
        if (!kbd) reinterpret_cast<fn_this>(F_INPUT_CLEARKEYBUF)(in);
    }
    // IDirectInputDevice7: Acquire = slot 7, Unacquire = slot 8
    using fn = long(__stdcall*)(void*);
    if (IDIDev* d = at<IDIDev*>(DI_MOUSE)) reinterpret_cast<fn>(d->vt[mouse ? 7 : 8])(d);
    if (IDIDev* d = at<IDIDev*>(DI_KBD)) reinterpret_cast<fn>(d->vt[kbd ? 7 : 8])(d);
}

// ---- raytrace: zCWorld::TraceRay (custom conv: ecx=world, edx=&start, stack: &ray, ignoreList*, flags)
bool TraceRay(const zVEC3& start, const zVEC3& ray, int flags, TraceHit& out) {
    void* w = GameWorld();
    if (!w) return false;
    struct { void** arr; int alloc, num; } ign{ nullptr, 0, 0 };
    void* hero = Player();
    void* one[1] = { hero };
    if (hero) { ign.arr = one; ign.alloc = 0; ign.num = 1; }
    zVEC3 st = start, ry = ray;
    struct { const void* ray; void* ign; int flags; uintptr_t fn; } blk{ &ry, &ign, flags, 0x5F40C0 };
    void* ecx_ = w; const zVEC3* edx_ = &st; void* eax_ = &blk;
    asm volatile(
        "pushl 8(%%eax)\n\t"
        "pushl 4(%%eax)\n\t"
        "pushl 0(%%eax)\n\t"
        "call *12(%%eax)\n\t"
        : "+a"(eax_), "+c"(ecx_), "+d"(edx_)
        :
        : "memory", "cc");
    out.hit = fld<int>(w, 0x38) != 0;
    out.vob = fld<void*>(w, 0x3C);
    out.poly = fld<void*>(w, 0x40);
    const float* f = &fld<float>(w, 0x44);
    out.pos = { f[0], f[1], f[2] };
    out.water = false;
    if (out.poly) {
        void* mat = fld<void*>(out.poly, 0x18);
        if (mat && fld<int>(mat, 0x40) == 5) out.water = true;
    }
    return out.hit;
}
bool FindGround(const zVEC3& p, float above, float below, zVEC3& out, bool* water) {
    TraceHit h;
    const int flags = TR_NEAREST | TR_VOB_IGNORE | TR_STAT_POLY | TR_POLY_IGNORE_TRANSP | TR_POLY_TEST_WATER;
    if (!TraceRay({ p.x, p.y + above, p.z }, { 0, -(above + below), 0 }, flags, h)) return false;
    out = h.pos;
    if (water) *water = h.water;
    return true;
}
void SafeSetPos(void* vob, const zVEC3& p) {
    uint32_t& f = fld<uint32_t>(vob, VOB_FLAGS);
    uint32_t old = f;
    f &= ~(VOBF_COLLSTAT | VOBF_COLLDYN);
    SetPos(vob, p);
    f = (f & ~(VOBF_COLLSTAT | VOBF_COLLDYN)) | (old & (VOBF_COLLSTAT | VOBF_COLLDYN));
}
std::string WorldName() {
    void* w = GameWorld();
    return w ? zs(fld<zSTRING>(w, 0x10)) : std::string();
}

} // namespace g1
