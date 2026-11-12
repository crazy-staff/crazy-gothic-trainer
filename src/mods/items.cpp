// Items module: spawning single items, preset kits and user-made item sets.
#include "mods.h"
#include <algorithm>
#include "../tester.h"
#include <unordered_set>
#include <unordered_map>

using namespace g1;
using namespace gx;

namespace {
void GiveWhere(std::function<bool(const ItemEntry&)> pred, int amount, const char* msg) {
    int c = 0;
    for (auto& e : g_cat.items) if (pred(e) && !T.Crashed("item:" + e.id)) { CreateItems(Player(), e.sym, amount); c++; }
    T.Toast(std::string(msg) + " (" + std::to_string(c) + ")");
}
const ItemEntry* Best(int cat, std::function<int(const ItemEntry&)> score, std::function<bool(const ItemEntry&)> pred = nullptr) {
    const ItemEntry* b = nullptr;
    for (auto& e : g_cat.items)
        if (e.cat == cat && (!pred || pred(e)) && (!b || score(e) > score(*b))) b = &e;
    return b;
}
void GiveInst(const char* id, int n) { int s = SymIndex(id); if (s >= 0) CreateItems(Player(), s, n); }
}

void Act_GiveItem(int sym, int amount) {
    Sym* s = SymAt(sym); std::string id = s ? zs(s->name) : "?";
    T.Queue("item:" + id, [=] { if (!InGame()) return; CreateItems(Player(), sym, amount); T.Toast("+" + std::to_string(amount) + " " + id); });
}
void Act_DropItem(int sym, int amount) {
    Sym* s = SymAt(sym); std::string id = s ? zs(s->name) : "?";
    T.Queue("item:" + id, [=] {
        if (!InGame()) return;
        zVEC3 base = Grounded(InFront(220.f));
        for (int i = 0; i < std::min(amount, 50); i++)
            SpawnItem(sym, { base.x + (float)(i % 5) * 25.f, base.y + 10.f, base.z + (float)(i / 5) * 25.f });
        T.Toast("Upuszczono przed tobą: " + id);
    });
}
void Act_GiveItemTarget(int sym, int amount) {
    Sym* s = SymAt(sym); std::string id = s ? zs(s->name) : "?";
    T.Queue("item:" + id, [=] {
        void* t = Target();
        if (!t) { T.Toast("Brak celu - spójrz na NPC albo zapisz cel"); return; }
        CreateItems(t, sym, amount);
        T.Toast("Dano celowi: " + U(NpcName(t)));
    });
}
void Act_GiveSet(const ItemSet& set) {
    ItemSet copy = set;
    T.Queue("set:" + set.name, [copy] {
        if (!InGame()) return;
        int c = 0;
        for (auto& it : copy.items) { int s = SymIndex(it.first); if (s >= 0) { CreateItems(Player(), s, it.second); c++; } }
        T.Toast("Zestaw \"" + copy.name + "\": " + std::to_string(c) + " pozycji");
    });
}

