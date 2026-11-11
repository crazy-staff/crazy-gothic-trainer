// Photo module: drives the portrait capture. For two frames the game camera is parked in
// front of the character's face (camera AI paused); in the second frame the character is
// hidden so Present can cut it out of the background (see portrait.cpp).
#include "mods.h"
#include "../portrait.h"
#include "MinHook.h"
#include <algorithm>
#include <cmath>
#include <deque>

using namespace g1;
using namespace gx;

namespace {
std::deque<void*> g_req;
bool g_override = false;
float g_m[12];
void* g_npc = nullptr; uint32_t g_flags = 0; bool g_hidden = false; int g_wait = 0;

using fnDoAI = void(__fastcall*)(void*, void*, void*, int*);
fnDoAI oDoAI = nullptr;
void ApplyCam(void* cam) {
    if (!cam) return;
    float* m = &fld<float>(cam, VOB_TRAFO);
    for (int i = 0; i < 12; i++) if (i != 3 && i != 7 && i != 11) m[i] = g_m[i];
    SafeSetPos(cam, { g_m[3], g_m[7], g_m[11] });
}
void __fastcall hkDoAI(void* self, void* edx, void* vob, int* x) {
    if (g_override) { ApplyCam(vob); return; }   // camera AI paused while the photo is taken
    oDoAI(self, edx, vob, x);
}
bool SetupCam(void* npc) {
    const float* b = &fld<float>(npc, VOB_BBOX);
    zVEC3 p = Pos(npc), at = AtVec(npc);
    float l = std::sqrt(at.x * at.x + at.z * at.z);
    if (l < 1e-3f) return false;
    at.x /= l; at.z /= l;
    bool mon = IsMonster(npc);
    zVEC3 head{ p.x, b[4] - (mon ? 45.f : 14.f), p.z };
    float dist = mon ? 140.f : 58.f;
    zVEC3 cp{ head.x + at.x * dist, head.y + 3.f, head.z + at.z * dist };
    zVEC3 f{ head.x - cp.x, head.y - cp.y, head.z - cp.z };
    float fl = std::sqrt(f.x * f.x + f.y * f.y + f.z * f.z); f.x /= fl; f.y /= fl; f.z /= fl;
    zVEC3 r{ f.z, 0, -f.x };                     // cross(up, f)
    float rl = std::sqrt(r.x * r.x + r.z * r.z); r.x /= rl; r.z /= rl;
    zVEC3 u{ f.y * r.z - f.z * r.y, f.z * r.x - f.x * r.z, f.x * r.y - f.y * r.x };   // cross(f, r)
    float m[12] = { r.x, u.x, f.x, cp.x, r.y, u.y, f.y, cp.y, r.z, u.z, f.z, cp.z };
    std::copy(m, m + 12, g_m);
    return true;
}
void Finish() {
    if (g_hidden && NpcExists(g_npc)) fld<uint32_t>(g_npc, VOB_FLAGS) = (fld<uint32_t>(g_npc, VOB_FLAGS) & ~1u) | (g_flags & 1u);
    g_hidden = false; g_override = false; g_npc = nullptr; g_photoStage = 0;
}
}

void Photo_Request(void* npc) {
    if (!npc || std::find(g_req.begin(), g_req.end(), npc) != g_req.end()) return;
    g_req.push_back(npc);
}
void Act_Photo(uintptr_t p) { T.Queue("row:photo", [p] { Photo_Request(p ? (void*)p : Subject()); }); }

void Photo_Register() {}
void Photo_Hooks() {
    void** vt = reinterpret_cast<void**>(0x7D25C0);   // zCAICamera vtable, slot 4 = DoAI
    MH_CreateHook(vt[4], (LPVOID)&hkDoAI, (LPVOID*)&oDoAI);
}
void Photo_World() { g_req.clear(); g_override = false; g_hidden = false; g_npc = nullptr; g_photoStage = 0; }
void Photo_Tick(TickCtx& c) {
    void* cam = fld<void*>(c.game, OG_CAMVOB);
    switch (g_photoStage) {
    case 0:
        while (!g_req.empty()) {
            void* n = g_req.front(); g_req.pop_front();
            if (!NpcExists(n) || !NpcReady(n) || !SetupCam(n)) continue;
            g_npc = n; g_override = true; g_wait = 0;
            ApplyCam(cam);
            g_photoNpc = (uintptr_t)n;
            g_photoStage = 1;
            break;
        }
        break;
    case 2:
        if (!NpcExists(g_npc)) { Finish(); break; }
        g_flags = fld<uint32_t>(g_npc, VOB_FLAGS); g_hidden = true;
        fld<uint32_t>(g_npc, VOB_FLAGS) &= ~1u;    // showVisual off for frame B
        ApplyCam(cam); g_wait = 0;
        g_photoStage = 3;
        break;
    case 4: Finish(); break;
    default:
        ApplyCam(cam);
        if (++g_wait > 20) Finish();               // Present never captured -> give up
    }
}
