// Photo studio: portraits (NPCs) and icons (items) rendered by the game.
// A clone of the character (fresh from its script instance -> neutral standing pose) or the item
// is placed high above the player, the camera is parked in front of it for two frames (with and
// without the subject) and Present cuts the subject out. The camera is restored exactly afterwards.
// While anything is being photographed the UI covers the screen, so the camera jump is never seen.
#include "mods.h"
#include "../portrait.h"
#include "MinHook.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <deque>

using namespace g1;
using namespace gx;

namespace {
struct Job { int kind; void* npc; int sym; std::string key; bool force; int look[4] = { -1, 0, 0, 0 }; };   // kind 0 real npc, 1 clone, 2 item
std::deque<Job> g_jobs;
bool g_override = false, g_extThisFrame = false;
float g_m[12], g_ext[12], g_camSave[12];
Job g_cur; bool g_active = false;
void* g_subj = nullptr; bool g_spawned = false; uint32_t g_flags = 0; bool g_hidden = false; bool g_turned = false;
int g_settle = 0, g_wait = 0, g_done = 0, g_total = 0; bool g_batch = false;
zVEC3 g_studio;
std::set<void*> g_customLook;

using fnDoAI = void(__fastcall*)(void*, void*, void*, int*);
fnDoAI oDoAI = nullptr;
void WriteCam(void* cam, const float* m) {
    if (!cam) return;
    float* t = &fld<float>(cam, VOB_TRAFO);
    for (int i = 0; i < 12; i++) if (i != 3 && i != 7 && i != 11) t[i] = m[i];
    SafeSetPos(cam, { m[3], m[7], m[11] });
}
void __fastcall hkDoAI(void* self, void* edx, void* vob, int* x) {
    if (g_override) { WriteCam(vob, g_m); return; }
    if (g_extThisFrame) { g_extThisFrame = false; WriteCam(vob, g_ext); return; }   // noclip: camera rigidly attached to the hero
    oDoAI(self, edx, vob, x);
}
void LookAt(const zVEC3& target, const zVEC3& from) {
    zVEC3 f{ target.x - from.x, target.y - from.y, target.z - from.z };
    float fl = std::sqrt(f.x * f.x + f.y * f.y + f.z * f.z); if (fl < 1e-3f) fl = 1; f.x /= fl; f.y /= fl; f.z /= fl;
    zVEC3 r{ f.z, 0, -f.x };
    float rl = std::sqrt(r.x * r.x + r.z * r.z); if (rl < 1e-4f) { r = { 1, 0, 0 }; rl = 1; } r.x /= rl; r.z /= rl;
    zVEC3 u{ f.y * r.z - f.z * r.y, f.z * r.x - f.x * r.z, f.x * r.y - f.y * r.x };
    float m[12] = { r.x, u.x, f.x, from.x, r.y, u.y, f.y, from.y, r.z, u.z, f.z, from.z };
    std::copy(m, m + 12, g_m);
}
bool Frame() {
    void* v = g_subj;
    const float* b = &fld<float>(v, VOB_BBOX);
    if (g_cur.kind == 2) {
        zVEC3 c{ (b[0] + b[3]) * 0.5f, (b[1] + b[4]) * 0.5f, (b[2] + b[5]) * 0.5f };
        float r = std::max({ b[3] - b[0], b[4] - b[1], b[5] - b[2] }) * 0.5f;
        float d = std::max(25.f, r * 2.9f);
        LookAt(c, { c.x + d * 0.62f, c.y + d * 0.45f, c.z + d * 0.62f });
        return true;
    }
    zVEC3 p = Pos(v), at = AtVec(v);
    float l = std::sqrt(at.x * at.x + at.z * at.z); if (l < 1e-3f) return false;
    at.x /= l; at.z /= l;
    zVEC3 head;
    bool mon = IsMonster(v);
    if (!HeadPos(v, head)) head = { p.x, b[4] - (mon ? 45.f : 14.f), p.z };
    float size = std::max(b[3] - b[0], b[5] - b[2]);
    float d = mon ? std::max(130.f, size * 1.3f) : 88.f;
    zVEC3 tgt{ head.x, head.y - (mon ? 0.f : 6.f), head.z };
    LookAt(tgt, { tgt.x + at.x * d, tgt.y + (mon ? d * 0.15f : 4.f), tgt.z + at.z * d });
    return true;
}
void Hold() {   // keep the subject floating at the studio spot
    if (!g_spawned || !g_subj) return;
    fld<uint32_t>(g_subj, VOB_FLAGS) &= ~VOBF_PHYSICS;
    if (Dist(Pos(g_subj), g_studio) > 1.f) SafeSetPos(g_subj, g_studio);
}
void Turn(void* v) {   // 180 degrees around the vertical axis (right and at axes flip)
    float* t = &fld<float>(v, VOB_TRAFO);
    for (int i = 0; i < 3; i++) { t[i * 4 + 0] = -t[i * 4 + 0]; t[i * 4 + 2] = -t[i * 4 + 2]; }
    g_turned = !g_turned;
}
void Show(bool on) {
    if (!g_subj) return;
    uint32_t& f = fld<uint32_t>(g_subj, VOB_FLAGS);
    if (on) { f = (f & ~1u) | (g_flags & 1u); g_hidden = false; }
    else { if (!g_hidden) g_flags = f; f &= ~1u; g_hidden = true; }
}
void Finish(void* cam) {
    if (g_hidden && g_subj) Show(true);
    if (g_turned && g_subj && !g_spawned) Turn(g_subj);
    if (g_spawned && g_subj) RemoveVob(g_subj);
    if (g_override) WriteCam(cam, g_camSave);
    g_hidden = false; g_turned = false; g_override = false; g_subj = nullptr; g_spawned = false; g_active = false; g_photoStage = 0;
    g_done++;
    if (g_jobs.empty()) { g_batch = false; g_done = g_total = 0; }
}
// spawn without stealing the script instance (scripts that reference e.g. DIEGO keep the real one)
void* SpawnQuiet(int kind, int sym, const zVEC3& p) {
    Sym* s = SymAt(sym);
    int saved = s ? s->offset : 0;
    void* v = kind == 2 ? SpawnItem(sym, p) : SpawnNpc(sym, p);
    if (s) s->offset = saved;
    return v;
}
}

