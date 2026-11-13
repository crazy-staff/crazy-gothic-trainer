// ESP module: labels/boxes for NPCs, items and chests. Expensive engine queries
// (attitude, names, equipment) are cached and refreshed a few times per second;
// only projection runs every frame.
#include "mods.h"
#include "gfx.h"
#include <psapi.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <unordered_map>

using namespace g1;
using namespace gx;
std::string Npcs_Kind(void* n);   // npcs.cpp

unsigned g_espColors[7] = { 0xFF6E6EFF, 0xFF8CF28C, 0xFF7AD8FF, 0xFF55AAFF, 0xFF9A9AA8, 0xFF7AE07A, 0xFF40C8FF };
// 0 enemy 1 ally 2 neutral 3 monster 4 dead 5 item 6 chest

namespace {
struct NpcInfo { std::string name, guild; unsigned color; bool hostile, monster, ally; std::vector<std::string> eq; double eqAt = 0, attAt = 0; void* target = nullptr; };
std::unordered_map<void*, NpcInfo> g_npc;
std::vector<void*> g_chests;
std::unordered_map<void*, std::pair<double, std::vector<std::string>>> g_chestInv;
double g_lastNpc = 0, g_lastChest = 0;

// ---- skeleton from the model node hierarchy (zCModel nodeList, local trafos composed to world)
struct M34 { float m[12]; };
M34 Mul(const M34& a, const float* b) {   // a (3x4) * b (4x4 row-major, affine)
    M34 r;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 4; j++)
            r.m[i * 4 + j] = a.m[i * 4 + 0] * b[0 * 4 + j] + a.m[i * 4 + 1] * b[1 * 4 + j] + a.m[i * 4 + 2] * b[2 * 4 + j] + (j == 3 ? a.m[i * 4 + 3] : 0.f);
    return r;
}
bool g_skelBroken = false;
bool Skeleton(void* npc, std::vector<zVEC3>& pts, std::vector<int>& par) {
    if (g_skelBroken) return false;
    void* model = VobModel(npc);
    if (!model) return false;
    void** arr = fld<void**>(model, MODEL_NODELIST);
    int num = fld<int>(model, MODEL_NODELIST + 8);
    if (!arr || num <= 0 || num > 200 || IsBadReadPtr(arr, num * 4)) return false;
    std::unordered_map<void*, int> idx;
    for (int i = 0; i < num; i++) { if (!arr[i] || IsBadReadPtr(arr[i], 0x4C)) return false; idx[arr[i]] = i; }
    std::vector<M34> om(num);
    std::vector<char> done(num, 0);
    pts.assign(num, { 0, 0, 0 }); par.assign(num, -1);
    M34 world; const float* W = &fld<float>(npc, VOB_TRAFO); for (int k = 0; k < 12; k++) world.m[k] = W[k];
    zVEC3 base = Pos(npc);
    for (int pass = 0; pass < 3; pass++)
        for (int i = 0; i < num; i++) {
            if (done[i]) continue;
            void* p = fld<void*>(arr[i], 0);
            const float* local = &fld<float>(arr[i], 0xC);
            auto it = p ? idx.find(p) : idx.end();
            if (it == idx.end()) { om[i] = Mul(world, local); done[i] = 1; continue; }
            if (!done[it->second]) continue;
            om[i] = Mul(om[it->second], local); par[i] = it->second; done[i] = 1;
        }
    for (int i = 0; i < num; i++) {
        pts[i] = { om[i].m[3], om[i].m[7], om[i].m[11] };
        if (!done[i] || Dist(pts[i], base) > 600.f) { par[i] = -2; }
    }
    // slot nodes (ZS_*) carry no bone geometry - drop their links
    for (int i = 0; i < num; i++) {
        void* proto = fld<void*>(arr[i], 4);
        if (!proto || IsBadReadPtr(proto, 0x18)) continue;
        const zSTRING& nm = fld<zSTRING>(proto, 4);
        if (nm.ptr && nm.len >= 3 && nm.len < 64 && !IsBadReadPtr(nm.ptr, nm.len) && nm.ptr[0] == 'Z' && nm.ptr[1] == 'S' && nm.ptr[2] == '_') par[i] = -2;
    }
    return true;
}
void Boxes(const void* vob, Label& L, int mode) {
    const float* b = &fld<float>(vob, VOB_BBOX);
    if (mode == 1) {   // silhouette: upright box from model height
        zVEC3 p = Pos(vob);
        float x0, y0, x1, y1;
        if (!Project({ p.x, b[4], p.z }, x0, y0) || !Project({ p.x, b[1], p.z }, x1, y1)) return;
        float hgt = y1 - y0; float wid = hgt * 0.42f * 0.5625f;   // aspect-corrected
        L.box = 1; L.bx0 = x0 - wid * 0.5f; L.bx1 = x0 + wid * 0.5f; L.by0 = y0; L.by1 = y1;
        return;
    }
    float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f; int ok = 0;
    for (int i = 0; i < 8; i++) {
        zVEC3 c{ b[(i & 1) ? 3 : 0], b[(i & 2) ? 4 : 1], b[(i & 4) ? 5 : 2] };
        float x, y;
        if (!Project(c, x, y)) return;
        L.corners[i * 2] = x; L.corners[i * 2 + 1] = y;
        ok++; x0 = std::min(x0, x); y0 = std::min(y0, y); x1 = std::max(x1, x); y1 = std::max(y1, y);
    }
    L.box = mode == 2 ? 2 : 1; L.bx0 = x0; L.by0 = y0; L.bx1 = x1; L.by1 = y1;
}
std::string ItemLine(void* it, int amt = -1) {
    std::string n = K.offItemName >= 0 ? U(zs(fld<zSTRING>(it, K.offItemName))) : "?";
    if (amt < 0) amt = fld<int>(it, ITEM_AMOUNT);
    if (amt > 1) n += " x" + std::to_string(amt);
    return n;
}
std::vector<std::string> NpcEquipment(void* npc) {
    std::vector<std::pair<int, std::string>> all;
    Sym* itemSym = SymByName("ITEM");
    static int flagsOff = -1; if (flagsOff < 0) flagsOff = FieldOff("C_ITEM.FLAGS", -1);
    for (int cat = 0; cat < 10; cat++)
        for (int slot = 0; slot < 60; slot++) {
            int amount = CallExt("Npc_GetInvItemBySlot", { Arg::Npc(npc), Arg::Int(cat), Arg::Int(slot) });
            if (amount <= 0) break;
            void* it = itemSym ? (void*)(uintptr_t)itemSym->offset : nullptr;
            if (!IsItem(it)) break;
            bool worn = flagsOff >= 0 && (fld<int>(it, flagsOff) & (1 << 30));
            int val = K.offItemValue >= 0 ? fld<int>(it, K.offItemValue) : 0;
            all.push_back({ (worn ? 1000000 : 0) + val, (worn ? "[założone] " : "") + ItemLine(it, amount) });
        }
    std::sort(all.begin(), all.end(), [](auto& a, auto& b) { return a.first > b.first; });
    std::vector<std::string> out;
    for (size_t i = 0; i < all.size() && i < 5; i++) out.push_back(all[i].second);
    if (all.size() > 5) out.push_back("... +" + std::to_string(all.size() - 5));
    return out;
}
std::vector<std::string> ChestContents(void* chest) {
    std::vector<std::string> out;
    void* node = fld<void*>(chest, MOBC_CONTAINLIST + 8);
    int n = 0, total = 0;
    while (node && total < 200) {
        void* it = fld<void*>(node, 4);
        if (IsItem(it)) { if (n < 6) out.push_back(ItemLine(it)); n++; }
        node = fld<void*>(node, 8); total++;
    }
    if (n == 0) out.push_back("(pusta)");
    else if (n > 6) out.push_back("... +" + std::to_string(n - 6));
    return out;
}
}

