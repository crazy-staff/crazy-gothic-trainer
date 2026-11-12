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
// build 9: every job gets its own spot (previous clone may still be in the world for a frame or two),
// a short pause between jobs, and 2 frames after each change before the shot (renderer can lag a frame)
int g_slot = 0, g_cool = 0, g_delay = 0;
void* g_gone = nullptr; int g_goneWait = 0;   // last clone: the next job starts only after it has left the world
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
    if (!VobInWorld(g_subj)) EnsureNpcAt(g_subj, g_studio);
    else if (Dist(Pos(g_subj), g_studio) > 1.f) SafeSetPos(g_subj, g_studio);
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
    if (g_spawned && g_subj) { RemoveVob(g_subj); g_gone = g_subj; }
    JobEnd();
    if (g_override) WriteCam(cam, g_camSave);
    g_hidden = false; g_turned = false; g_override = false; g_subj = nullptr; g_spawned = false; g_active = false; g_photoStage = 0;
    g_done++; g_cool = 6;
    if (g_jobs.empty()) { g_batch = false; g_done = g_total = 0; }
}
// spawn without stealing the script instance (scripts that reference e.g. DIEGO keep the real one)
void* SpawnQuiet(int kind, int sym, const zVEC3& p) {
    Sym* s = SymAt(sym);
    int saved = s ? s->offset : 0;
    // build 10: clones never get a daily routine - with one the game beamed them to their routine spot and,
    // far from the player, took them out of the world (no body in the shot: empty / broken portraits)
    void* v = kind == 2 ? SpawnItem(sym, p) : SpawnNpc(sym, p, true);
    if (s) s->offset = saved;
    if (v && kind != 2 && (Dist(Pos(v), p) > 50.f || !VobInWorld(v))) EnsureNpcAt(v, p);
    return v;
}
}