void Photo_Request(void* npc) {   // portrait of a character (cached per script instance)
    if (!NpcExists(npc)) return;
    std::string key = npc == MainHero() ? "PC_HERO" : InstName(npc);
    bool real = npc == MainHero() || g_customLook.count(npc) || npc == Player();
    for (auto& j : g_jobs) if (j.key == key) return;
    if (PortraitHas(key)) return;
    g_jobs.push_back({ real ? 0 : 1, npc, NpcInstance(npc), key, false });
    g_total++;
}
void Photo_Refresh(void* npc) {   // look changed: real character, overwrite cache
    if (!NpcExists(npc)) return;
    g_customLook.insert(npc);
    std::string key = npc == MainHero() ? "PC_HERO" : InstName(npc);
    g_jobs.push_front({ 0, npc, NpcInstance(npc), key, true });
    g_total++;
}
void Act_Photo(uintptr_t p, bool force) {
    T.Queue("row:photo", [p, force] { void* n = p ? (void*)p : Subject(); if (force) Photo_Refresh(n); else Photo_Request(n); });
}
// preview of a look on a stand-in (same script instance as the edited character, or any human)
void Act_LookPreview() {
    T.Queue("row:photo", [] {
        void* sb = Subject();
        int sym = -1;
        if (sb && !IsMonster(sb) && sb != MainHero()) sym = NpcInstance(sb);
        if (sym < 0) for (auto& e : g_cat.npcs) if (!e.monster && e.group != NG_MAGES) { sym = e.sym; break; }
        if (sym < 0) return;
        for (auto it = g_jobs.begin(); it != g_jobs.end();) { if (it->key == "LOOKPREVIEW") it = g_jobs.erase(it); else ++it; }
        Job j{ 1, nullptr, sym, "LOOKPREVIEW", true };
        j.look[0] = (int)Val("vhead"); j.look[1] = (int)Val("vface"); j.look[2] = (int)Val("vbody"); j.look[3] = (int)Val("vskin");
        g_jobs.push_front(j); g_total++;
    });
}
void Act_PhotoItem(int sym) {
    T.Queue("row:photo", [sym] {
        Sym* s = SymAt(sym); if (!s) return;
        std::string key = "I_" + zs(s->name);
        for (auto& j : g_jobs) if (j.key == key) return;
        if (PortraitHas(key)) return;
        g_jobs.push_back({ 2, nullptr, sym, key, false }); g_total++;
    });
}
void Act_PhotoBatch(int what) {
    T.Queue("row:photobatch", [what] {
        g_batch = true;
        if (what == 0) {
            std::set<std::string> seen;
            ForEachNpc([&](void* n) {
                if (!NpcReady(n)) return;
                std::string k = n == MainHero() ? "PC_HERO" : InstName(n);
                if (seen.count(k) || PortraitHas(k)) return;
                seen.insert(k); Photo_Request(n);
            });
        } else {
            for (auto& e : g_cat.items) {
                std::string key = "I_" + e.id;
                if (PortraitHas(key)) continue;
                g_jobs.push_back({ 2, nullptr, e.sym, key, false }); g_total++;
            }
        }
        if (g_jobs.empty()) { g_batch = false; T.Toast("Wszystkie obrazki już są"); }
    });
}
void Act_PhotoCancel() { T.Queue("row:photocancel", [] { g_jobs.clear(); g_batch = false; T.Toast("Przerwano generowanie obrazków"); }); }
void Cam_Override(const float* m) { std::copy(m, m + 12, g_ext); g_extThisFrame = true; }

