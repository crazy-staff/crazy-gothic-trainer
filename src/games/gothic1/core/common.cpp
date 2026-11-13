#include <unordered_map>
#include "common.h"
#include "catalog.h"
#include "../trainer.h"
#include <algorithm>
#include <cmath>
#include <unordered_set>

extern HWND g_gameHwnd;

namespace gx {
Consts K;
std::mt19937 rng{ (unsigned)GetTickCount() };
std::vector<void*> g_savedTargets;
int g_activeSaved = -1;
std::vector<void*> g_allies;
static void* g_mainHero = nullptr;
struct Sched { double t; std::function<void()> f; };
static std::vector<Sched> g_sched;

void Consts::Refresh() {
    sepHum = ConstInt("GIL_SEPERATOR_HUM", 16); sepOrc = ConstInt("GIL_SEPERATOR_ORC", 37); gilMax = ConstInt("GIL_MAX", 42);
    attHostile = ConstInt("ATT_HOSTILE", 0); attNeutral = ConstInt("ATT_NEUTRAL", 2); attFriendly = ConstInt("ATT_FRIENDLY", 3);
    damFire = ConstInt("DAM_FIRE", 8); damMagic = ConstInt("DAM_MAGIC", 32);
    aivParty = ConstInt("AIV_PARTYMEMBER", 36);
    stAttack = SymIndex("ZS_ATTACK"); stMmAttack = SymIndex("ZS_MM_ATTACK"); stFollow = SymIndex("ZS_FOLLOWPC");
    stSummoned = SymIndex("ZS_MM_SUMMONEDBYPC"); stMmAll = SymIndex("ZS_MM_ALLSCHEDULER");
    instHero = SymIndex("PC_HERO"); nugget = SymIndex("ITMINUGGET");
    offGuild = FieldOff("C_NPC.GUILD", 0x1E8); offLevel = FieldOff("C_NPC.LEVEL", 0x1EC); offStartAi = FieldOff("C_NPC.START_AISTATE", -1);
    offItemName = FieldOff("C_ITEM.NAME", -1); offItemValue = FieldOff("C_ITEM.VALUE", -1); offItemAmount = FieldOff("C_ITEM.AMOUNT", -1);
}

std::string U(const std::string& s) { return Cp1250ToUtf8(s); }
bool IsMonster(void* npc) { return fld<int>(npc, K.offGuild) > K.sepHum; }
// npc pointers of the current frame: NpcExists used to walk the whole npc list on every call
// (called per npc in many loops -> O(n^2) spikes)
static std::unordered_set<void*> g_npcSet; static double g_npcSetAt = -1;
void RefreshNpcSet() {
    g_npcSet.clear();
    ForEachNpc([&](void* x) { g_npcSet.insert(x); });
    g_npcSetAt = T.gameClock;
}
bool NpcExists(void* n) {
    if (!n) return false;
    return g_npcSet.count(n) != 0;
}
bool NpcAlive(void* n) { return NpcExists(n) && !NpcDead(n); }

void* MainHero() { return NpcExists(g_mainHero) ? g_mainHero : nullptr; }
bool Swapped() { void* m = MainHero(); return m && Player() != m; }
void TrackMainHero() {
    void* p = Player();
    if (!p) return;
    static double lastScan = 0;
    if (!g_mainHero || !NpcExists(g_mainHero)) {
        g_mainHero = nullptr;
        if (NpcInstance(p) == K.instHero) { g_mainHero = p; return; }
        if (NowSec() - lastScan < 1.0) return;
        lastScan = NowSec();
        ForEachNpc([&](void* n) { if (!g_mainHero && NpcInstance(n) == K.instHero) g_mainHero = n; });
    }
}

void* SavedTarget() {
    // drop dead references
    g_savedTargets.erase(std::remove_if(g_savedTargets.begin(), g_savedTargets.end(), [](void* n) { return !NpcExists(n); }),
                         g_savedTargets.end());
    if (g_activeSaved >= (int)g_savedTargets.size()) g_activeSaved = (int)g_savedTargets.size() - 1;
    return g_activeSaved >= 0 ? g_savedTargets[g_activeSaved] : nullptr;
}
void* g_targetOverride = nullptr;   // build 14: Otoczenie runs target actions on a picked NPC
void* Target() {
    if (g_targetOverride && NpcExists(g_targetOverride)) return g_targetOverride;
    if (void* s = SavedTarget()) return s;
    return FocusNpc();
}

void* Subject() {
    int s = (int)Val("subject");
    if (s == 1 && MainHero()) return MainHero();
    if (s == 2) if (void* t = Target()) return t;
    return Player();
}
// build 16: whom an NPC is fighting (Npc_GetTarget fills the script global OTHER - restored right after)
void* NpcTarget(void* n) {
    static Sym* other = nullptr; if (!other) other = SymByName("OTHER");
    if (!other || !NpcReady(n)) return nullptr;
    int saved = other->offset;
    void* t = nullptr;
    if (CallExt("Npc_GetTarget", { Arg::Npc(n) })) t = (void*)(uintptr_t)other->offset;
    other->offset = saved;
    return (t && t != n && NpcExists(t)) ? t : nullptr;
}
bool NpcFighting(void* n) {
    if (!NpcReady(n)) return false;
    return (K.stAttack >= 0 && CallExt("Npc_IsInState", { Arg::Npc(n), Arg::Int(K.stAttack) })) ||
           (K.stMmAttack >= 0 && CallExt("Npc_IsInState", { Arg::Npc(n), Arg::Int(K.stMmAttack) }));
}
bool IsAlly(void* n) { return std::find(g_allies.begin(), g_allies.end(), n) != g_allies.end(); }
// AI calls on an npc whose animation controller/model is not set up yet (fresh spawn, far away)
// crash inside the engine (build 3: arena/spawn/peace). Such npcs are retried a bit later.
static void Retry(void* npc, int tries, void (*f)(void*, int)) {
    if (tries <= 0) return;
    Schedule(0.25, [npc, tries, f] { if (NpcAlive(npc)) Guard("ai:ally-retry", [&] { f(npc, tries - 1); }); });
}
static void MakeAllyT(void* npc, int tries);
void MakeAlly(void* npc) { MakeAllyT(npc, 12); }
static void MakeAllyT(void* npc, int tries) {
    if (!npc || NpcDead(npc)) return;
    if (!NpcReady(npc)) { if (!IsAlly(npc)) g_allies.push_back(npc); Retry(npc, tries, MakeAllyT); return; }
    CallExt("Npc_SetAttitude", { Arg::Npc(npc), Arg::Int(K.attFriendly) });
    CallExt("Npc_SetTempAttitude", { Arg::Npc(npc), Arg::Int(K.attFriendly) });
    CallExt("Npc_ClearAIQueue", { Arg::Npc(npc) });
    if (IsMonster(npc)) {
        SetNpcAivar(npc, 20, 300); SetNpcAivar(npc, 21, 5); SetNpcAivar(npc, 23, 1);
        if (K.stSummoned >= 0) CallExt("AI_StartState", { Arg::Npc(npc), Arg::Int(K.stSummoned), Arg::Int(1), Arg::Str("") });
    } else {
        SetNpcAivar(npc, K.aivParty, 1);
        if (K.stFollow >= 0) CallExt("AI_StartState", { Arg::Npc(npc), Arg::Int(K.stFollow), Arg::Int(1), Arg::Str("") });
    }
    if (!IsAlly(npc)) g_allies.push_back(npc);
}
static void AggroT(void* npc, void* victim, int tries);
void Aggro(void* npc, void* victim) { AggroT(npc, victim, 12); }
static void AggroT(void* npc, void* victim, int tries) {
    if (!npc || !victim || NpcDead(npc)) return;
    if (!NpcReady(npc) || !NpcReady(victim)) {
        if (tries > 0) Schedule(0.25, [npc, victim, tries] {
            if (NpcAlive(npc) && NpcExists(victim)) Guard("ai:aggro-retry", [&] { AggroT(npc, victim, tries - 1); }); });
        return;
    }
    CallExt("Npc_SetAttitude", { Arg::Npc(npc), Arg::Int(K.attHostile) });
    CallExt("Npc_SetTempAttitude", { Arg::Npc(npc), Arg::Int(K.attHostile) });
    CallExt("Npc_ClearAIQueue", { Arg::Npc(npc) });
    CallExt("Npc_SetTarget", { Arg::Npc(npc), Arg::Npc(victim) });
    int st = IsMonster(npc) ? K.stMmAttack : K.stAttack;
    if (st >= 0) CallExt("AI_StartState", { Arg::Npc(npc), Arg::Int(st), Arg::Int(0), Arg::Str("") });
}
void Peace(void* npc) {
    if (!npc || NpcDead(npc) || IsAlly(npc) || npc == Player() || npc == MainHero() || !NpcReady(npc)) return;
    void* h = Player();
    CallExt("Npc_ClearAIQueue", { Arg::Npc(npc) });
    int perm = CallExt("Npc_GetPermAttitude", { Arg::Npc(npc), Arg::Npc(h) });
    CallExt("Npc_SetTempAttitude", { Arg::Npc(npc), Arg::Int(perm) });
    if (IsMonster(npc)) {
        int st = K.offStartAi >= 0 ? fld<int>(npc, K.offStartAi) : -1;
        if (st <= 0) st = K.stMmAll;
        if (st > 0) CallExt("AI_StartState", { Arg::Npc(npc), Arg::Int(st), Arg::Int(0), Arg::Str("") });
    } else {
        CallExt("AI_ContinueRoutine", { Arg::Npc(npc) });
    }
}
void PeaceAll(float radius) { NpcsInRadius(radius, [](void* n) { Peace(n); }); }

void Effect(const char* fx, void* origin, void* target, int lvl, int dmg, int dtype, bool proj) {
    CallExt("Wld_PlayEffect", { Arg::Str(fx), Arg::Npc(origin), Arg::Npc(target), Arg::Int(lvl), Arg::Int(dmg),
                                Arg::Int(dtype), Arg::Int(proj ? 1 : 0) });
}
zVEC3 InFront(float dist) {
    void* h = Player();
    zVEC3 p = Pos(h), a = AtVec(h);
    return { p.x + a.x * dist, p.y + 30.f, p.z + a.z * dist };
}
zVEC3 Grounded(zVEC3 p, bool* water) {
    zVEC3 g;
    if (FindGround(p, 400.f, 3000.f, g, water)) return { g.x, g.y + 30.f, g.z };
    if (water) *water = false;
    return p;
}
zVEC3 SpawnPointNear(float minR, float maxR, float ang) {
    zVEC3 hp = Pos(Player());
    float r = minR + (maxR - minR) * (float)(rng() % 1000) / 1000.f;
    for (int tries = 0; tries < 8; tries++) {
        zVEC3 p{ hp.x + std::cos(ang) * r, hp.y + 50.f, hp.z + std::sin(ang) * r };
        bool water = false;
        zVEC3 g;
        if (!FindGround(p, 600.f, 2500.f, g, &water)) { if (r > 250) { r -= 150; continue; } return p; }
        if (!water) return { g.x, g.y + 40.f, g.z };
        r -= 150.f;
        if (r < 150.f) break;
    }
    return { hp.x + std::cos(ang) * 150.f, hp.y + 50.f, hp.z + std::sin(ang) * 150.f };
}

void Schedule(double d, std::function<void()> f) { g_sched.push_back({ T.gameClock + d, std::move(f) }); }
void RunSchedule() {
    double now = T.gameClock;
    std::vector<Sched> due;
    for (auto it = g_sched.begin(); it != g_sched.end();) {
        if (it->t <= now) { due.push_back(std::move(*it)); it = g_sched.erase(it); } else ++it;
    }
    for (auto& d : due) d.f();
}
void ModelCacheClear();
void ClearWorldState() {
    ModelCacheClear(); g_savedTargets.clear(); g_activeSaved = -1; g_allies.clear(); g_sched.clear(); g_mainHero = nullptr;
}

void DealDamage(void* npc, int dmg, void* killer) {
    if (!npc || NpcDead(npc) || dmg <= 0) return;
    if (npc == MainHero() && On("god")) return;
    int hp = Attr(npc, 0) - dmg;
    if (hp > 0) { SetAttr(npc, 0, hp); return; }
    KillClean(npc, killer);
}
// Kill that also works on NPCs stuck in a scripted state (Thorus & co. stayed standing and lootable):
// drop their AI queue first, and if the body is still not lying a moment later, force the death animation.
void KillClean(void* npc, void* killer) {
    if (!npc || NpcDead(npc) || !NpcReady(npc)) return;
    { CallExt("Npc_ClearAIQueue", { Arg::Npc(npc) }); CallExt("AI_StandupQuick", { Arg::Npc(npc) }); }
    Kill(npc, killer);
    Schedule(0.4, [npc] {
        if (!NpcExists(npc) || !NpcReady(npc) || !NpcDead(npc)) return;
        CallExt("Npc_ClearAIQueue", { Arg::Npc(npc) });
        CallExt("AI_PlayAni", { Arg::Npc(npc), Arg::Str("T_DEAD") });
    });
}
bool Project(const zVEC3& w, float& sx, float& sy) {
    void* cam = at<void*>(ACTIVE_CAM_PTR);
    if (!cam) return false;
    const float* M = &fld<float>(cam, CAM_MATRIX);
    float vx = M[0] * w.x + M[1] * w.y + M[2] * w.z + M[3];
    float vy = M[4] * w.x + M[5] * w.y + M[6] * w.z + M[7];
    float vz = M[8] * w.x + M[9] * w.y + M[10] * w.z + M[11];
    if (vz < 30.f) return false;
    float xc = fld<float>(cam, CAM_XCENTER), yc = fld<float>(cam, CAM_YCENTER);
    float dx = fld<float>(cam, CAM_VDISTX), dy = fld<float>(cam, CAM_VDISTY);
    float xd = (float)fld<int>(cam, CAM_XDIM), yd = (float)fld<int>(cam, CAM_YDIM);
    if (xd <= 0 || yd <= 0) return false;
    sx = (xc + vx * dx / vz) / xd;
    sy = (yc - vy * dy / vz) / yd;
    return sx > -0.2f && sx < 1.2f && sy > -0.2f && sy < 1.2f;
}

std::string InstName(void* npc) {
    Sym* s = SymAt(NpcInstance(npc));
    return s ? zs(s->name) : std::string("?");
}
std::string PlaceName(const zVEC3& p) {
    const WpEntry* best = nullptr; float bd = 1e30f;
    for (auto& w : g_cat.wps) {
        float dx = w.x - p.x, dy = w.y - p.y, dz = w.z - p.z, d = dx * dx + dy * dy * 0.3f + dz * dz;
        if (d < bd) { bd = d; best = &w; }
    }
    if (!best) return "";
    std::string r = RegionOfWp(best->name);
    return r.empty() ? best->name : r + " (" + best->name + ")";
}
std::string RegionOfWp(const std::string& n) {
    static const struct { const char* pre; const char* name; } kMap[] = {
        { "OCC_", "Stary Obóz - zamek" }, { "OCR_", "Stary Obóz" }, { "OC_", "Stary Obóz" }, { "OC1", "Stary Obóz" },
        { "NC_", "Nowy Obóz" }, { "NCI", "Nowy Obóz" }, { "NCE", "Nowy Obóz" }, { "NC", "Nowy Obóz" },
        { "PSI", "Obóz Bractwa" }, { "OM", "Stara Kopalnia" }, { "FM_", "Wolna Kopalnia" }, { "DT_", "Wieża Xardasa" },
        { "TPL_", "Świątynia Śniącego" }, { "ORC_", "Ziemie orków" }, { "OW_ORC", "Ziemie orków" }, { "LOCATION_", "Dzicz" },
        { "SPAWN_", "Dzicz" }, { "OW_", "Górnicza Dolina" }, { "PATH_", "Górnicza Dolina" }, { "WP_", "Górnicza Dolina" },
    };
    for (auto& m : kMap) if (n.rfind(m.pre, 0) == 0) return m.name;
    return "";
}
WpLabel DescribeWp(const std::string& n) {
    static const struct { const char* tok; const char* word; const char* tag; } kTok[] = {
        { "MAINGATE", "brama główna", "Brama" }, { "SOUTHGATE", "brama południowa", "Brama" }, { "NORTHGATE", "brama północna", "Brama" },
        { "BACKGATE", "tylna brama", "Brama" }, { "GATE", "brama", "Brama" }, { "TOWER", "wieża", "Wieża" }, { "WALL", "mur", "Mur" },
        { "HUT", "chata", "Chata" }, { "HOUSE", "dom", "Chata" }, { "TENT", "namiot", "Chata" }, { "ROUND", "pierścień chat", "Chata" },
        { "CASTLE", "zamek", "Zamek" }, { "BARONS", "baronowie", "Zamek" }, { "GREATHALL", "wielka sala", "Zamek" }, { "THRONE", "sala tronowa", "Zamek" },
        { "ARENA", "arena", "Arena" }, { "BAR", "karczma", "Karczma" }, { "TAVERN", "karczma", "Karczma" }, { "PUB", "karczma", "Karczma" },
        { "KITCHEN", "kuchnia", "Karczma" }, { "SMITH", "kuźnia", "Rzemiosło" }, { "ALCHEMY", "alchemik", "Rzemiosło" }, { "MARKET", "targ", "Targ" },
        { "TEMPLE", "świątynia", "Świątynia" }, { "SHRINE", "kapliczka", "Świątynia" }, { "SLEEPER", "Śniący", "Świątynia" },
        { "MINE", "kopalnia", "Kopalnia" }, { "SHAFT", "szyb", "Kopalnia" }, { "CAVE", "jaskinia", "Jaskinia" }, { "GROTTO", "grota", "Jaskinia" },
        { "DUNGEON", "loch", "Jaskinia" }, { "CRYPT", "krypta", "Jaskinia" }, { "LAKE", "jezioro", "Woda" }, { "RIVER", "rzeka", "Woda" },
        { "SEA", "morze", "Woda" }, { "BEACH", "plaża", "Woda" }, { "SHORE", "brzeg", "Woda" }, { "WATER", "woda", "Woda" },
        { "BRIDGE", "most", "Most" }, { "FOREST", "las", "Las" }, { "WOOD", "las", "Las" }, { "SWAMP", "bagno", "Bagno" },
        { "PATH", "ścieżka", "Droga" }, { "ROAD", "droga", "Droga" }, { "WAY", "droga", "Droga" }, { "PLATEAU", "płaskowyż", "Wzgórza" },
        { "MOUNTAIN", "góra", "Wzgórza" }, { "HILL", "wzgórze", "Wzgórza" }, { "CANYON", "kanion", "Wzgórza" }, { "VALLEY", "dolina", "Wzgórza" },
        { "ORC", "orkowie", "Orkowie" }, { "TROLL", "troll", "Potwory" }, { "SPAWN", "legowisko", "Potwory" }, { "MONSTER", "potwory", "Potwory" },
        { "STONEHENGE", "kamienny krąg", "Ruiny" }, { "RUIN", "ruiny", "Ruiny" }, { "FIRE", "ognisko", "Obóz" }, { "CAMP", "obóz", "Obóz" },
        { "GUARD", "straż", "Straż" }, { "WATCH", "posterunek", "Straż" }, { "TRAIN", "trening", "Trening" }, { "XARDAS", "Xardas", "Wieża" },
        { "EXCHANGE", "punkt wymiany", "Targ" }, { "LIBRARY", "biblioteka", "Świątynia" }, { "ENTRANCE", "wejście", "" }, { "CENTER", "środek", "" },
    };
    WpLabel L;
    L.region = RegionOfWp(n);
    std::string up = n;
    std::vector<std::string> words;
    for (auto& t : kTok) {
        if (up.find(t.tok) == std::string::npos) continue;
        bool dup = false; for (auto& w : words) if (w == t.word) dup = true;
        if (!dup && words.size() < 3) words.push_back(t.word);
        if (t.tag[0] && std::find(L.tags.begin(), L.tags.end(), t.tag) == L.tags.end()) L.tags.push_back(t.tag);
    }
    std::string num;
    for (size_t i = n.size(); i > 0 && isdigit((unsigned char)n[i - 1]); i--) num.insert(num.begin(), n[i - 1]);
    std::string t;
    for (auto& w : words) { if (!t.empty()) t += ", "; t += w; }
    if (t.empty()) t = "punkt";
    t[0] = (char)toupper((unsigned char)t[0]);
    if (!num.empty()) t += " " + num;
    L.title = t;
    if (!L.region.empty()) L.tags.insert(L.tags.begin(), L.region);
    return L;
}
namespace {
struct M34 { float m[12]; };
M34 Mul(const M34& a, const float* b) {
    M34 r;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 4; j++)
            r.m[i * 4 + j] = a.m[i * 4 + 0] * b[0 * 4 + j] + a.m[i * 4 + 1] * b[1 * 4 + j] + a.m[i * 4 + 2] * b[2 * 4 + j] + (j == 3 ? a.m[i * 4 + 3] : 0.f);
    return r;
}
}
// Skeleton topology (parents, names) never changes for a model, so it is validated once and cached;
// per frame only the matrices are multiplied. Build 6 re-validated every node with IsBadReadPtr every
// frame for every NPC on screen, which is what made the ESP stutter.
namespace {
struct Topo { void** arr = nullptr; int num = 0; std::vector<int> parent; std::vector<std::string> names; double at = 0; };
std::unordered_map<void*, Topo> g_topo;
const Topo* TopoOf(void* model) {
    void** arr = fld<void**>(model, MODEL_NODELIST);
    int num = fld<int>(model, MODEL_NODELIST + 8);
    double now = NowSec();
    auto it = g_topo.find(model);
    if (it != g_topo.end() && it->second.arr == arr && it->second.num == num && now - it->second.at < 10.0)
        return it->second.parent.empty() ? nullptr : &it->second;
    if (g_topo.size() > 400) g_topo.clear();
    Topo& t = g_topo[model];
    t = Topo{}; t.arr = arr; t.num = num; t.at = now;
    if (!arr || num <= 0 || num > 200 || IsBadReadPtr(arr, num * 4)) return nullptr;
    std::unordered_map<void*, int> idx;
    for (int i = 0; i < num; i++) { if (!arr[i] || IsBadReadPtr(arr[i], 0x4C)) return nullptr; idx[arr[i]] = i; }
    t.parent.assign(num, -1); t.names.assign(num, std::string());
    for (int i = 0; i < num; i++) {
        void* p = fld<void*>(arr[i], 0);
        if (p) { auto f = idx.find(p); t.parent[i] = f == idx.end() ? -1 : f->second; }
        void* proto = fld<void*>(arr[i], 4);
        if (proto && !IsBadReadPtr(proto, 0x18)) {
            const zSTRING& nm = fld<zSTRING>(proto, 4);
            if (nm.ptr && nm.len > 0 && nm.len < 64 && !IsBadReadPtr(nm.ptr, nm.len)) t.names[i].assign(nm.ptr, nm.len);
        }
    }
    return &t;
}
}
void ModelCacheClear() { g_topo.clear(); }
bool ModelNodes(void* npc, std::vector<ModelNode>& out) {
    out.clear();
    void* model = VobModel(npc);
    if (!model) return false;
    const Topo* t = TopoOf(model);
    if (!t) return false;
    int num = t->num;
    std::vector<M34> om(num);
    std::vector<char> done(num, 0);
    out.resize(num);
    M34 world; const float* W = &fld<float>(npc, VOB_TRAFO); for (int k = 0; k < 12; k++) world.m[k] = W[k];
    zVEC3 base = Pos(npc);
    for (int i = 0; i < num; i++) out[i].parent = -1;
    for (int pass = 0; pass < 6; pass++) {
        bool all = true;
        for (int i = 0; i < num; i++) {
            if (done[i]) continue;
            const float* local = &fld<float>(t->arr[i], 0xC);
            int pi = t->parent[i];
            if (pi < 0) { om[i] = Mul(world, local); done[i] = 1; continue; }
            if (!done[pi]) { all = false; continue; }
            om[i] = Mul(om[pi], local); out[i].parent = pi; done[i] = 1;
        }
        if (all) break;
    }
    for (int i = 0; i < num; i++) {
        out[i].pos = { om[i].m[3], om[i].m[7], om[i].m[11] };
        if (!done[i] || Dist(out[i].pos, base) > 800.f) out[i].parent = -2;
        out[i].name = t->names[i];
        if (out[i].name.rfind("ZS_", 0) == 0) out[i].parent = -2;
    }
    return true;
}
bool HeadPos(void* npc, zVEC3& o) {
    std::vector<ModelNode> n;
    if (!ModelNodes(npc, n)) return false;
    for (auto& x : n) if (x.parent != -2 && x.name.find("HEAD") != std::string::npos) { o = x.pos; return true; }
    return false;
}
bool GameFocused() { return GetForegroundWindow() == g_gameHwnd; }
bool On(const char* id) { Row* r = T.Get(id); return r && r->on; }
float Val(const char* id) { Row* r = T.Get(id); return r ? r->val : 0.f; }
}