void Photo_Request(void* npc) {   // portrait of a character (cached per script instance)
    if (!NpcExists(npc)) return;
    std::string key = npc == MainHero() ? "PC_HERO" : InstName(npc);
    // build 11: always the real character where it stands (clones were never enabled by the game: empty shots,
    // "testmodel" leftovers and crashes) - portraits are made on the fly for targeted / saved characters
    if (!NpcReady(npc) || NpcDead(npc)) return;
    for (auto& j : g_jobs) if (j.key == key) return;
    if (PortraitHas(key)) return;
    g_jobs.push_back({ 0, npc, NpcInstance(npc), key, false });
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
// build 13: look preview on a stand-in again (clones get a body now - oCNpc::Enable). Same instance as the edited
// character; for the hero (or monsters) a plain human instance - never the TESTMODELL ones.
void Act_LookPreview() {
    T.Queue("row:photo", [] {
        void* sb = Subject();
        int sym = -1;
        if (sb && !IsMonster(sb) && sb != MainHero() && sb != Player()) sym = NpcInstance(sb);
        for (const char* id : { "PC_FIGHTER", "PC_THIEF", "PC_MAGE" }) if (sym < 0) sym = SymIndex(id);
        if (sym < 0) for (auto& e : g_cat.npcs) if (!e.monster && e.id.find("TEST") == std::string::npos && e.group != NG_MAGES) { sym = e.sym; break; }
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
        if (what == 0 || what == 2) { g_batch = false; return; }   // build 11: no batch portraits of characters
        if (false) {
            std::set<std::string> seen;
            ForEachNpc([&](void* n) {
                if (!NpcReady(n)) return;
                std::string k = n == MainHero() ? "PC_HERO" : InstName(n);
                if (seen.count(k) || PortraitHas(k)) return;
                seen.insert(k); Photo_Request(n);
            });
        } else if (what == 2) {   // every character instance in the scripts (also ones the game has not loaded)
            if (!PortraitHas("PC_HERO") && MainHero()) Photo_Request(MainHero());
            std::set<std::string> seen;
            for (auto& e : g_cat.npcs) {
                if (seen.count(e.id) || PortraitHas(e.id)) continue;
                seen.insert(e.id);
                g_jobs.push_back({ 1, nullptr, e.sym, e.id, false }); g_total++;
            }
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
// regenerate one cached picture by its key (compendium)
void Act_PhotoKey(const std::string& key) {
    T.Queue("row:photo", [key] {
        if (key == "PC_HERO") { if (MainHero()) Photo_Refresh(MainHero()); return; }
        bool item = key.rfind("I_", 0) == 0;
        if (!item) {   // characters: photograph the real one if it is loaded
            void* found = nullptr;
            ForEachNpc([&](void* n) { if (!found && InstName(n) == key && NpcReady(n) && !NpcDead(n)) found = n; });
            if (found) Photo_Refresh(found); else T.Toast("Tej postaci nie ma w pobliżu - podejdź i wyceluj w nią");
            return;
        }
        int sym = SymIndex((item ? key.substr(2) : key).c_str());
        if (sym < 0) { T.Toast("Nie znam instancji " + key); return; }
        for (auto it = g_jobs.begin(); it != g_jobs.end();) { if (it->key == key) it = g_jobs.erase(it); else ++it; }
        g_jobs.push_front({ item ? 2 : 1, nullptr, sym, key, true }); g_total++;
    });
}
// build 15: delete every cached picture of one kind (1 items, 0 characters); items are generated again
void Act_PhotoReset(int what) {
    T.Queue("row:photoreset", [what] {
        int n = 0;
        for (auto& k : PortraitKeys()) {
            bool item = k.rfind("I_", 0) == 0;
            if ((what == 1) == item) { PortraitDelete(k); n++; }
        }
        g_jobs.clear();
        T.Toast("Usunięto " + std::to_string(n) + (what == 1 ? " ikon - generuję od nowa" : " portretów"));
        if (what == 1) Act_PhotoBatch(1);
    });
}
void Act_PhotoCancel() { T.Queue("row:photocancel", [] { g_jobs.clear(); g_batch = false; T.Toast("Przerwano generowanie obrazków"); }); }
// build 11: also write the camera right away (like the portrait studio does every frame - that path is proven);
// the DoAI hook alone did not turn the camera in no-clip
void Cam_Override(const float* m) {
    std::copy(m, m + 12, g_ext); g_extThisFrame = true;
    if (void* g = Game()) WriteCam(fld<void*>(g, OG_CAMVOB), m);
}

// build 11: remove stand-in clones left by builds 6-10 ("...TESTMODELL" instances from the look preview). The game
// may insert old ones later when you walk near, so this also runs every 10 s.
void CleanClones() {
    std::vector<void*> junk;
    ForEachNpc([&](void* n) { std::string k = InstName(n); if (k.find("TESTMODELL") != std::string::npos && n != Player() && n != MainHero()) junk.push_back(n); });
    for (void* n : junk) RemoveVob(n);
    if (!junk.empty()) { RefreshNpcSet(); Log("usunieto %d klonow testmodel", (int)junk.size()); }
}
void Photo_Register() {
    CardTitle("Portrety i ikony");
    T.Add(MkRow("photoitems", "Zrób ikony wszystkich przedmiotów", "około minuty; Esc przerywa", "Konfiguracja", "Konfiguracja", K_ACTION, 0, 0, 0, [](Row&) { Act_PhotoBatch(1); }));
    NoVal(); St(ST_NEW, "nowe w build 6");
}
void Photo_Hooks() {
    void** vt = reinterpret_cast<void**>(0x7D25C0);   // zCAICamera vtable, slot 4 = DoAI
    MH_CreateHook(vt[4], (LPVOID)&hkDoAI, (LPVOID*)&oDoAI);
}
void Photo_World() {
    g_gone = nullptr; g_goneWait = 0;
    g_jobs.clear(); g_override = false; g_hidden = false; g_subj = nullptr; g_spawned = false; g_active = false; g_photoStage = 0;
    g_customLook.clear(); g_batch = false; g_done = g_total = 0;
    // build 11: remove stand-in clones left by builds 6-10 ("...TESTMODELL", look preview, portrait studio)
    Schedule(2.0, [] { CleanClones(); });
}
void Photo_Tick(TickCtx& c) {
    static double cleanAt = 0;
    if (T.gameClock - cleanAt > 10.0) { cleanAt = T.gameClock; CleanClones(); }
    void* cam = fld<void*>(c.game, OG_CAMVOB);
    if (!g_active && g_gone) {   // strictly one character at a time: wait until the previous clone is really gone
        RefreshNpcSet();
        if (!NpcExists(g_gone) || ++g_goneWait > 120) { g_gone = nullptr; g_goneWait = 0; }
    }
    else if (!g_active && g_cool > 0) { g_cool--; }
    else if (!g_active) {
        while (!g_jobs.empty() && !g_active) {
            Job j = g_jobs.front(); g_jobs.pop_front();
            if (!j.force && PortraitHas(j.key)) { g_done++; continue; }
            if (T.Crashed("photo:" + j.key)) { g_done++; continue; }   // this one crashed the game before
            zVEC3 hp = Pos(c.hero);
            g_slot = (g_slot + 1) % 4;
            g_studio = { hp.x, hp.y + 3500.f + g_slot * 500.f, hp.z };   // straight up: sky behind the subject
            g_cur = j; g_spawned = false; g_subj = nullptr;
            JobBegin("photo:" + j.key);
            if (j.kind == 0) { if (!NpcExists(j.npc) || !NpcReady(j.npc)) { g_done++; JobEnd(); continue; } g_subj = j.npc; g_settle = 0; }
            else {
                void* v = SpawnQuiet(j.kind, j.sym, g_studio);
                RefreshNpcSet();
                if (!v) { g_done++; JobEnd(); continue; }
                g_subj = v; g_spawned = true; g_settle = j.kind == 1 ? 14 : 4;
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
            if (g_cur.kind == 1 && g_settle == 14 && NpcReady(g_subj)) {
                CallExt("Npc_ClearAIQueue", { Arg::Npc(g_subj) });
                CallExt("AI_StandupQuick", { Arg::Npc(g_subj) });
                SetSleeping(g_subj, false);
            }
            g_settle--;
            if (g_settle == 0) {
                if (g_cur.kind != 2 && (!NpcReady(g_subj) || !VobInWorld(g_subj))) { Log("portret %s: brak modelu, pomijam", g_cur.key.c_str()); Finish(cam); }
                else if (!Frame()) { Finish(cam); }
                else { g_override = true; WriteCam(cam, g_m); g_delay = 2; g_photoStage = 0; }
            }
        } else if (g_delay > 0) {                                // let the renderer catch up before a shot
            WriteCam(cam, g_m);
            if (--g_delay == 0) g_photoStage++;                  // even -> odd = capture in Present
        } else if (g_photoStage == 0 && !g_override) {
            if (!Frame()) Finish(cam); else { g_override = true; WriteCam(cam, g_m); g_delay = 2; }
        } else if (g_photoStage == 2 || g_photoStage == 6) {   // background shot: subject hidden
            Show(false); WriteCam(cam, g_m);
            g_delay = 2;
        } else if (g_photoStage == 4) {                          // second pair: subject turned around
            Show(true); Turn(g_subj);
            if (!Frame()) Finish(cam); else { WriteCam(cam, g_m); g_delay = 2; }
        } else if (g_photoStage == 8) {
            Finish(cam);
        } else {
            WriteCam(cam, g_m);
            if (++g_wait > 90) Finish(cam);
        }
    }
    c.s.photoBusy = g_active || !g_jobs.empty();
    c.s.photoBatch = g_batch;
    c.s.photoDone = g_done; c.s.photoTotal = std::max(g_total, g_done);
    c.s.photoLabel = g_active ? g_cur.key : "";
}