void Photo_Register() {
    CardTitle("Portrety i ikony");
    T.Add(MkRow("photoauto", "Portrety postaci przy wczytaniu", "przy wczytaniu świata robi portrety postaci, których jeszcze nie ma (raz, potem z pliku)",
                "Konfiguracja", "Konfiguracja", K_TOGGLE));
    T.rows.back().on = true; St(ST_NEW, "nowe w build 6");
    T.Add(MkRow("photoall", "Zrób portrety wszystkich postaci", "", "Konfiguracja", "Konfiguracja", K_ACTION, 0, 0, 0, [](Row&) { Act_PhotoBatch(0); }));
    NoVal(); St(ST_NEW, "nowe w build 6");
    T.Add(MkRow("photoitems", "Zrób ikony wszystkich przedmiotów", "około minuty; Esc przerywa", "Konfiguracja", "Konfiguracja", K_ACTION, 0, 0, 0, [](Row&) { Act_PhotoBatch(1); }));
    NoVal(); St(ST_NEW, "nowe w build 6");
}
void Photo_Hooks() {
    void** vt = reinterpret_cast<void**>(0x7D25C0);   // zCAICamera vtable, slot 4 = DoAI
    MH_CreateHook(vt[4], (LPVOID)&hkDoAI, (LPVOID*)&oDoAI);
}
void Photo_World() {
    g_jobs.clear(); g_override = false; g_hidden = false; g_subj = nullptr; g_spawned = false; g_active = false; g_photoStage = 0;
    g_customLook.clear(); g_batch = false; g_done = g_total = 0;
    if (On("photoauto")) Schedule(3.0, [] { Act_PhotoBatch(0); });
}
void Photo_Tick(TickCtx& c) {
    void* cam = fld<void*>(c.game, OG_CAMVOB);
    if (!g_active) {
        while (!g_jobs.empty() && !g_active) {
            Job j = g_jobs.front(); g_jobs.pop_front();
            if (!j.force && PortraitHas(j.key)) { g_done++; continue; }
            if (T.Crashed("photo:" + j.key)) { g_done++; continue; }   // this one crashed the game before
            zVEC3 hp = Pos(c.hero);
            g_studio = { hp.x, hp.y + 2600.f, hp.z };
            g_cur = j; g_spawned = false; g_subj = nullptr;
            if (j.kind == 0) { if (!NpcExists(j.npc) || !NpcReady(j.npc)) { g_done++; continue; } g_subj = j.npc; g_settle = 0; }
            else {
                void* v = nullptr;
                Guard("photo:" + j.key, [&] { v = SpawnQuiet(j.kind, j.sym, g_studio); });
                if (!v) { g_done++; continue; }
                g_subj = v; g_spawned = true; g_settle = j.kind == 1 ? 8 : 3;
                if (j.kind == 1) fld<int>(v, NPC_FLAGS) |= 2;   // the clone cannot be hurt
                if (j.look[0] >= 0) {
                    static const char* heads[] = { "HUM_HEAD_FIGHTER", "HUM_HEAD_BALD", "HUM_HEAD_THIEF", "HUM_HEAD_PONY", "HUM_HEAD_PSIONIC", "HUM_HEAD_FATBALD" };
                    CallExt("Mdl_SetVisualBody", { Arg::Npc(v), Arg::Str("HUM_BODY_NAKED0"), Arg::Int(j.look[2]), Arg::Int(j.look[3]),
                                                  Arg::Str(heads[std::max(0, std::min(5, j.look[0]))]), Arg::Int(j.look[1]), Arg::Int(0), Arg::Int(-1) });
                }
            }
            float* t = &fld<float>(cam, VOB_TRAFO); std::copy(t, t + 12, g_camSave);
            g_active = true; g_wait = 0; g_photoStage = 0;
            snprintf(g_photoKey, sizeof g_photoKey, "%s", j.key.c_str());
            g_photoItem = j.kind == 2;
        }
    }
    if (g_active) {
        at<float>(ZTIMER) = 1.f;   // let the clone's idle animation run even when the menu pauses the game
        Hold();
        if (g_settle > 0) {
            if (g_cur.kind == 1 && g_settle == 8 && NpcReady(g_subj)) {
                CallExt("Npc_ClearAIQueue", { Arg::Npc(g_subj) });
                CallExt("AI_StandupQuick", { Arg::Npc(g_subj) });
                SetSleeping(g_subj, false);
            }
            g_settle--;
            if (g_settle == 0) {
                if (!Frame()) { Finish(cam); }
                else { g_override = true; WriteCam(cam, g_m); g_photoStage = 1; }
            }
        } else if (g_photoStage == 0 && !g_override) {
            if (!Frame()) Finish(cam); else { g_override = true; WriteCam(cam, g_m); g_photoStage = 1; }
        } else if (g_photoStage == 2 || g_photoStage == 6) {   // background shot: subject hidden
            Show(false); WriteCam(cam, g_m);
            g_photoStage++;
        } else if (g_photoStage == 4) {                          // second pair: subject turned around
            Show(true); Turn(g_subj);
            if (!Frame()) Finish(cam); else { WriteCam(cam, g_m); g_photoStage = 5; }
        } else if (g_photoStage == 8) {
            Finish(cam);
        } else {
            WriteCam(cam, g_m);
            if (++g_wait > 60) Finish(cam);
        }
    }
    c.s.photoBusy = g_active || !g_jobs.empty();
    c.s.photoBatch = g_batch;
    c.s.photoDone = g_done; c.s.photoTotal = std::max(g_total, g_done);
    c.s.photoLabel = g_active ? g_cur.key : "";
}