// build 17: where a point is on the screen even when it is outside it or behind the camera (for edge markers)
bool ProjectRaw(const zVEC3& w, float& sx, float& sy, bool& behind) {
    void* cam = at<void*>(ACTIVE_CAM_PTR);
    if (!cam) return false;
    const float* M = &fld<float>(cam, CAM_MATRIX);
    float vx = M[0] * w.x + M[1] * w.y + M[2] * w.z + M[3], vy = M[4] * w.x + M[5] * w.y + M[6] * w.z + M[7], vz = M[8] * w.x + M[9] * w.y + M[10] * w.z + M[11];
    float xc = fld<float>(cam, CAM_XCENTER), yc = fld<float>(cam, CAM_YCENTER), dx = fld<float>(cam, CAM_VDISTX), dy = fld<float>(cam, CAM_VDISTY);
    float xd = (float)fld<int>(cam, CAM_XDIM), yd = (float)fld<int>(cam, CAM_YDIM);
    if (xd <= 0 || yd <= 0) return false;
    behind = vz < 30.f;
    float z = std::max(std::fabs(vz), 30.f);
    float px = vx * dx / z, py = vy * dy / z;
    if (behind) { px = -px * 50.f; py = -py * 50.f; if (std::fabs(px) + std::fabs(py) < 1) py = yd; }   // mirrored and pushed out
    sx = (xc + px) / xd; sy = (yc - py) / yd;
    return true;
}
void Cap(std::vector<std::pair<float, Label>>& v, int maxN, Snapshot& s) {   // nearest first, at most maxN
    std::sort(v.begin(), v.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    if ((int)v.size() > maxN) { s.espHidden += (int)v.size() - maxN; v.resize(maxN); }
    for (auto& e : v) s.labels.push_back(std::move(e.second));
}
void Esp_Hooks() {}
void Esp_World() { g_npc.clear(); g_chests.clear(); g_chestInv.clear(); g_lastNpc = g_lastChest = 0; }

// build 14: radar dots (independent of the ESP switches)
void RadarTick(TickCtx& c) {
    Snapshot& s = c.s;
    void* h = c.hero; zVEC3 hp = Pos(h);
    const float* m = &fld<float>(h, VOB_TRAFO);
    float fl = std::sqrt(m[2] * m[2] + m[10] * m[10]); if (fl < 1e-3f) fl = 1;
    s.rfx = m[2] / fl; s.rfz = m[10] / fl;
    float rl = std::sqrt(m[0] * m[0] + m[8] * m[8]); if (rl < 1e-3f) rl = 1;
    s.rrx = m[0] / rl; s.rrz = m[8] / rl;
    float range = Val("radarrange") * 100.f;
    int filter = (int)Val("radarfilter");
    // build 17: radar and compass choose separately - collect a category if either of them wants it
    bool rOn = On("radar"), cOn = On("compass") && On("compmark");
    auto want = [&](int cat) { return (rOn && On(RadarCatId(cat, false))) || (cOn && On(RadarCatId(cat, true))); };
    static std::unordered_map<void*, std::pair<double, bool>> hostile;   // attitude: script call, rare
    double now = NowSec();
    ForEachNpc([&](void* n) {
        if (n == h) return;
        zVEC3 p = Pos(n); float dx = p.x - hp.x, dz = p.z - hp.z;
        if (std::fabs(dx) > range || std::fabs(dz) > range) return;
        bool dead = NpcDead(n);
        if (dead && !want(4)) return;
        bool ally = IsAlly(n), mon = IsMonster(n);
        if (dead) { RadarDot d{ dx, dz, g_espColors[4], 0 }; d.cat = 4; d.name = U(NpcName(n)); d.info = "martwy"; s.radar.push_back(std::move(d)); return; }
        auto& hc = hostile[n];
        if (now - hc.first > 1.0) { hc.first = now; hc.second = !dead && !ally && CallExt("Npc_GetAttitude", { Arg::Npc(n), Arg::Npc(h) }) == K.attHostile; }
        bool host = hc.second;
        (void)filter;   // build 15: checkmarks per kind instead of one filter
        int cat = ally ? 3 : host ? 0 : mon ? 2 : 1;
        if (!want(cat)) return;
        RadarDot d{ dx, dz, dead ? g_espColors[4] : ally ? g_espColors[1] : host ? g_espColors[0] : mon ? g_espColors[3] : g_espColors[2], ally ? 3 : 0 };
        d.cat = cat;
        d.name = U(NpcName(n));
        char b[160]; std::string k = Npcs_Kind(n);
        snprintf(b, sizeof b, "%s%s%s  ·  %d/%d HP  ·  %.0f m", GuildName(fld<int>(n, K.offGuild)).c_str(), k.empty() ? "" : "  ·  ", k.c_str(), Attr(n, 0), Attr(n, 1), std::sqrt(dx * dx + dz * dz) / 100.f);
        d.info = b;
        s.radar.push_back(std::move(d));
    });
    if (hostile.size() > 400) hostile.clear();
    void* w = GameWorld();
    if (want(5)) ForEachList(fld<void*>(w, W_ITEMLIST), [&](void* it) {
        zVEC3 p = Pos(it); float dx = p.x - hp.x, dz = p.z - hp.z;
        if (std::fabs(dx) > range || std::fabs(dz) > range) return;
        RadarDot d{ dx, dz, g_espColors[5], 1 }; d.cat = 5;
        d.name = ItemLine(it);
        int v = K.offItemValue >= 0 ? fld<int>(it, K.offItemValue) : 0;
        char b[64]; snprintf(b, sizeof b, "%d rudy  ·  %.0f m", v / 2, std::sqrt(dx * dx + dz * dz) / 100.f); d.info = b;
        s.radar.push_back(std::move(d));
    });
    if (want(6)) {
        if (g_lastChest == 0 || now - g_lastChest > 30.0) {
            g_lastChest = now; g_chests.clear();
            ForEachList(fld<void*>(w, W_VOBLIST), [&](void* v) { if (*reinterpret_cast<uintptr_t*>(v) == VTBL_OCMOBCONTAINER) g_chests.push_back(v); });
        }
        for (void* v : g_chests) {
            zVEC3 p = Pos(v); float dx = p.x - hp.x, dz = p.z - hp.z;
            if (std::fabs(dx) > range || std::fabs(dz) > range) continue;
            RadarDot d{ dx, dz, g_espColors[6], 2 }; d.cat = 6;
            d.name = "Skrzynia";
            char b[40]; snprintf(b, sizeof b, "%.0f m", std::sqrt(dx * dx + dz * dz) / 100.f); d.info = b;
            s.radar.push_back(std::move(d));
        }
    }
    if (s.radar.size() > 300) s.radar.resize(300);
}
void WidgetTick(TickCtx& c) {   // build 16: threats + weapon for the dock
    Snapshot& s = c.s; void* h = c.hero;
    static double at = 0; static int cnt = 0; static std::string nm, wpn; static float nd = 0;
    if (NowSec() - at > 0.5) {
        at = NowSec();
        cnt = 0; nm.clear(); nd = 1e9f;
        if (On("wthreat")) NpcsInRadius(Val("wthreatr") * 100.f, [&](void* n) {
            if (NpcDead(n) || IsAlly(n)) return;
            static std::unordered_map<void*, std::pair<double, bool>> hc;
            auto& e = hc[n];
            if (NowSec() - e.first > 1.0) { e.first = NowSec(); e.second = CallExt("Npc_GetAttitude", { Arg::Npc(n), Arg::Npc(h) }) == K.attHostile; }
            if (!e.second) return;
            cnt++; float d = Dist(Pos(n), Pos(h)) / 100.f;
            if (d < nd) { nd = d; nm = U(NpcName(n)); }
        });
        if (On("wweapon")) {
            static int flagsOff = -1; if (flagsOff < 0) flagsOff = FieldOff("C_ITEM.FLAGS", -1);
            Sym* itemSym = SymByName("ITEM"); std::string melee, ranged;
            for (int cat = 1; cat <= 2; cat++)   // INV_WEAPON, INV_ARMOR... category 1 = weapons in G1 inventory
                for (int slot = 0; slot < 40; slot++) {
                    int amount = CallExt("Npc_GetInvItemBySlot", { Arg::Npc(h), Arg::Int(cat), Arg::Int(slot) });
                    if (amount <= 0) break;
                    void* it = itemSym ? (void*)(uintptr_t)itemSym->offset : nullptr;
                    if (!IsItem(it) || flagsOff < 0 || !(fld<int>(it, flagsOff) & (1 << 30))) continue;
                    int mf = FieldOff("C_ITEM.MAINFLAG", -1) >= 0 ? fld<int>(it, FieldOff("C_ITEM.MAINFLAG", -1)) : 0;
                    std::string n2 = K.offItemName >= 0 ? U(zs(fld<zSTRING>(it, K.offItemName))) : "?";
                    if (mf & ConstInt("ITEM_KAT_FF", 4)) ranged = n2; else if (mf & ConstInt("ITEM_KAT_NF", 2)) melee = n2;
                }
            wpn = melee.empty() ? ranged : ranged.empty() ? melee : melee + " / " + ranged;
            if (wpn.empty()) wpn = "pięści";
        }
    }
    s.thrCount = cnt; s.thrName = nm; s.thrDist = nd; s.weapon = wpn;
    static double at2 = 0; static int ar = 0, bo = 0, nc = 0, ic = 0; static float mem = 0;
    if (NowSec() - at2 > 0.5) {   // build 17: ammo, counts, memory
        at2 = NowSec();
        int sa = SymIndex("ITAMARROW"), sb = SymIndex("ITAMBOLT");
        ar = sa >= 0 ? CallExt("Npc_HasItems", { Arg::Npc(h), Arg::Int(sa) }) : 0; bo = sb >= 0 ? CallExt("Npc_HasItems", { Arg::Npc(h), Arg::Int(sb) }) : 0;
        nc = 0; ForEachNpc([&](void*) { nc++; });
        ic = 0; if (void* w = GameWorld()) ForEachList(fld<void*>(w, W_ITEMLIST), [&](void*) { ic++; });
        PROCESS_MEMORY_COUNTERS pm{}; if (K32GetProcessMemoryInfo(GetCurrentProcess(), &pm, sizeof pm)) mem = pm.WorkingSetSize / 1048576.f;
    }
    s.arrows = ar; s.bolts = bo; s.npcCount = nc; s.itemCount = ic; s.memMB = mem;
}
void Esp_Tick(TickCtx& c) {
    Snapshot& s = c.s;
    { static double gdxAt = 0; if (NowSec() - gdxAt > 1.0) { gdxAt = NowSec(); Guard("gfx:gd3d11", [] { GdxTick(); }); } }
    if (Row* v = T.Get("xhver")) if (v->val < 2) {   // build 17: 2 diagonal->cross+45°, 3 circle->2, 4 corners->3
        Row* st = T.Get("xhstyle"); Row* rot = T.Get("xhrot");
        if (st) { int o = (int)st->val; if (o == 2) { st->val = 0; if (rot) rot->val = std::fmod(rot->val + 45.f, 360.f); } else if (o == 3) st->val = 2; else if (o == 4) st->val = 3; }
        v->val = 2; T.Save();
    }
    {   // heading for compass / arrows
        const float* m = &fld<float>(c.hero, VOB_TRAFO);
        float fl = std::sqrt(m[2] * m[2] + m[10] * m[10]); if (fl > 1e-3f) { s.rfx = m[2] / fl; s.rfz = m[10] / fl; }
        float rl = std::sqrt(m[0] * m[0] + m[8] * m[8]); if (rl > 1e-3f) { s.rrx = m[0] / rl; s.rrz = m[8] / rl; }
    }
    if (On("radar") || (On("compass") && On("compmark"))) RadarTick(c);   // build 17: per-widget categories inside
    WidgetTick(c);
    s.espLines = On("espline");
    bool itm = On("espitm");
    bool npcOn = On("espnpc"), itemOn = itm && On("espitems"), chestOn = itm && On("espchest");
    if (!npcOn) s.espLines = false;
    if (!npcOn && !itemOn && !chestOn) return;
    void* h = c.hero; zVEC3 hp = Pos(h);
    float range = Val("esprange") * 100.f, irange = Val("espitemrange") * 100.f;
    int filter = (int)Val("espfilter");
    int boxMode = (int)Val("espboxmode");
    double now = NowSec();

    std::vector<std::pair<float, Label>> npcL, itemL, chestL;
    if (npcOn) {
        bool refresh = now - g_lastNpc > 0.4;
        if (refresh) {
            g_lastNpc = now;
            std::unordered_map<void*, NpcInfo> next;
            ForEachNpc([&](void* n) {
                if (n == h || Dist(Pos(n), hp) > range * 1.2f) return;
                NpcInfo inf;
                auto old = g_npc.find(n);
                bool have = old != g_npc.end();
                if (have) { inf = old->second; }
                else { inf.guild = GuildName(fld<int>(n, K.offGuild)); inf.monster = IsMonster(n); }
                inf.name = U(NpcName(n));
                inf.ally = IsAlly(n);
                if (!have || now - inf.attAt > 1.0) {   // script call - keep it rare
                    inf.attAt = now;
                    inf.hostile = !NpcDead(n) && CallExt("Npc_GetAttitude", { Arg::Npc(n), Arg::Npc(h) }) == K.attHostile;
                }
                inf.color = NpcDead(n) ? g_espColors[4] : inf.ally ? g_espColors[1] : inf.hostile ? g_espColors[0] : inf.monster ? g_espColors[3] : g_espColors[2];
                if (On("espeq") && Dist(Pos(n), hp) < 1500.f && now - inf.eqAt > 2.0) { inf.eq = NpcEquipment(n); inf.eqAt = now; }
                inf.target = (On("esptarget") && !NpcDead(n) && Dist(Pos(n), hp) < 3000.f) ? NpcTarget(n) : nullptr;   // build 16
                next[n] = std::move(inf);
            });
            g_npc.swap(next);
        }
        ForEachNpc([&](void* n) {
            auto it = g_npc.find(n);
            if (it == g_npc.end()) return;
            const NpcInfo& inf = it->second;
            zVEC3 p = Pos(n);
            float d = Dist(p, hp);
            if (d > range) return;
            bool dead = NpcDead(n);
            if (dead && !On("espdead")) return;
            if (filter == 1 && !inf.hostile) return;
            if (filter == 2 && inf.monster) return;
            if (filter == 3 && !inf.monster) return;
            if (filter == 4 && !inf.ally) return;
            if (On("esparrows") && !dead) {   // build 16: direction indicators
                RadarDot dd{ p.x - hp.x, p.z - hp.z, inf.color, 0 }; dd.name = inf.name; s.ind.push_back(std::move(dd));
            }
            Label L{}; L.type = 0; L.hp = -1;
            const float* b = &fld<float>(n, VOB_BBOX);
            if (On("espedge")) {   // build 17: the box left the screen -> a marker on the edge where the box is
                float ex, ey; bool beh;
                if (ProjectRaw({ p.x, (b[1] + b[4]) * 0.5f, p.z }, ex, ey, beh) && (beh || ex < 0.f || ex > 1.f || ey < 0.f || ey > 1.f)) {
                    RadarDot e{ ex, ey, inf.color, 0 }; e.name = inf.name; s.edges.push_back(std::move(e));
                }
            }
            if (!Project({ p.x, b[4] + 15.f, p.z }, L.x, L.y)) return;
            bool wantSkel = On("espskel") && d < 4000.f;
            bool fit = On("espbox") && (boxMode == 3 || boxMode == 4);
            if (On("espbox") && !fit) Boxes(n, L, boxMode);
            if (fit || wantSkel || s.espLines) {
                std::vector<ModelNode> nodes;
                if (ModelNodes(n, nodes)) {
                    float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f; int nproj = 0;
                    std::vector<float> sx(nodes.size()), sy(nodes.size()); std::vector<char> ok(nodes.size(), 0);
                    for (size_t i = 0; i < nodes.size(); i++) {
                        if (nodes[i].parent == -2) continue;
                        if (!Project(nodes[i].pos, sx[i], sy[i])) continue;
                        ok[i] = 1; nproj++;
                        x0 = std::min(x0, sx[i]); y0 = std::min(y0, sy[i]); x1 = std::max(x1, sx[i]); y1 = std::max(y1, sy[i]);
                    }
                    if (wantSkel)
                        for (size_t i = 0; i < nodes.size(); i++) {
                            int pi = nodes[i].parent;
                            if (pi >= 0 && ok[i] && ok[pi]) { L.bones.push_back(sx[i]); L.bones.push_back(sy[i]); L.bones.push_back(sx[pi]); L.bones.push_back(sy[pi]); }
                        }
                    if (nproj > 3) { L.anchor = true; L.ax = (x0 + x1) * 0.5f; L.ay = y1; }
                    if (fit && boxMode == 3 && nproj > 3) {
                        float px = (x1 - x0) * 0.18f + 0.002f, py = (y1 - y0) * 0.08f + 0.004f;
                        L.box = 1; L.bx0 = x0 - px; L.bx1 = x1 + px; L.by0 = y0 - py * 1.6f; L.by1 = y1 + py * 0.5f;
                    }
                    if (fit && boxMode == 4) {   // 3D box fitted to the bones, in the character's own orientation
                        const float* m = &fld<float>(n, VOB_TRAFO);
                        float mn[3] = { 1e9f, 1e9f, 1e9f }, mx[3] = { -1e9f, -1e9f, -1e9f };
                        int cnt = 0;
                        for (auto& nd : nodes) {
                            if (nd.parent == -2) continue;
                            float dv[3] = { nd.pos.x - m[3], nd.pos.y - m[7], nd.pos.z - m[11] };
                            for (int a2 = 0; a2 < 3; a2++) {
                                float l = m[0 * 4 + a2] * dv[0] + m[1 * 4 + a2] * dv[1] + m[2 * 4 + a2] * dv[2];
                                mn[a2] = std::min(mn[a2], l); mx[a2] = std::max(mx[a2], l);
                            }
                            cnt++;
                        }
                        if (cnt > 3) {
                            const float pad[3] = { 14.f, 12.f, 14.f };
                            for (int a2 = 0; a2 < 3; a2++) { mn[a2] -= pad[a2]; mx[a2] += pad[a2]; }
                            bool all = true; float bx0 = 1e9f, by0 = 1e9f, bx1 = -1e9f, by1 = -1e9f;
                            for (int i = 0; i < 8 && all; i++) {
                                float l[3] = { (i & 1) ? mx[0] : mn[0], (i & 2) ? mx[1] : mn[1], (i & 4) ? mx[2] : mn[2] };
                                zVEC3 wp{ m[3] + m[0] * l[0] + m[1] * l[1] + m[2] * l[2], m[7] + m[4] * l[0] + m[5] * l[1] + m[6] * l[2], m[11] + m[8] * l[0] + m[9] * l[1] + m[10] * l[2] };
                                float qx, qy;
                                if (!Project(wp, qx, qy)) { all = false; break; }
                                L.corners[i * 2] = qx; L.corners[i * 2 + 1] = qy;
                                bx0 = std::min(bx0, qx); by0 = std::min(by0, qy); bx1 = std::max(bx1, qx); by1 = std::max(by1, qy);
                            }
                            if (all) { L.box = 2; L.bx0 = bx0; L.by0 = by0; L.bx1 = bx1; L.by1 = by1; }
                        }
                    }
                }
            }
            if (fit && L.box == 0) Boxes(n, L, 1);   // no skeleton -> silhouette
            if (On("espname")) L.text = inf.name;
            std::string sub;
            auto add = [&](const std::string& t) { if (!sub.empty()) sub += "  ·  "; sub += t; };
            if (On("espguild")) add(inf.guild);
            if (On("esptype")) { std::string k = Npcs_Kind(n); if (!k.empty()) add("[" + k + "]"); }
            if (On("espdist")) { char bb[24]; snprintf(bb, sizeof bb, "%.0fm", d / 100.f); add(bb); }
            if (On("esphpnum")) { char bb[40]; snprintf(bb, sizeof bb, "%d/%d HP", Attr(n, 0), Attr(n, 1)); add(bb); }
            if (inf.target && NpcExists(inf.target)) {   // build 16: who fights whom (party tests)
                add("atakuje: " + (inf.target == h ? std::string("ciebie") : U(NpcName(inf.target))));
                const float* tb = &fld<float>(inf.target, VOB_BBOX); zVEC3 tp = Pos(inf.target);
                if (Project({ tp.x, tb[4], tp.z }, L.tx, L.ty)) L.hasT = true;
            }
            L.sub = sub;
            if (On("esphp") && Attr(n, 1) > 0) L.hp = std::max(0.f, std::min(1.f, (float)Attr(n, 0) / Attr(n, 1)));
            if (On("espeq") && d < 1500.f) L.lines = inf.eq;
            L.color = dead ? g_espColors[4] : inf.color;
            npcL.push_back({ d, std::move(L) });
        });
        Cap(npcL, (int)Val("espmaxnpc"), s);
    }
    void* w = GameWorld();
    if (itemOn) {
        int minVal = (int)Val("espitemval");
        ForEachList(fld<void*>(w, W_ITEMLIST), [&](void* it) {
            zVEC3 p = Pos(it);
            float d = Dist(p, hp);
            if (d > irange) return;
            int v = K.offItemValue >= 0 ? fld<int>(it, K.offItemValue) : 0;
            if (v < minVal) return;
            {   // build 17: weapons in someone's hand and arrows / bolts are hidden unless asked for
                static int flagsOff = FieldOff("C_ITEM.FLAGS", -1), mainOff = FieldOff("C_ITEM.MAINFLAG", -1);
                static int kMun = ConstInt("ITEM_KAT_MUN", 8);
                if (!On("espactive") && flagsOff >= 0 && (fld<int>(it, flagsOff) & (1 << 30))) return;
                if (!On("espammo") && mainOff >= 0 && (fld<int>(it, mainOff) & kMun)) return;
            }
            Label L{}; L.type = 1; L.hp = -1;
            if (!Project(p, L.x, L.y)) return;
            L.text = ItemLine(it);
            char b[64]; snprintf(b, sizeof b, "%d rudy  ·  %.0fm", v / 2, d / 100.f); L.sub = b;   // Gothic 1 traders pay half
            L.color = g_espColors[5];
            itemL.push_back({ d, std::move(L) });
        });
        Cap(itemL, (int)Val("espmaxitem"), s);
    }
    if (chestOn) {
        if (g_lastChest == 0 || now - g_lastChest > 30.0) {   // chests never move: scan the big vob list rarely
            g_lastChest = now; g_chests.clear();
            ForEachList(fld<void*>(w, W_VOBLIST), [&](void* v) { if (*reinterpret_cast<uintptr_t*>(v) == VTBL_OCMOBCONTAINER) g_chests.push_back(v); });
        }
        for (void* v : g_chests) {
            zVEC3 p = Pos(v);
            float d = Dist(p, hp);
            if (d > irange) continue;
            Label L{}; L.type = 2; L.hp = -1;
            p.y += 60;
            if (!Project(p, L.x, L.y)) continue;
            L.text = "Skrzynia"; char b[24]; snprintf(b, sizeof b, "%.0fm", d / 100.f); L.sub = b;
            if (On("espchestinv") && d < 2500.f) {
                auto& cc = g_chestInv[v];
                if (now - cc.first > 1.5) { cc.first = now; cc.second = ChestContents(v); }
                L.lines = cc.second;
            }
            L.color = g_espColors[6];
            chestL.push_back({ d, std::move(L) });
        }
        Cap(chestL, (int)Val("espmaxchest"), s);
    }
}

void Esp_Register() {
    const char* C = "Wizualia";
    const char* P = "ESP postaci";   // build 9 (#15): subcategories + master switch + separate ranges
    auto tog = [&](const char* id, const char* name, const char* sub, bool def, Status st, const char* h) {
        T.Add(MkRow(id, name, "", C, sub, K_TOGGLE)); T.rows.back().on = def; St(st, h);
    };
    CardTitle("Główny wyłącznik");
    tog("espnpc", "ESP postaci", P, false, ST_NEW, "build 9: jeden wyłącznik dla wszystkiego; build 10: osobne wyłączniki postaci i przedmiotów (#15)"); Icon(ICN_USERS);
    CardTitle("Postacie");
    T.Add(MkRow("esprange", "Maksymalny zasięg", "m", C, P, K_VALUE, 50, 5, 300)); St(ST_OK, "działa");
    T.Add(MkRow("espmaxnpc", "Maks. postaci naraz", "najbliższe; mniej = lżej dla słabszych komputerów", C, P, K_VALUE, 40, 1, 200)); St(ST_NEW, "nowe w build 6");
    T.Add(MkRow("espfilter", "Pokazuj", "", C, P, K_VALUE, 0, 0, 4));
    Opts({ { 0, "Wszystkich" }, { 1, "Tylko wrogów" }, { 2, "Tylko ludzi" }, { 3, "Tylko potwory" }, { 4, "Tylko drużynę" } }); St(ST_OK, "działa");
    tog("espdead", "Martwi", P, false, ST_OK, "działa"); Hidden();
    CardTitle("Informacje");
    tog("espname", "Imię", P, true, ST_OK, "działa"); Hidden();
    tog("espdist", "Odległość", P, true, ST_OK, "działa"); Hidden();
    tog("espguild", "Gildia", P, false, ST_OK, "działa"); Hidden();
    tog("esptype", "Typ postaci", P, true, ST_NEW, "nowe w build 13: [Arena] / [Drużyna] / [Zrespiony]"); Hidden();
    tog("esphp", "Pasek HP", P, true, ST_OK, "działa"); Hidden();
    tog("esphpnum", "HP liczbowo", P, false, ST_OK, "działa"); Hidden();
    tog("espeq", "Ekwipunek (do 15 m)", P, false, ST_OK, "build 3: działa; Hidden(); build 4: liczba sztuk");
    CardTitle("Ramka i szkielet");
    tog("espbox", "Ramka", P, false, ST_BUG, "build 3: ramki 2D/3D niedopasowane, sylwetka OK; build 4: ramka dopasowana do kości");
    T.Add(MkRow("espboxmode", "Rodzaj ramki", "", C, P, K_VALUE, 3, 0, 4));
    Opts({ { 3, "2D dopasowana do kości" }, { 4, "3D dopasowana do kości" }, { 1, "Sylwetka" }, { 0, "Bounding box 2D" }, { 2, "Bounding box 3D" } });
    St(ST_NEW, "build 6: nowa ramka 3D z kości");
    tog("espskel", "Szkielet", P, false, ST_NEW, "nowe w build 4 (do 40 m)");
    tog("espline", "Linie od dołu ekranu", P, false, ST_OK, "działa");
    CardTitle("Wskaźniki kierunku");
    tog("esparrows", "Strzałki wokół celownika", P, false, ST_NEW, "nowe w build 16: zamiast linii od dołu");
    T.Add(MkRow("esparrowr", "Promień okręgu", "px", C, P, K_VALUE, 140, 50, 500)); St(ST_NEW, "nowe w build 16");
    tog("espedge", "Znaczniki na krawędziach ekranu", P, false, ST_NEW, "build 16: nie trzymały się ramki; build 17: ramka poza ekranem = znacznik w jej miejscu na krawędzi");
    tog("esptarget", "Kogo atakuje", P, false, ST_OK, "build 16: działa"); Hidden();

    // build 16: Wizualia › Widżety - every on-screen widget lives in one dock (no more random positions)
    const char* RD = "Widżety";
    CardTitle("Dok widżetów");
    T.Add(MkRow("wdock", "Położenie", "", C, RD, K_VALUE, 0, 0, 3)); Opts({ { 0, "Lewy górny róg" }, { 1, "Prawy górny róg" }, { 2, "Lewy dolny róg" }, { 3, "Prawy dolny róg" } }); St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("wwidth", "Szerokość", "px", C, RD, K_VALUE, 230, 160, 420)); St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("walpha", "Krycie", "%", C, RD, K_VALUE, 80, 20, 100)); St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("wmenu", "Też przy otwartym menu", "", C, RD, K_TOGGLE)); T.rows.back().on = true; St(ST_NEW, "nowe w build 16");
    CardTitle("Zdrowie");
    tog("whp", "Pasek zdrowia", RD, true, ST_NEW, "nowe w build 16");
    tog("wmana", "Pasek many", RD, false, ST_NEW, "nowe w build 16");
    CardTitle("Radar");
    tog("radar", "Radar", RD, false, ST_OK, "build 14: działa; build 16: w doku widżetów"); Icon(ICN_CROSSHAIR);
    T.Add(MkRow("radarrange", "Zasięg", "m", C, RD, K_VALUE, 60, 10, 300)); St(ST_OK, "build 14: działa");
    T.Add(MkRow("radarsize", "Rozmiar", "px", C, RD, K_VALUE, 240, 120, 600)); Hidden(); St(ST_OK, "build 16: = szerokość doku");
    T.Add(MkRow("radaralpha", "Krycie tła", "%", C, RD, K_VALUE, 75, 0, 100)); Hidden(); St(ST_OK, "build 16: krycie doku");
    T.Add(MkRow("radarpos", "Położenie", "", C, RD, K_VALUE, 0, 0, 3)); Hidden(); St(ST_OK, "build 16: położenie doku");
    tog("radarrot", "Obracaj z postacią", RD, true, ST_OK, "build 14: działa");
    CardTitle("Zagrożenia");
    tog("wthreat", "Zagrożenia", RD, true, ST_OK, "build 16: działa");
    T.Add(MkRow("wthreatr", "Zasięg", "m", C, RD, K_VALUE, 30, 5, 150)); St(ST_NEW, "nowe w build 16");
    CardTitle("Broń i FPS");
    tog("wweapon", "Broń w ręku / założona", RD, true, ST_NEW, "nowe w build 16");
    tog("wfps", "FPS", RD, true, ST_NEW, "nowe w build 16");
    CardTitle("Kompas");
    tog("compass", "Kompas u góry ekranu", RD, false, ST_NEW, "nowe w build 16"); Icon(ICN_MAP);
    T.Add(MkRow("compw", "Szerokość", "px", C, RD, K_VALUE, 440, 240, 1000)); St(ST_NEW, "nowe w build 16");
    T.Add(MkRow("comprange", "Zakres", "stopnie widoczne na pasku", C, RD, K_VALUE, 120, 60, 360)); St(ST_NEW, "nowe w build 16");
    tog("compmark", "Znaczniki", RD, true, ST_OK, "build 16: działa; build 17: własne „co pokazywać”");
    for (int i = 0; i < 7; i++) { static const char* nm[7] = { "Wrogowie", "Neutralni", "Potwory", "Drużyna", "Martwi", "Przedmioty", "Skrzynie" };
        T.Add(MkRow(RadarCatId(i, true), nm[i], "", C, RD, K_TOGGLE)); T.rows.back().on = i < 4; Hidden(); St(ST_NEW, "nowe w build 17"); }
    CardTitle("Cel, amunicja, doświadczenie");
    tog("wtarget", "Pasek życia celu u góry ekranu", RD, false, ST_NEW, "nowe w build 17: imię, gildia i HP postaci, w którą celujesz");
    tog("wammo", "Amunicja (strzały / bełty)", RD, true, ST_NEW, "nowe w build 17");
    tog("wxp", "Pasek doświadczenia", RD, false, ST_NEW, "nowe w build 17");
    // build 15: "what to show" is one card of checkmarks (Wizualia › Radar)
    tog("raden", "Wrogowie", RD, true, ST_NEW, "nowe w build 15"); Hidden();
    tog("radne", "Neutralni", RD, true, ST_NEW, "nowe w build 15"); Hidden();
    tog("radmo", "Potwory", RD, true, ST_NEW, "nowe w build 15"); Hidden();
    tog("radpa", "Drużyna", RD, true, ST_NEW, "nowe w build 15"); Hidden();
    tog("radardead", "Martwi", RD, false, ST_OK, "build 14: działa"); Hidden();
    tog("radaritems", "Przedmioty", RD, false, ST_OK, "build 14: działa"); Hidden();
    tog("radarchests", "Skrzynie", RD, false, ST_OK, "build 14: działa"); Hidden();
    tog("radarnames", "Imiona najbliższych", RD, false, ST_OK, "build 14: działa"); Hidden();
    T.Add(MkRow("radarfilter", "Które postacie", "", C, RD, K_VALUE, 0, 0, 4)); Hidden();

    {   // build 16: crosshair editor (Wizualia › Celownik)
        const char* X = "Celownik";
        CardTitle("Celownik");
        tog("xhair", "Pokaż celownik", X, false, ST_NEW, "nowe w build 16"); Icon(ICN_CROSSHAIR);
        // build 17: four shapes, each card shows only its own settings (Wizualia › Celownik builds the card)
        T.Add(MkRow("xhstyle", "Kształt", "", C, X, K_VALUE, 0, 0, 3)); Opts({ { 0, "Krzyż" }, { 1, "Kropka" }, { 2, "Okrąg" }, { 3, "Kąty" } }); St(ST_NEW, "build 17: bez krzyża ukośnego (= krzyż obrócony o 45°)");
        T.Add(MkRow("xhver", "", "", C, X, K_VALUE, 0, 0, 9)); Hidden(); St(ST_NEW, "wersja ustawień");
        T.Add(MkRow("xhgap", "Przerwa", "px od środka (okrąg: promień)", C, X, K_VALUE, 4, 0, 40, nullptr, true)); St(ST_NEW, "nowe w build 16");
        T.Add(MkRow("xhlen", "Długość", "px", C, X, K_VALUE, 12, 1, 80, nullptr, true)); St(ST_NEW, "nowe w build 16");
        T.Add(MkRow("xhthick", "Grubość", "px", C, X, K_VALUE, 2, 1, 10, nullptr, true)); St(ST_NEW, "nowe w build 16");
        T.Add(MkRow("xhrot", "Obrót", "stopnie", C, X, K_VALUE, 0, 0, 360)); St(ST_NEW, "nowe w build 16");
        T.Add(MkRow("xhspin", "Stały obrót", "stopnie na sekundę (0 = stoi)", C, X, K_VALUE, 0, 0, 720)); St(ST_NEW, "nowe w build 16");
        T.Add(MkRow("xhalpha", "Krycie", "%", C, X, K_VALUE, 100, 10, 100)); St(ST_NEW, "nowe w build 16");
        tog("xhdot", "Kropka w środku", X, false, ST_OK, "build 16: działa");
        T.Add(MkRow("xhdotsize", "Rozmiar kropki", "px", C, X, K_VALUE, 2, 1, 20, nullptr, true)); St(ST_NEW, "nowe w build 17");
        T.Add(MkRow("xhdotshape", "Kształt kropki", "", C, X, K_VALUE, 0, 0, 2)); Opts({ { 0, "Kółko" }, { 1, "Kwadrat" }, { 2, "Romb" } }); St(ST_NEW, "nowe w build 17");
        tog("xhoutline", "Obrys", X, true, ST_NEW, "nowe w build 16");
        for (const char* id : { "xhr", "xhg", "xhb" }) { T.Add(MkRow(id, id, "", C, X, K_VALUE, 255, 0, 255)); Hidden(); St(ST_NEW, "nowe w build 16"); }
        if (Row* r = T.Get("xhb")) r->val = 206;
        if (Row* r = T.Get("xhg")) r->val = 250;
    }
    {   // build 16: Wizualia › Grafika - colour pass over the game frame (menu and overlay stay untouched)
        const char* G = "Grafika";
        CardTitle("Nadpisywanie grafiki");
        tog("gfx", "Włącz nadpisywanie grafiki", G, false, ST_NEW, "nowe w build 16"); Icon(ICN_EYE);
        T.Add(MkRow("gfxmode", "Filtr", "", C, G, K_VALUE, 0, 0, 4)); Opts({ { 0, "Brak" }, { 1, "Czarno-biały" }, { 2, "Sepia" }, { 3, "Noktowizor" }, { 4, "Negatyw" } }); St(ST_NEW, "nowe w build 16");
        T.Add(MkRow("gfxreset", "Przywróć domyślne", "", C, G, K_ACTION, 0, 0, 0, [](Row&) {
            struct { const char* id; float v; } d[] = { { "gfxbright", 0 }, { "gfxcontrast", 100 }, { "gfxsat", 100 }, { "gfxgamma", 100 }, { "gfxtemp", 0 }, { "gfxsharp", 0 }, { "gfxvign", 0 }, { "gfxmode", 0 } };
            for (auto& e : d) if (Row* r = T.Get(e.id)) r->val = e.v;
            T.Toast("Grafika: wartości domyślne"); }));
        NoVal(); Icon(ICN_REFRESH); St(ST_NEW, "nowe w build 16");
        CardTitle("Jasność i kolor");
        T.Add(MkRow("gfxbright", "Jasność", "%", C, G, K_VALUE, 0, -50, 50)); St(ST_NEW, "nowe w build 16");
        T.Add(MkRow("gfxgamma", "Gamma", "% (więcej = jaśniejsze cienie)", C, G, K_VALUE, 100, 50, 250)); St(ST_NEW, "nowe w build 16");
        T.Add(MkRow("gfxcontrast", "Kontrast", "%", C, G, K_VALUE, 100, 50, 200)); St(ST_NEW, "nowe w build 16");
        T.Add(MkRow("gfxsat", "Nasycenie", "%", C, G, K_VALUE, 100, 0, 250)); St(ST_NEW, "nowe w build 16");
        T.Add(MkRow("gfxtemp", "Temperatura barw", "- zimniej / + cieplej", C, G, K_VALUE, 0, -100, 100)); St(ST_NEW, "nowe w build 16");
        CardTitle("Ostrość i winieta");
        T.Add(MkRow("gfxsharp", "Wyostrzenie", "%", C, G, K_VALUE, 0, 0, 150)); St(ST_NEW, "nowe w build 16");
        T.Add(MkRow("gfxvign", "Winieta", "% przyciemnienia rogów", C, G, K_VALUE, 0, 0, 100)); St(ST_NEW, "nowe w build 16");
        // build 17: GD3D11's own settings
        CardTitle("Silnik GD3D11");
        tog("gdx", "Nadpisuj ustawienia GD3D11", G, false, ST_NEW, "nowe w build 17: wyłączenie przywraca wartości gry"); Icon(ICN_EYE);
        T.Add(MkRow("gdxinit", "", "", C, G, K_VALUE, 0, 0, 1)); Hidden(); St(ST_NEW, "");
        T.Add(MkRow("gdxdraw", "Zasięg rysowania obiektów", "m", C, G, K_VALUE, 300, 20, 2000)); St(ST_NEW, "nowe w build 17");
        T.Add(MkRow("gdxsmall", "Zasięg małych obiektów", "m (trawa, kamienie)", C, G, K_VALUE, 100, 10, 1000)); St(ST_NEW, "nowe w build 17");
        T.Add(MkRow("gdxnpc", "Zasięg postaci", "m", C, G, K_VALUE, 100, 10, 1000)); St(ST_NEW, "nowe w build 17");
        T.Add(MkRow("gdxfx", "Zasięg efektów", "m", C, G, K_VALUE, 100, 10, 1000)); St(ST_NEW, "nowe w build 17");
        T.Add(MkRow("gdxsect", "Zasięg świata", "sekcje terenu wokół ciebie", C, G, K_VALUE, 4, 1, 32)); St(ST_NEW, "nowe w build 17");
        T.Add(MkRow("gdxfog", "Gęstość mgły", "% wartości gry", C, G, K_VALUE, 100, 0, 400)); St(ST_NEW, "nowe w build 17");
        T.Add(MkRow("gdxsun", "Siła słońca", "% wartości gry", C, G, K_VALUE, 100, 0, 300)); St(ST_NEW, "nowe w build 17");
        T.Add(MkRow("gdxshadow", "Siła cieni", "% wartości gry", C, G, K_VALUE, 100, 0, 200)); St(ST_NEW, "nowe w build 17");
        T.Add(MkRow("gdxwind", "Wiatr", "% wartości gry", C, G, K_VALUE, 100, 0, 400)); St(ST_NEW, "nowe w build 17");
        tog("gdxscatter", "Rozpraszanie atmosferyczne", G, true, ST_NEW, "nowe w build 17");
    }
    const char* I = "ESP przedmiotów";
    CardTitle("Główny wyłącznik");
    tog("espitm", "ESP przedmiotów i skrzyń", I, true, ST_NEW, "nowe w build 10: wyłącza przedmioty i skrzynie bez odznaczania opcji (#15)"); Icon(ICN_BAG);
    CardTitle("Przedmioty na ziemi");
    tog("espitems", "Przedmioty na ziemi", I, false, ST_OK, "działa"); Icon(ICN_BAG); Hidden();
    T.Add(MkRow("espitemrange", "Maksymalny zasięg", "m (też skrzynie)", C, I, K_VALUE, 50, 5, 300)); St(ST_NEW, "nowe w build 9 - osobny zasięg dla przedmiotów");
    T.Add(MkRow("espitemval", "Min. wartość", "rudy", C, I, K_VALUE, 0, 0, 5000)); St(ST_OK, "działa");
    T.Add(MkRow("espmaxitem", "Maks. przedmiotów naraz", "najbliższe", C, I, K_VALUE, 40, 1, 200)); St(ST_NEW, "nowe w build 6");
    CardTitle("Skrzynie");
    tog("espchest", "Skrzynie", I, false, ST_OK, "działa"); Icon(ICN_CUBE); Hidden();
    tog("espchestinv", "Zawartość skrzyń", I, false, ST_OK, "build 4: liczba sztuk z obiektu przedmiotu"); Hidden();
    tog("espactive", "Broń w rękach postaci", I, false, ST_NEW, "nowe w build 17: domyślnie ukryta"); Hidden();
    tog("espammo", "Strzały i bełty", I, false, ST_NEW, "nowe w build 17: domyślnie ukryte"); Hidden();
    T.Add(MkRow("espmaxchest", "Maks. skrzyń naraz", "najbliższe", C, I, K_VALUE, 20, 1, 100)); St(ST_NEW, "nowe w build 6");
}
