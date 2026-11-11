// Target module: saved targets (with location/instance info), mind control, body swap, target manipulation.
#include "mods.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>

using namespace g1;
using namespace gx;

namespace {
void ReplaceTargetRef(void* oldN, void* newN) {
    for (auto& t : g_savedTargets) if (t == oldN) t = newN;
    g_savedTargets.erase(std::remove(g_savedTargets.begin(), g_savedTargets.end(), nullptr), g_savedTargets.end());
}
struct Cached { std::string place, inst; double at = -10; };
std::unordered_map<void*, Cached> g_info;

const Cached& Info(void* n) {
    Cached& c = g_info[n];
    if (NowSec() - c.at > 1.0) { c.at = NowSec(); c.place = PlaceName(Pos(n)); c.inst = InstName(n); }
    return c;
}
void Respawn(void* t) {
    int inst = NpcInstance(t); zVEC3 p = Pos(t);
    if (inst <= 0) { T.Toast("Nie da się odczytać instancji"); return; }
    RemoveVob(t);
    void* n = SpawnNpc(inst, Grounded({ p.x, p.y + 50, p.z }));
    ReplaceTargetRef(t, n);
    T.Toast(n ? "Ożywienie w miejscu nie wyszło - odrodzono postać" : "Nie udało się");
}
// Revive the same object (keeps inventory, guild changes, quest flags): HP back, stand up, resume AI.
// If the body is still lying after 2 s the engine refused -> fall back to a respawn.
void Revive(void* t) {
    if (!NpcDead(t)) { SetAttr(t, 0, Attr(t, 1)); T.Toast("Uleczono: " + U(NpcName(t))); return; }
    SetAttr(t, 0, std::max(1, Attr(t, 1)));
    if (NpcReady(t)) {
        CallExt("Npc_ClearAIQueue", { Arg::Npc(t) });
        CallExt("AI_StandupQuick", { Arg::Npc(t) });
        if (IsMonster(t)) {
            int st = K.offStartAi >= 0 ? fld<int>(t, K.offStartAi) : -1;
            if (st <= 0) st = K.stMmAll;
            if (st > 0) CallExt("AI_StartState", { Arg::Npc(t), Arg::Int(st), Arg::Int(1), Arg::Str("") });
        } else CallExt("AI_ContinueRoutine", { Arg::Npc(t) });
    }
    T.Toast("Ożywiono: " + U(NpcName(t)));
    Schedule(2.0, [t] {
        if (!NpcExists(t)) return;
        int bsDead = ConstInt("BS_DEAD", 0) & 0x7FFF;
        int bs = CallExt("Npc_GetBodyState", { Arg::Npc(t) }) & 0x7FFF;
        if (NpcDead(t) || (bsDead && bs == bsDead)) Guard("row:trevive:respawn", [t] { Respawn(t); });
    });
}
}

void Act_SelectSavedTarget(int idx) { T.Queue("row:tselect", [idx] { if (idx >= 0 && idx < (int)g_savedTargets.size()) g_activeSaved = idx; }); }
void Act_SelectSavedPtr(uintptr_t p) {
    T.Queue("row:tselect", [p] {
        for (size_t i = 0; i < g_savedTargets.size(); i++) if ((uintptr_t)g_savedTargets[i] == p) g_activeSaved = (int)i;
    });
}
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
void Target_World() { g_info.clear(); }
void Target_Tick(TickCtx& c) {
    Snapshot& s = c.s;
    void* saved = SavedTarget();
    zVEC3 hp = Pos(c.hero);
    if (void* t = Target()) {
        s.tValid = true; s.tSaved = t == saved; s.tPtr = (uintptr_t)t;
        s.tName = U(NpcName(t)); s.tGuild = GuildName(fld<int>(t, K.offGuild));
        s.tHp = Attr(t, 0); s.tHpMax = Attr(t, 1); s.tDead = NpcDead(t);
        s.tLevel = fld<int>(t, K.offLevel); s.tMonster = IsMonster(t);
        s.tGroup = kNpcGroupNames[NpcGroupOfGuild(fld<int>(t, K.offGuild))];
        const Cached& ci = Info(t);
        s.tInst = ci.inst; s.tPlace = ci.place;
        zVEC3 p = Pos(t); s.tx = p.x; s.ty = p.y; s.tz = p.z;
    }
    for (void* n : g_savedTargets) {
        s.savedTargets.push_back(U(NpcName(n)));
        SavedInfo si{};
        si.ptr = (uintptr_t)n; si.name = U(NpcName(n));
        const Cached& ci = Info(n);
        si.inst = ci.inst; si.place = ci.place;
        si.guild = GuildName(fld<int>(n, K.offGuild));
        si.group = kNpcGroupNames[NpcGroupOfGuild(fld<int>(n, K.offGuild))];
        zVEC3 p = Pos(n); si.x = p.x; si.y = p.y; si.z = p.z; si.dist = Dist(p, hp) / 100.f;
        si.hp = Attr(n, 0); si.hpmax = Attr(n, 1); si.level = fld<int>(n, K.offLevel); si.dead = NpcDead(n); si.monster = IsMonster(n);
        // head on screen (for the portrait): top of the bounding box
        const float* b = &fld<float>(n, VOB_BBOX);
        float x0, y0, x1, y1;
        float top = b[4], headC = top - (si.monster ? 40.f : 14.f);
        if (!si.dead && si.dist < 9.f && Project({ p.x, headC, p.z }, x0, y0) && Project({ p.x, headC + 30.f, p.z }, x1, y1)) {
            si.onScreen = x0 > 0.05f && x0 < 0.95f && y0 > 0.05f && y0 < 0.95f;
            si.headX = x0; si.headY = y0; si.headR = std::fabs(y1 - y0);
        }
        s.saved.push_back(std::move(si));
    }
    s.activeSaved = g_activeSaved;
}

