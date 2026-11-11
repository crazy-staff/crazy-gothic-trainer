// ESP module: labels/boxes for NPCs, items and chests. Expensive engine queries
// (attitude, names, equipment) are cached and refreshed a few times per second;
// only projection runs every frame.
#include "mods.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <unordered_map>

using namespace g1;
using namespace gx;

unsigned g_espColors[7] = { 0xFF6E6EFF, 0xFF8CF28C, 0xFF7AD8FF, 0xFF55AAFF, 0xFF9A9AA8, 0xFF7AE07A, 0xFF40C8FF };
// 0 enemy 1 ally 2 neutral 3 monster 4 dead 5 item 6 chest

namespace {
struct NpcInfo { std::string name, guild; unsigned color; bool hostile, monster, ally; std::vector<std::string> eq; double eqAt = 0; };
std::unordered_map<void*, NpcInfo> g_npc;
std::vector<void*> g_chests;
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

void Esp_Hooks() {}
void Esp_World() { g_npc.clear(); g_chests.clear(); g_lastNpc = g_lastChest = 0; }

void Esp_Tick(TickCtx& c) {
    Snapshot& s = c.s;
    s.espLines = On("espline");
    bool npcOn = On("espnpc"), itemOn = On("espitems"), chestOn = On("espchest");
    if (!npcOn && !itemOn && !chestOn) return;
    void* h = c.hero; zVEC3 hp = Pos(h);
    float range = Val("esprange") * 100.f;
    int filter = (int)Val("espfilter");
    int boxMode = (int)Val("espboxmode");
    double now = NowSec();

    if (npcOn) {
        bool refresh = now - g_lastNpc > 0.25;
        if (refresh) {
            g_lastNpc = now;
            std::unordered_map<void*, NpcInfo> next;
            ForEachNpc([&](void* n) {
                if (n == h || Dist(Pos(n), hp) > range * 1.2f) return;
                NpcInfo inf;
                auto old = g_npc.find(n);
                if (old != g_npc.end()) { inf.eq = old->second.eq; inf.eqAt = old->second.eqAt; }
                inf.name = U(NpcName(n));
                inf.guild = GuildName(fld<int>(n, K.offGuild));
                inf.monster = IsMonster(n); inf.ally = IsAlly(n);
                inf.hostile = !NpcDead(n) && CallExt("Npc_GetAttitude", { Arg::Npc(n), Arg::Npc(h) }) == K.attHostile;
                inf.color = NpcDead(n) ? g_espColors[4] : inf.ally ? g_espColors[1] : inf.hostile ? g_espColors[0] : inf.monster ? g_espColors[3] : g_espColors[2];
                if (On("espeq") && Dist(Pos(n), hp) < 1500.f && now - inf.eqAt > 2.0) { inf.eq = NpcEquipment(n); inf.eqAt = now; }
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
            Label L{}; L.type = 0; L.hp = -1;
            const float* b = &fld<float>(n, VOB_BBOX);
            if (!Project({ p.x, b[4] + 15.f, p.z }, L.x, L.y)) return;
            bool wantSkel = On("espskel") && d < 4000.f;
            if (On("espbox") && boxMode != 3) Boxes(n, L, boxMode);
            if ((On("espbox") && boxMode == 3) || wantSkel) {
                std::vector<zVEC3> pts; std::vector<int> par;
                if (Skeleton(n, pts, par)) {
                    float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f; int nproj = 0;
                    std::vector<float> sx(pts.size()), sy(pts.size()); std::vector<char> ok(pts.size(), 0);
                    for (size_t i = 0; i < pts.size(); i++) {
                        if (par[i] == -2) continue;
                        if (!Project(pts[i], sx[i], sy[i])) continue;
                        ok[i] = 1; nproj++;
                        x0 = std::min(x0, sx[i]); y0 = std::min(y0, sy[i]); x1 = std::max(x1, sx[i]); y1 = std::max(y1, sy[i]);
                    }
                    if (wantSkel)
                        for (size_t i = 0; i < pts.size(); i++)
                            if (par[i] >= 0 && ok[i] && ok[par[i]]) { L.bones.push_back(sx[i]); L.bones.push_back(sy[i]); L.bones.push_back(sx[par[i]]); L.bones.push_back(sy[par[i]]); }
                    if (On("espbox") && boxMode == 3 && nproj > 3) {
                        float px = (x1 - x0) * 0.18f + 0.002f, py = (y1 - y0) * 0.08f + 0.004f;
                        L.box = 1; L.bx0 = x0 - px; L.bx1 = x1 + px; L.by0 = y0 - py * 1.6f; L.by1 = y1 + py * 0.5f;
                    }
                }
            }
            if (On("espbox") && boxMode == 3 && L.box == 0) Boxes(n, L, 1);   // no skeleton -> silhouette
            if (On("espname")) L.text = inf.name;
            std::string sub;
            auto add = [&](const std::string& t) { if (!sub.empty()) sub += "  ·  "; sub += t; };
            if (On("espguild")) add(inf.guild);
            if (On("espdist")) { char bb[24]; snprintf(bb, sizeof bb, "%.0fm", d / 100.f); add(bb); }
            if (On("esphpnum")) { char bb[40]; snprintf(bb, sizeof bb, "%d/%d HP", Attr(n, 0), Attr(n, 1)); add(bb); }
            L.sub = sub;
            if (On("esphp") && Attr(n, 1) > 0) L.hp = std::max(0.f, std::min(1.f, (float)Attr(n, 0) / Attr(n, 1)));
            if (On("espeq") && d < 1500.f) L.lines = inf.eq;
            L.color = dead ? g_espColors[4] : inf.color;
            s.labels.push_back(std::move(L));
        });
    }
    void* w = GameWorld();
    if (itemOn) {
        int minVal = (int)Val("espitemval");
        ForEachList(fld<void*>(w, W_ITEMLIST), [&](void* it) {
            zVEC3 p = Pos(it);
            float d = Dist(p, hp);
            if (d > range) return;
            int v = K.offItemValue >= 0 ? fld<int>(it, K.offItemValue) : 0;
            if (v < minVal) return;
            Label L{}; L.type = 1; L.hp = -1;
            if (!Project(p, L.x, L.y)) return;
            L.text = ItemLine(it);
            char b[48]; snprintf(b, sizeof b, "%d rudy  ·  %.0fm", v, d / 100.f); L.sub = b;
            L.color = g_espColors[5];
            s.labels.push_back(std::move(L));
        });
    }
    if (chestOn) {
        if (now - g_lastChest > 3.0 || g_chests.empty()) {
            g_lastChest = now; g_chests.clear();
            ForEachList(fld<void*>(w, W_VOBLIST), [&](void* v) { if (*reinterpret_cast<uintptr_t*>(v) == VTBL_OCMOBCONTAINER) g_chests.push_back(v); });
        }
        for (void* v : g_chests) {
            zVEC3 p = Pos(v);
            float d = Dist(p, hp);
            if (d > range) continue;
            Label L{}; L.type = 2; L.hp = -1;
            p.y += 60;
            if (!Project(p, L.x, L.y)) continue;
            L.text = "Skrzynia"; char b[24]; snprintf(b, sizeof b, "%.0fm", d / 100.f); L.sub = b;
            if (On("espchestinv") && d < 2500.f) L.lines = ChestContents(v);
            L.color = g_espColors[6];
            s.labels.push_back(std::move(L));
        }
    }
}

void Esp_Register() {
    const char* C = "Wizualia";
    const char* P = "ESP postaci";
    auto tog = [&](const char* id, const char* name, const char* sub, bool def, Status st, const char* h) {
        T.Add(MkRow(id, name, "", C, sub, K_TOGGLE)); T.rows.back().on = def; St(st, h);
    };
    CardTitle("Postacie");
    tog("espnpc", "Pokaż postacie", P, false, ST_OK, "działa"); Icon(ICN_EYE);
    T.Add(MkRow("esprange", "Maksymalny zasięg", "m", C, P, K_VALUE, 50, 5, 300)); St(ST_OK, "działa");
    T.Add(MkRow("espfilter", "Pokazuj", "", C, P, K_VALUE, 0, 0, 4));
    Opts({ { 0, "Wszystkich" }, { 1, "Tylko wrogów" }, { 2, "Tylko ludzi" }, { 3, "Tylko potwory" }, { 4, "Tylko drużynę" } }); St(ST_OK, "działa");
    tog("espdead", "Martwi", P, false, ST_OK, "działa");
    CardTitle("Informacje");
    tog("espname", "Imię", P, true, ST_OK, "działa");
    tog("espdist", "Odległość", P, true, ST_OK, "działa");
    tog("espguild", "Gildia", P, false, ST_OK, "działa");
    tog("esphp", "Pasek HP", P, true, ST_OK, "działa");
    tog("esphpnum", "HP liczbowo", P, false, ST_OK, "działa");
    tog("espeq", "Ekwipunek (do 15 m)", P, false, ST_OK, "build 3: działa; build 4: liczba sztuk");
    CardTitle("Ramka i szkielet");
    tog("espbox", "Ramka", P, false, ST_BUG, "build 3: ramki 2D/3D niedopasowane, sylwetka OK; build 4: ramka dopasowana do kości");
    T.Add(MkRow("espboxmode", "Rodzaj ramki", "", C, P, K_VALUE, 3, 0, 3));
    Opts({ { 3, "Dopasowana (z kości)" }, { 1, "Sylwetka" }, { 0, "Bounding box 2D" }, { 2, "Bounding box 3D" } }); St(ST_NEW, "build 4: nowa 'dopasowana'");
    tog("espskel", "Szkielet", P, false, ST_NEW, "nowe w build 4 (do 40 m)");
    tog("espline", "Linie od dołu ekranu", P, false, ST_OK, "działa");

    const char* I = "ESP przedmiotów";
    CardTitle("Przedmioty na ziemi");
    tog("espitems", "Pokaż przedmioty", I, false, ST_OK, "działa"); Icon(ICN_BAG);
    T.Add(MkRow("espitemval", "Min. wartość", "rudy", C, I, K_VALUE, 0, 0, 5000)); St(ST_OK, "działa");
    CardTitle("Skrzynie");
    tog("espchest", "Pokaż skrzynie", I, false, ST_OK, "działa"); Icon(ICN_CUBE);
    tog("espchestinv", "Zawartość skrzyń", I, false, ST_BUG, "build 3: brak liczby sztuk; build 4: liczba sztuk z obiektu przedmiotu");
}
