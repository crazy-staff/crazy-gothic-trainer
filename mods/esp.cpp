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
std::string ItemLine(void* it) {
    std::string n = K.offItemName >= 0 ? U(zs(fld<zSTRING>(it, K.offItemName))) : "?";
    int amt = K.offItemAmount >= 0 ? fld<int>(it, K.offItemAmount) : 1;
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
            all.push_back({ (worn ? 1000000 : 0) + val, (worn ? "[założone] " : "") + ItemLine(it) });
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
            if (On("espbox")) Boxes(n, L, boxMode);
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
    const char* C = "Render";
    auto tog = [&](const char* id, const char* name, const char* sub, bool def, Status st, const char* h) {
        T.Add(MkRow(id, name, "", C, sub, K_TOGGLE)); T.rows.back().on = def; St(st, h); Hidden();
    };
    tog("espnpc", "NPC i potwory", "ESP", false, ST_OK, "działa");
    tog("espitems", "Przedmioty na ziemi", "ESP", false, ST_OK, "działa");
    tog("espchest", "Skrzynie", "ESP", false, ST_OK, "działa");
    tog("espchestinv", "Zawartość skrzyń", "ESP", false, ST_NEW, "nowe w build 3");
    tog("espeq", "Ekwipunek postaci (do 15 m)", "ESP", false, ST_NEW, "nowe w build 3");
    T.Add(MkRow("esprange", "Maksymalny zasięg", "m", C, "ESP", K_VALUE, 50, 5, 300)); Hidden(); St(ST_OK, "działa");
    tog("espname", "Imię", "Wygląd", true, ST_OK, "działa");
    tog("espdist", "Odległość", "Wygląd", true, ST_OK, "działa");
    tog("espguild", "Gildia", "Wygląd", false, ST_OK, "działa");
    tog("esphp", "Pasek HP", "Wygląd", true, ST_OK, "działa");
    tog("esphpnum", "HP liczbowo", "Wygląd", false, ST_OK, "działa");
    tog("espbox", "Ramka", "Wygląd", false, ST_BUG, "build 2: ramka nie pasowała do postaci; build 3: 3 rodzaje ramek");
    T.Add(MkRow("espboxmode", "Rodzaj ramki", "", C, "Wygląd", K_VALUE, 1, 0, 2)); Hidden();
    Opts({ { 0, "Bounding box 2D" }, { 1, "Sylwetka" }, { 2, "Bounding box 3D" } }); St(ST_NEW, "nowe w build 3");
    tog("espline", "Linie od dołu ekranu", "Wygląd", false, ST_OK, "działa");
    T.Add(MkRow("espfilter", "Pokazuj", "", C, "Filtry", K_VALUE, 0, 0, 4)); Hidden();
    Opts({ { 0, "Wszystkich" }, { 1, "Tylko wrogów" }, { 2, "Tylko ludzi" }, { 3, "Tylko potwory" }, { 4, "Tylko sojuszników" } }); St(ST_OK, "działa");
    tog("espdead", "Martwi", "Filtry", false, ST_OK, "działa");
    T.Add(MkRow("espitemval", "Min. wartość przedmiotu", "rudy", C, "Filtry", K_VALUE, 0, 0, 5000)); Hidden(); St(ST_OK, "działa");
}
