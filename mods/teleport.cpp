// Teleport module: saved positions, manual coordinates, waypoints, quick jumps.
#include "mods.h"

using namespace g1;
using namespace gx;

void Act_Teleport(float x, float y, float z, const std::string& name) {
    T.Queue("row:teleport", [=] { if (!InGame()) return; SafeSetPos(Player(), { x, y + 60.f, z }); T.Toast("Teleport: " + name); });
}
void Act_SavePos(const std::string& name) {
    T.Queue("row:savepos", [=] {
        if (!InGame()) return;
        zVEC3 p = Pos(Player());
        { std::lock_guard<std::mutex> l(T.mtx);
          T.positions.push_back({ name.empty() ? "Pozycja " + std::to_string(T.positions.size() + 1) : name, p.x, p.y, p.z }); }
        T.SavePositions(WorldName());
        T.Toast("Zapisano pozycję: " + (name.empty() ? std::string("bez nazwy") : name));
    });
}
void Act_DeletePos(int idx) {
    T.Queue("row:delpos", [=] {
        { std::lock_guard<std::mutex> l(T.mtx); if (idx >= 0 && idx < (int)T.positions.size()) T.positions.erase(T.positions.begin() + idx); }
        T.SavePositions(WorldName());
    });
}

void Teleport_Hooks() {}
void Teleport_World() {}
void Teleport_Tick(TickCtx& c) { zVEC3 p = Pos(c.hero); c.s.px = p.x; c.s.py = p.y; c.s.pz = p.z; }

void Teleport_Register() {
    T.Add(MkRow("tpfwd", "Teleport do przodu", "metry, ląduje na podłożu", "Teleport", "Szybkie", K_ACTION, 20, 1, 500, [](Row& r) {
        SafeSetPos(Player(), Grounded(InFront(r.val * 100.f))); }));
    St(ST_OK, "działa"); Icon(ICN_SIGNIN);
}