void Target_Register() {
    const char* C = "Postacie";
    const char* S = "Cel";
    CardTitle("Wybór celu");
    T.Add(MkRow("tsave", "Zapisz cel", "dodaje NPC pod wzrokiem do listy zapisanych", C, S, K_ACTION, 0, 0, 0, [](Row&) {
        void* f = FocusNpc(); if (!f) { T.Toast("Spójrz na NPC, żeby go zapisać"); return; }
        auto it = std::find(g_savedTargets.begin(), g_savedTargets.end(), f);
        if (it == g_savedTargets.end()) { g_savedTargets.push_back(f); g_activeSaved = (int)g_savedTargets.size() - 1; }
        else g_activeSaved = (int)(it - g_savedTargets.begin());
        Photo_Request(f);
        T.Toast("Zapisano cel: " + U(NpcName(f))); }));
    NoVal(); Icon(ICN_BOOKMARK); St(ST_OK, "działa; build 4: portret, instancja i miejsce na liście"); Hidden();
    T.Add(MkRow("tforget", "Użyj NPC pod wzrokiem", "odznacza zapisany cel (lista zostaje)", C, S, K_ACTION, 0, 0, 0, [](Row&) {
        g_activeSaved = -1; T.Toast("Cel: NPC pod wzrokiem"); }));
    NoVal(); Icon(ICN_EYE); St(ST_OK, "działa"); Hidden();

    CardTitle("Akcje");
    T.Add(MkRow("mind", "Kontrola umysłu", "cel walczy po twojej stronie i idzie za tobą", C, S, K_ACTION, 0, 0, 0,
        NeedT([](void* t) { MakeAlly(t); T.Toast(U(NpcName(t)) + " jest teraz twój"); }))); NoVal(); Icon(ICN_MAGIC); Hidden();
    St(ST_OK, "build 3: działa (ta sama logika co drużyna)");
    T.Add(MkRow("swap", "Zamiana ciał", "grasz celem; powrót przyciskiem w nagłówku menu", C, S, K_ACTION, 0, 0, 0,
        NeedT([](void* t) {
            if (NpcDead(t)) { T.Toast("Cel nie żyje"); return; }
            if (On("noclip")) { Row* r = T.Get("noclip"); r->on = false; }
            SetAsPlayer(t);
            T.Toast("Jesteś teraz: " + U(NpcName(t)));
        }))); NoVal(); Icon(ICN_EXCHANGE); Hidden(); St(ST_OK, "działa (też powrót)");
    T.Add(MkRow("tpull", "Przyciągnij do siebie", "", C, S, K_ACTION, 0, 0, 0,
        NeedT([](void* t) { SafeSetPos(t, Grounded(InFront(200.f))); T.Toast("Przyciągnięto"); }))); NoVal(); Icon(ICN_MAGNET); Hidden(); St(ST_OK, "działa");
    T.Add(MkRow("tpto", "Teleport do celu", "", C, S, K_ACTION, 0, 0, 0,
        NeedT([](void* t) { zVEC3 p = Pos(t); SafeSetPos(Player(), { p.x, p.y + 60, p.z }); }))); NoVal(); Icon(ICN_SIGNIN); Hidden(); St(ST_OK, "działa");
    T.Add(MkRow("yeet", "Wystrzel w powietrze", "wysokość z ustawień akcji", C, S, K_ACTION, 0, 0, 0,
        NeedT([](void* t) { zVEC3 p = Pos(t); p.y += Val("yeetv") * 100.f; SafeSetPos(t, p); T.Toast("Lot!"); }))); NoVal(); Icon(ICN_ROCKET); Hidden(); St(ST_OK, "działa");
    T.Add(MkRow("tkill", "Zabij", "działa też na nieśmiertelnych", C, S, K_ACTION, 0, 0, 0,
        NeedT([](void* t) { Kill(t, Player()); T.Toast("Zabito: " + U(NpcName(t))); }))); NoVal(); Icon(ICN_KILL); Hidden(); St(ST_OK, "działa");
    T.Add(MkRow("trevive", "Ożyw / ulecz", "ożywia tę samą postać w miejscu (ekwipunek i questy zostają)", C, S, K_ACTION, 0, 0, 0,
        NeedT([](void* t) { Revive(t); })));
    NoVal(); Icon(ICN_REFRESH); Hidden(); St(ST_NEW, "build 3: działało jako odrodzenie (nowa postać); build 4: ożywienie w miejscu, odrodzenie tylko awaryjnie");
    T.Add(MkRow("trob", "Okradnij do zera", "cały ekwipunek celu trafia do ciebie", C, S, K_ACTION, 0, 0, 0, NeedT([](void* t) {
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
    NoVal(); Icon(ICN_BAG); Hidden(); St(ST_NEW, "build 3: nie zgłoszone - sprawdź");
    T.Add(MkRow("tremove", "Usuń ze świata", "", C, S, K_ACTION, 0, 0, 0, NeedT([](void* t) {
        ReplaceTargetRef(t, nullptr); RemoveVob(t); T.Toast("Puf."); }))); NoVal(); Icon(ICN_TRASH); Hidden();

    CardTitle("Magia na celu");
    T.Add(MkRow("tstorm", "Burza ognia", "obrażenia; zaklęcie pojawia się na celu, nie jest twoje", C, S, K_ACTION, 0, 0, 0,
        NeedT([](void* t) { SpellOn(t, "SPELLFX_FIRESTORM", (int)Val("stormdmg")); })));
    Icon(ICN_FIRE); Hidden(); NoVal(); St(ST_NEW, "build 3: działało, ale rzucał bohater (pudła, agresja na ciebie); build 4: efekt na celu + bezpośrednie obrażenia");
    T.Add(MkRow("tlight", "Piorun", "obrażenia", C, S, K_ACTION, 0, 0, 0, NeedT([](void* t) { SpellOn(t, "SPELLFX_THUNDERBOLT", (int)Val("lightdmg")); })));
    Icon(ICN_BOLT); Hidden(); NoVal(); St(ST_NEW, "jak wyżej (build 4)");
    T.Add(MkRow("tice", "Zamroź w lodzie", "4 sekundy", C, S, K_ACTION, 0, 0, 0, NeedT([](void* t) { SpellOn(t, "SPELLFX_ICECUBE", 0); })));
    NoVal(); Icon(ICN_SNOW); Hidden(); St(ST_NEW, "jak wyżej (build 4)");

    CardTitle("Ustawienia akcji");
    T.Add(MkRow("yeetv", "Wysokość lotu", "m", C, S, K_VALUE, 15, 1, 200)); St(ST_OK, "działa");
    T.Add(MkRow("stormdmg", "Obrażenia burzy ognia", "", C, S, K_VALUE, 200, 1, 2000)); St(ST_NEW, "build 4");
    T.Add(MkRow("lightdmg", "Obrażenia pioruna", "", C, S, K_VALUE, 150, 1, 2000)); St(ST_NEW, "build 4");
    CardTitle("Wygląd celu");
    T.Add(MkRow("tscale", "Skala modelu", "1 = normalnie", C, S, K_ACTION, 2, 0.1f, 10, NeedT([](void* t) {
        float v = Val("tscale");
        CallExt("Mdl_SetModelScale", { Arg::Npc(t), Arg::Flt(v), Arg::Flt(v), Arg::Flt(v) }); }), true));
    Icon(ICN_EXPAND); St(ST_OK, "build 3: działa");
    T.Add(MkRow("tfat", "Grubość", "0 = normalnie, ujemne = chudy", C, S, K_ACTION, 3, -3, 10, NeedT([](void* t) {
        CallExt("Mdl_SetModelFatness", { Arg::Npc(t), Arg::Flt(Val("tfat")) }); }), true));
    St(ST_OK, "build 3: działa");
}