// ---- build 15: auto pick-up, items on the map, pick-up bot ----------------------------------------
namespace {
const char* kLootIds[IC_COUNT] = { "lootc0", "lootc1", "lootc2", "lootc3", "lootc4", "lootc5", "lootc6", "lootc7", "lootc8", "lootc9", "lootc10", "lootc11" };
int ItemCatOf(int sym) {
    static std::unordered_map<int, int> m; static size_t built = 0;
    if (built != g_cat.items.size()) { m.clear(); for (auto& e : g_cat.items) m[e.sym] = e.cat; built = g_cat.items.size(); }
    auto it = m.find(sym); return it == m.end() ? IC_MISC : it->second;
}
int ItemValue(void* it) { return K.offItemValue >= 0 ? fld<int>(it, K.offItemValue) : 0; }
bool LootWanted(void* it) {
    int sym = NpcInstance(it);
    if (sym == K.nugget) return On("lootore");
    if (!On(kLootIds[ItemCatOf(sym)])) return false;
    return ItemValue(it) >= (int)Val("lootval");
}
std::unordered_map<void*, double> g_animTaken;   // queued AI_TakeItem -> instant fallback after 3 s
void PickNow(void* hero, void* it) {
    int sym = NpcInstance(it), amt = std::max(1, fld<int>(it, ITEM_AMOUNT));
    if (sym < 0) return;
    CreateItems(hero, sym, amt);
    RemoveVob(it);
}
std::unordered_set<void*> WorldItems() {
    std::unordered_set<void*> s; void* w = GameWorld();
    if (w) ForEachList(fld<void*>(w, W_ITEMLIST), [&](void* it) { s.insert(it); });
    return s;
}
std::vector<void*> g_route; size_t g_ri = 0; bool g_bot = false; double g_botAt = 0; zVEC3 g_botHome{}; int g_botGot = 0;
std::shared_ptr<const std::vector<MapItem>> g_map; double g_mapAt = -10;
}
void Act_PickItem(uintptr_t p) {
    T.Queue("item:pick", [p] { void* it = (void*)p; if (WorldItems().count(it) && Player()) { PickNow(Player(), it); T.Toast("Podniesiono"); } });
}
void Act_BotStart(const std::vector<uintptr_t>& items) {
    T.Queue("item:bot", [items] {
        void* h = Player(); if (!h) return;
        // nearest neighbour order from where you stand
        std::vector<void*> left; for (auto p : items) left.push_back((void*)p);
        g_route.clear(); zVEC3 at = Pos(h);
        while (!left.empty()) {
            size_t bi = 0; float bd = 1e30f;
            for (size_t i = 0; i < left.size(); i++) { float d = Dist(Pos(left[i]), at); if (d < bd) { bd = d; bi = i; } }
            g_route.push_back(left[bi]); at = Pos(left[bi]); left.erase(left.begin() + bi);
        }
        g_ri = 0; g_bot = !g_route.empty(); g_botHome = Pos(h); g_botGot = 0; g_botAt = 0;
        T.Toast("Bot: " + std::to_string(g_route.size()) + " przedmiotów do zebrania (Esc przerywa)");
        tst::Event("lootbot", "start: " + std::to_string(g_route.size()));
    });
}
void Act_BotStop() { T.Queue("item:botstop", [] { if (g_bot) T.Toast("Bot zatrzymany: zebrano " + std::to_string(g_botGot)); g_bot = false; }); }

void Items_Hooks() {}
void Items_World() { g_bot = false; g_route.clear(); g_animTaken.clear(); g_map.reset(); g_mapAt = -10; }
void Items_Tick(TickCtx& c) {
    void* h = c.hero;
    double now = NowSec();
    // auto pick-up
    static double lootAt = 0;
    if (On("autoloot") && !c.paused && h && !NpcDead(h) && now - lootAt > std::max(0.1f, Val("lootint"))) {
        lootAt = now;
        bool anim = On("lootanim");
        float r = Val("lootrange") * 100.f; if (anim) r = std::min(r, 350.f);
        zVEC3 hp = Pos(h);
        std::vector<std::pair<float, void*>> v;
        ForEachList(fld<void*>(GameWorld(), W_ITEMLIST), [&](void* it) { float d = Dist(Pos(it), hp); if (d <= r && LootWanted(it)) v.push_back({ d, it }); });
        std::sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.first < b.first; });
        int maxN = anim ? 1 : std::max(1, (int)Val("lootmax")), n = 0;
        for (auto& e : v) {
            if (n >= maxN) break;
            if (anim) {
                auto it = g_animTaken.find(e.second);
                if (it == g_animTaken.end()) { g_animTaken[e.second] = now; CallExt("AI_TakeItem", { Arg::Npc(h), Arg::Npc(e.second) }); n++; }
                else if (now - it->second > 3.0) { PickNow(h, e.second); g_animTaken.erase(it); n++; }   // animation did not take it
            } else { PickNow(h, e.second); n++; }
        }
        if (g_animTaken.size() > 200) g_animTaken.clear();
    }
    // bot
    if (g_bot && h) {
        if (GameFocused() && (GetAsyncKeyState(VK_ESCAPE) & 0x8000)) { g_bot = false; T.Toast("Bot przerwany (Esc): zebrano " + std::to_string(g_botGot)); }
        else if (!c.paused && now - g_botAt > std::max(0.05f, Val("lootbotdelay"))) {
            g_botAt = now;
            auto live = WorldItems();
            while (g_ri < g_route.size() && !live.count(g_route[g_ri])) g_ri++;
            if (g_ri >= g_route.size()) {
                g_bot = false;
                if (On("lootbotback")) SafeSetPos(h, g_botHome);
                T.Toast("Bot skończył: zebrano " + std::to_string(g_botGot));
                tst::Event("lootbot", "koniec: " + std::to_string(g_botGot));
            } else {
                void* it = g_route[g_ri++];
                zVEC3 p = Pos(it);
                SafeSetPos(h, { p.x, p.y + 100.f, p.z });
                PickNow(h, it); g_botGot++;
            }
        }
    }
    c.s.botOn = g_bot; c.s.botGot = g_botGot; c.s.botLeft = g_bot ? (int)(g_route.size() - g_ri) : 0;
    // map list (only while its page is open)
    if (T.menuOpen && T.mapWanted && now - g_mapAt > 1.5 && h) {
        g_mapAt = now;
        auto v = std::make_shared<std::vector<MapItem>>();
        zVEC3 hp = Pos(h);
        ForEachList(fld<void*>(GameWorld(), W_ITEMLIST), [&](void* it) {
            MapItem m; m.ptr = (uintptr_t)it; m.sym = NpcInstance(it);
            m.cat = m.sym == K.nugget ? IC_COUNT : ItemCatOf(m.sym);
            m.value = ItemValue(it); m.amount = std::max(1, fld<int>(it, ITEM_AMOUNT));
            m.name = K.offItemName >= 0 ? U(zs(fld<zSTRING>(it, K.offItemName))) : "?";
            m.id = InstName(it);
            zVEC3 p = Pos(it); m.x = p.x; m.y = p.y; m.z = p.z; m.dist = Dist(p, hp) / 100.f;
            v->push_back(std::move(m));
        });
        std::sort(v->begin(), v->end(), [](const MapItem& a, const MapItem& b) { return a.dist < b.dist; });
        g_map = v;
    }
    T.mapWanted = false;
    if (T.menuOpen) c.s.mapItems = g_map;
}

