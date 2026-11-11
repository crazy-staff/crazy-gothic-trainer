// Items module: spawning single items, preset kits and user-made item sets.
#include "mods.h"
#include <algorithm>

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

void Items_Hooks() {}
void Items_World() {}
void Items_Tick(TickCtx&) {}

void Items_Register() {
    const char* C = "Ekwipunek";
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
