// Target module: saved targets, mind control, body swap, target manipulation.
#include "mods.h"
#include <algorithm>
#include <cmath>

using namespace g1;
using namespace gx;

namespace {
void ReplaceTargetRef(void* oldN, void* newN) {
    for (auto& t : g_savedTargets) if (t == oldN) t = newN;
    g_savedTargets.erase(std::remove(g_savedTargets.begin(), g_savedTargets.end(), nullptr), g_savedTargets.end());
}
}

void Act_SelectSavedTarget(int idx) { T.Queue("row:tselect", [idx] { if (idx >= 0 && idx < (int)g_savedTargets.size()) g_activeSaved = idx; }); }
void Act_RemoveSavedTarget(int idx) {
    T.Queue("row:tremovesaved", [idx] {
        if (idx < 0 || idx >= (int)g_savedTargets.size()) return;
        g_savedTargets.erase(g_savedTargets.begin() + idx);
        if (g_activeSaved >= (int)g_savedTargets.size()) g_activeSaved = (int)g_savedTargets.size() - 1;
    });
}
void Act_Unswap() {
    T.Queue("row:unswap", [] {
        void* m = MainHero();
        if (!m) { T.Toast("Nie znaleziono głównego ciała"); return; }
        if (Player() == m) { T.Toast("Już jesteś w swoim ciele"); return; }
        if (NpcDead(m)) { T.Toast("Twoje główne ciało nie żyje..."); return; }
        SetAsPlayer(m);
        T.Toast("Wróciłeś do bezimiennego");
    });
}

void Target_Hooks() {}
void Target_World() {}
void Target_Tick(TickCtx& c) {
    Snapshot& s = c.s;
    void* saved = SavedTarget();
    if (void* t = Target()) {
        s.tValid = true; s.tSaved = t == saved;
        s.tName = U(NpcName(t)); s.tGuild = GuildName(fld<int>(t, K.offGuild));
        s.tHp = Attr(t, 0); s.tHpMax = Attr(t, 1); s.tDead = NpcDead(t);
        s.tLevel = fld<int>(t, K.offLevel); s.tMonster = IsMonster(t);
        s.tGroup = kNpcGroupNames[NpcGroupOfGuild(fld<int>(t, K.offGuild))];
    }
    for (void* n : g_savedTargets) s.savedTargets.push_back(U(NpcName(n)));
    s.activeSaved = g_activeSaved;
}