void Items_Register() {
    const char* C = "Ekwipunek";
    {   // build 15: Ekwipunek › Zbieranie
        const char* Z = "Zbieranie";
        CardTitle("Automatyczne podnoszenie");
        T.Add(MkRow("autoloot", "Podnoś automatycznie", "przedmioty z ziemi wokół ciebie trafiają do ekwipunku", C, Z, K_TOGGLE)); St(ST_NEW, "nowe w build 15"); Icon(ICN_MAGNET);
        T.Add(MkRow("lootrange", "Zasięg", "m (z animacją najwyżej 3,5 m)", C, Z, K_VALUE, 5, 1, 50)); St(ST_NEW, "nowe w build 15");
        T.Add(MkRow("lootmax", "Ile naraz", "przedmiotów w jednym przebiegu", C, Z, K_VALUE, 5, 1, 50)); St(ST_NEW, "nowe w build 15");
        T.Add(MkRow("lootint", "Co ile", "sekund", C, Z, K_VALUE, 0.5f, 0.1f, 5.f, nullptr, true)); St(ST_NEW, "nowe w build 15");
        T.Add(MkRow("lootval", "Min. wartość", "rudy (0 = wszystko)", C, Z, K_VALUE, 0, 0, 5000)); St(ST_NEW, "nowe w build 15");
        T.Add(MkRow("lootanim", "Z animacją podnoszenia", "bohater podnosi po jednym przedmiocie jak w grze (wolniej)", C, Z, K_TOGGLE)); St(ST_NEW, "nowe w build 15");
        for (int i = 0; i < IC_COUNT; i++) { T.Add(MkRow(kLootIds[i], kItemCatNames[i], "", C, Z, K_TOGGLE)); T.rows.back().on = true; Hidden(); St(ST_NEW, "nowe w build 15"); }
        T.Add(MkRow("lootore", "Ruda", "", C, Z, K_TOGGLE)); T.rows.back().on = true; Hidden(); St(ST_NEW, "nowe w build 15");
        CardTitle("Bot zbierający");
        T.Add(MkRow("lootbotdelay", "Odstęp między przedmiotami", "sekund", C, Z, K_VALUE, 0.4f, 0.05f, 3.f, nullptr, true)); St(ST_NEW, "nowe w build 15");
        T.Add(MkRow("lootbotback", "Wróć na miejsce po zebraniu", "", C, Z, K_TOGGLE)); T.rows.back().on = true; St(ST_NEW, "nowe w build 15");
    }
    const char* S = "Zestawy";
    CardTitle("Gotowe zestawy");
    T.Add(MkRow("kitwar", "Wojownik", "najmocniejsza broń 1H i 2H, najlepsza zbroja, 10 mikstur leczenia", C, S, K_ACTION, 0, 0, 0, [](Row&) {
        auto dmg = [](const ItemEntry& e) { return e.damage * 1000 + e.value; };
        if (auto b = Best(IC_MELEE, dmg, [](const ItemEntry& e) { return e.sub.find("1H") != std::string::npos; })) CreateItems(Player(), b->sym, 1);
        if (auto b = Best(IC_MELEE, dmg, [](const ItemEntry& e) { return e.sub.find("2H") != std::string::npos; })) CreateItems(Player(), b->sym, 1);
        if (auto b = Best(IC_ARMOR, [](const ItemEntry& e) { return e.prot[2] * 1000 + e.value; })) CreateItems(Player(), b->sym, 1);
        if (auto b = Best(IC_POTION, [](const ItemEntry& e) { return e.value; }, [](const ItemEntry& e) { return e.sub == "Lecznicze"; })) CreateItems(Player(), b->sym, 10);
        T.Toast("Zestaw wojownika"); }));
    NoVal(); Icon(ICN_SHIELD); St(ST_NEW, "build 2: 'Najlepszy sprzęt' - działało, ale słaby dobór; build 3: dobór wg obrażeń/ochrony");
    T.Add(MkRow("kitarcher", "Łucznik", "najlepszy łuk i kusza + 300 strzał i bełtów", C, S, K_ACTION, 0, 0, 0, [](Row&) {
        auto dmg = [](const ItemEntry& e) { return e.damage * 1000 + e.value; };
        if (auto b = Best(IC_RANGED, dmg, [](const ItemEntry& e) { return e.sub == "Łuki"; })) CreateItems(Player(), b->sym, 1);
        if (auto b = Best(IC_RANGED, dmg, [](const ItemEntry& e) { return e.sub == "Kusze"; })) CreateItems(Player(), b->sym, 1);
        GiveInst("ITAMARROW", 300); GiveInst("ITAMBOLT", 300);
        T.Toast("Zestaw łucznika"); }));
    NoVal(); Icon(ICN_CROSSHAIR); St(ST_NEW, "nowe w build 3");
    T.Add(MkRow("kitmage", "Mag", "wszystkie runy, 20 mikstur many, najlepsza szata maga", C, S, K_ACTION, 0, 0, 0, [](Row&) {
        for (auto& e : g_cat.items) if (e.cat == IC_RUNE) CreateItems(Player(), e.sym, 1);
        if (auto b = Best(IC_POTION, [](const ItemEntry& e) { return e.value; }, [](const ItemEntry& e) { return e.sub == "Mana"; })) CreateItems(Player(), b->sym, 20);
        if (auto b = Best(IC_ARMOR, [](const ItemEntry& e) { return e.prot[5] * 1000 + e.value; }, [](const ItemEntry& e) { return e.sub == "Magowie"; })) CreateItems(Player(), b->sym, 1);
        T.Toast("Zestaw maga"); }));
    NoVal(); Icon(ICN_MAGIC); St(ST_NEW, "nowe w build 3");
    T.Add(MkRow("kitthief", "Złodziej", "wszystkie klucze + 30 wytrychów", C, S, K_ACTION, 0, 0, 0, [](Row&) {
        for (auto& e : g_cat.items) if (e.cat == IC_KEY) CreateItems(Player(), e.sym, 1);
        for (auto& e : g_cat.items) if (e.id.find("LOCKPICK") != std::string::npos || e.id.find("DIETRICH") != std::string::npos) CreateItems(Player(), e.sym, 30);
        T.Toast("Zestaw złodzieja"); }));
    NoVal(); Icon(ICN_BAG); St(ST_NEW, "nowe w build 3");
    CardTitle("Wszystko z kategorii");
    T.Add(MkRow("allrunes", "Wszystkie runy", "", C, S, K_ACTION, 0, 0, 0, [](Row&) { GiveWhere([](const ItemEntry& e) { return e.cat == IC_RUNE; }, 1, "Runy dodane"); }));
    NoVal(); St(ST_OK, "działa");
    T.Add(MkRow("allscrolls", "Wszystkie zwoje", "ilość każdego", C, S, K_ACTION, 5, 1, 100, [](Row& r) { GiveWhere([](const ItemEntry& e) { return e.cat == IC_SCROLL; }, (int)r.val, "Zwoje dodane"); }));
    St(ST_OK, "działa");
    T.Add(MkRow("allpotions", "Wszystkie mikstury", "ilość każdej", C, S, K_ACTION, 10, 1, 100, [](Row& r) { GiveWhere([](const ItemEntry& e) { return e.cat == IC_POTION; }, (int)r.val, "Mikstury dodane"); }));
    St(ST_OK, "działa");
    T.Add(MkRow("allkeys", "Wszystkie klucze", "", C, S, K_ACTION, 0, 0, 0, [](Row&) { GiveWhere([](const ItemEntry& e) { return e.cat == IC_KEY; }, 1, "Klucze dodane"); }));
    NoVal(); St(ST_OK, "działa");
    T.Add(MkRow("allmagic", "Wszystkie amulety i pierścienie", "", C, S, K_ACTION, 0, 0, 0, [](Row&) { GiveWhere([](const ItemEntry& e) { return e.cat == IC_MAGIC; }, 1, "Biżuteria dodana"); }));
    NoVal(); St(ST_OK, "działa");
}