void Target_Register() {
    const char* C = "Cel";
    T.Add(MkRow("tsave", "Zapisz cel", "dodaje NPC pod wzrokiem do listy zapisanych celów", C, "Wybór celu", K_ACTION, 0, 0, 0, [](Row&) {
        void* f = FocusNpc(); if (!f) { T.Toast("Spójrz na NPC, żeby go zapisać"); return; }
        auto it = std::find(g_savedTargets.begin(), g_savedTargets.end(), f);
        if (it == g_savedTargets.end()) { g_savedTargets.push_back(f); g_activeSaved = (int)g_savedTargets.size() - 1; }
        else g_activeSaved = (int)(it - g_savedTargets.begin());
        T.Toast("Zapisano cel: " + U(NpcName(f))); }));
    NoVal(); Icon(ICN_BOOKMARK); St(ST_OK, "działa; build 3: wiele celów naraz");
    T.Add(MkRow("tforget", "Użyj NPC pod wzrokiem", "odznacza zapisany cel (lista zostaje)", C, "Wybór celu", K_ACTION, 0, 0, 0, [](Row&) {
        g_activeSaved = -1; T.Toast("Cel: NPC pod wzrokiem"); }));
    NoVal(); Icon(ICN_EYE); St(ST_NEW, "build 3: zmienione z 'Zapomnij cel'");

    const char* A = "Akcje (też na karcie celu)";
    T.Add(MkRow("mind", "Kontrola umysłu", "cel walczy po twojej stronie i idzie za tobą", C, A, K_ACTION, 0, 0, 0,
        NeedT([](void* t) { MakeAlly(t); T.Toast(U(NpcName(t)) + " jest teraz twój"); }))); NoVal(); Icon(ICN_MAGIC);
    St(ST_NEW, "build 2: CRASH (pusty argument NPC) - przyczyna naprawiona w build 3");
    T.Add(MkRow("swap", "Zamiana ciał", "grasz celem; powrót przyciskiem w nagłówku menu", C, A, K_ACTION, 0, 0, 0,
        NeedT([](void* t) {
            if (NpcDead(t)) { T.Toast("Cel nie żyje"); return; }
            if (On("noclip")) { Row* r = T.Get("noclip"); r->on = false; }
            SetAsPlayer(t);
            T.Toast("Jesteś teraz: " + U(NpcName(t)));
        }))); NoVal(); Icon(ICN_EXCHANGE); St(ST_OK, "działa; build 3: wykrywa też zamianę przez Marvina (O)");
    T.Add(MkRow("tpull", "Przyciągnij do siebie", "", C, A, K_ACTION, 0, 0, 0,
        NeedT([](void* t) { SafeSetPos(t, Grounded(InFront(200.f))); T.Toast("Przyciągnięto"); }))); NoVal(); Icon(ICN_MAGNET); St(ST_OK, "działa");
    T.Add(MkRow("tpto", "Teleport do celu", "", C, A, K_ACTION, 0, 0, 0,
        NeedT([](void* t) { zVEC3 p = Pos(t); SafeSetPos(Player(), { p.x, p.y + 60, p.z }); }))); NoVal(); Icon(ICN_SIGNIN); St(ST_OK, "działa");
    T.Add(MkRow("yeet", "Wystrzel w powietrze", "wysokość w metrach", C, A, K_ACTION, 15, 1, 200,
        NeedT([](void* t) { zVEC3 p = Pos(t); p.y += Val("yeet") * 100.f; SafeSetPos(t, p); T.Toast("Lot!"); }))); Icon(ICN_ROCKET); St(ST_OK, "działa");
    T.Add(MkRow("tkill", "Zabij", "działa też na nieśmiertelnych", C, A, K_ACTION, 0, 0, 0,
        NeedT([](void* t) { Kill(t, Player()); T.Toast("Zabito: " + U(NpcName(t))); }))); NoVal(); Icon(ICN_KILL);
    St(ST_OK, "działa; build 3: zdejmuje flagę nieśmiertelności");
    T.Add(MkRow("trevive", "Ożyw / ulecz", "martwy = odrodzenie w tym samym miejscu", C, A, K_ACTION, 0, 0, 0, NeedT([](void* t) {
        if (!NpcDead(t)) { SetAttr(t, 0, Attr(t, 1)); T.Toast("Uleczono"); return; }
        int inst = NpcInstance(t); zVEC3 p = Pos(t);
        if (inst <= 0) { T.Toast("Nie da się odczytać instancji"); return; }
        RemoveVob(t);
        void* n = SpawnNpc(inst, Grounded({ p.x, p.y + 50, p.z }));
        ReplaceTargetRef(t, n);
        T.Toast(n ? "Odrodzono" : "Nie udało się"); })));
    NoVal(); Icon(ICN_REFRESH); St(ST_NEW, "build 2: cel znikał (zła instancja) - build 3: instancja czytana bezpośrednio z obiektu");
    T.Add(MkRow("trob", "Okradnij do zera", "cały ekwipunek celu (też założony) trafia do ciebie", C, A, K_ACTION, 0, 0, 0, NeedT([](void* t) {
        std::vector<std::pair<int, int>> got;
        Sym* itemSym = SymByName("ITEM");
        for (int cat = 0; cat < 10; cat++)
            for (int slot = 0; slot < 400; slot++) {
                int amount = CallExt("Npc_GetInvItemBySlot", { Arg::Npc(t), Arg::Int(cat), Arg::Int(slot) });
                if (amount <= 0) break;
                void* it = itemSym ? (void*)(uintptr_t)itemSym->offset : nullptr;
                if (!IsItem(it)) break;
                int inst = NpcInstance(it);
                if (std::find_if(got.begin(), got.end(), [&](auto& g) { return g.first == inst; }) == got.end()) got.push_back({ inst, amount });
            }
        int total = 0;
        for (auto& g : got) {
            CallExt("Npc_RemoveInvItems", { Arg::Npc(t), Arg::Int(g.first), Arg::Int(g.second) });
            CreateItems(Player(), g.first, g.second); total += g.second;
        }
        T.Toast("Zabrano " + std::to_string(total) + " przedmiotów (" + std::to_string(got.size()) + " rodzajów)"); })));
    NoVal(); Icon(ICN_BAG); St(ST_NEW, "build 2: zawsze 0 (pusty argument NPC) - naprawione w build 3");
    T.Add(MkRow("tstorm", "Burza ognia", "obrażenia", C, A, K_ACTION, 200, 1, 5000, NeedT([](void* t) {
        Effect("SPELLFX_FIRESTORM", Player(), t, 4, (int)Val("tstorm"), K.damFire, true); })));
    Icon(ICN_FIRE); St(ST_NEW, "build 2: brak efektu (pusty argument NPC) - naprawione w build 3");
    T.Add(MkRow("tlight", "Piorun", "obrażenia", C, A, K_ACTION, 150, 1, 5000, NeedT([](void* t) {
        Effect("SPELLFX_THUNDERBOLT", Player(), t, 2, (int)Val("tlight"), K.damMagic, true); })));
    Icon(ICN_BOLT); St(ST_NEW, "build 2: brak efektu - naprawione w build 3");
    T.Add(MkRow("tice", "Zamroź w lodzie", "", C, A, K_ACTION, 0, 0, 0, NeedT([](void* t) {
        Effect("SPELLFX_ICECUBE", Player(), t, 1, 1, K.damMagic, true); })));
    NoVal(); Icon(ICN_SNOW); St(ST_NEW, "build 2: brak efektu - naprawione w build 3");
    T.Add(MkRow("tremove", "Usuń ze świata", "", C, A, K_ACTION, 0, 0, 0, NeedT([](void* t) {
        ReplaceTargetRef(t, nullptr); RemoveVob(t); T.Toast("Puf."); }))); NoVal(); Icon(ICN_TRASH);

    const char* W = "Wygląd";
    T.Add(MkRow("tscale", "Skala modelu celu", "1 = normalnie", C, W, K_ACTION, 2, 0.1f, 10, NeedT([](void* t) {
        float v = Val("tscale");
        CallExt("Mdl_SetModelScale", { Arg::Npc(t), Arg::Flt(v), Arg::Flt(v), Arg::Flt(v) });
        T.Toast("Skala x" + std::to_string(v).substr(0, 4)); }), true));
    Icon(ICN_EXPAND); St(ST_NEW, "build 2: brak efektu - wspólna przyczyna (pusty argument NPC) naprawiona, sprawdź ponownie");
    T.Add(MkRow("tfat", "Grubość celu", "0 = normalnie, ujemne = chudy", C, W, K_ACTION, 3, -3, 10, NeedT([](void* t) {
        CallExt("Mdl_SetModelFatness", { Arg::Npc(t), Arg::Flt(Val("tfat")) }); }), true));
    St(ST_NEW, "build 2: brak efektu - wspólna przyczyna naprawiona, sprawdź ponownie");
    T.Add(MkRow("pscale", "Skala modelu bohatera", "1 = normalnie", C, W, K_ACTION, 1, 0.1f, 10, [](Row& r) {
        CallExt("Mdl_SetModelScale", { Arg::Npc(Player()), Arg::Flt(r.val), Arg::Flt(r.val), Arg::Flt(r.val) }); }, true));
    St(ST_NEW, "build 2: brak efektu - wspólna przyczyna naprawiona");
}
