// Trainer core: queue, crash guard, self test, persistence, frame dispatch to modules.
#include "trainer.h"
#include "tester.h"
#include "mods/mods.h"
#include "MinHook.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>

using namespace g1;
using namespace gx;

Trainer T;
static const char* g_crumb = "idle";
static std::string g_crumbDyn;

double NowSec() {
    static LARGE_INTEGER f{}; if (!f.QuadPart) QueryPerformanceFrequency(&f);
    LARGE_INTEGER c; QueryPerformanceCounter(&c);
    return (double)c.QuadPart / (double)f.QuadPart;
}
Row* Trainer::Get(const std::string& id) { for (auto& r : rows) if (r.id == id) return &r; return nullptr; }
static std::string g_card, g_cardSub;
void CardTitle(const char* t) { g_card = t; g_cardSub.clear(); }
void Trainer::Add(Row r) {
    if (r.group.empty()) {
        if (!g_card.empty() && g_cardSub.empty()) g_cardSub = r.sub;
        if (r.sub != g_cardSub) { g_card = r.sub; g_cardSub = r.sub; }
        r.group = g_card;
    }
    rows.push_back(std::move(r));
}
void Trainer::Queue(const std::string& label, std::function<void()> f) { std::lock_guard<std::mutex> l(mtx); queue.emplace_back(label, std::move(f)); }
void Trainer::Toast(const std::string& s) { std::lock_guard<std::mutex> l(mtx); toast = s; toastUntil = NowSec() + 2.5; }

// ---- row builders --------------------------------------------------------------------
Row MkRow(const char* id, const char* name, const char* note, const char* cat, const char* sub, Kind k, float v, float mn,
          float mx, std::function<void(Row&)> act, bool flt) {
    Row r; r.id = id; r.name = name; r.note = note; r.cat = cat; r.sub = sub; r.kind = k; r.val = v; r.vmin = mn; r.vmax = mx;
    r.act = std::move(act); r.isFloat = flt; r.def = v; return r;
}
void St(Status s, const char* h) { T.rows.back().status = s; T.rows.back().hist = h; }
void NoVal() { T.rows.back().noValue = true; }
void Opts(std::vector<std::pair<int, std::string>> o) { T.rows.back().options = std::move(o); }
void Icon(const char* u) { T.rows.back().icon = u; }
void Hidden() { T.rows.back().hidden = true; }
void Live() { T.rows.back().live = true; }
void Excl(std::vector<std::string> ids) { T.rows.back().excl = std::move(ids); }
std::function<void(Row&)> NeedT(std::function<void(void*)> f) {
    return [f](Row&) { void* t = Target(); if (!t) { T.Toast("Brak celu - spójrz na NPC albo zapisz cel"); return; } f(t); };
}

// ==============================================================================
// crash guard
// ==============================================================================
std::string IniPath() {
    char p[MAX_PATH];
    HMODULE m = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&IniPath, &m);
    GetModuleFileNameA(m, p, MAX_PATH);
    std::string s(p);
    auto k = s.find_last_of("\\/");
    return (k == std::string::npos ? std::string(".") : s.substr(0, k)) + "\\G1CrazyTrainer.ini";
}
std::string DataPath(const char* file) { std::string p = IniPath(); return p.substr(0, p.find_last_of("\\/") + 1) + file; }

static void Pending(const std::string& key) {
    FILE* f = fopen(DataPath("G1CrazyTrainer_pending.txt").c_str(), "w");
    if (f) { fputs(key.c_str(), f); fclose(f); }
    g_crumbDyn = key;
}
static void PendingDone() { DeleteFileA(DataPath("G1CrazyTrainer_pending.txt").c_str()); g_crumbDyn.clear(); }
// long multi-frame jobs (portrait of one character): a crash anywhere inside is pinned to this key -> skipped next time
void gx::JobBegin(const std::string& key) { Pending(key); Log("> %s", key.c_str()); }
void gx::JobEnd() { PendingDone(); }
void gx::Guard(const std::string& key, const std::function<void()>& f) {
    // periodic actions (rain, party watch, photos...) only keep the name in memory: writing the pending
    // file + a log line every few hundred ms caused disk stalls (lag spikes)
    static const char* quiet[] = { "row:rain", "row:allykeep", "photo:", "row:photo", "ai:", "row:allvsme", "row:tselect" };
    for (auto q : quiet) if (key.rfind(q, 0) == 0) { std::string prev = g_crumbDyn; g_crumbDyn = key; f(); g_crumbDyn = prev; return; }
    Pending(key); Log("> %s", key.c_str()); f(); PendingDone();
}

static LONG CALLBACK Veh(EXCEPTION_POINTERS* e) {
    static int count = 0;
    DWORD c = e->ExceptionRecord->ExceptionCode;
    if (c != EXCEPTION_ACCESS_VIOLATION && c != EXCEPTION_ILLEGAL_INSTRUCTION && c != EXCEPTION_INT_DIVIDE_BY_ZERO &&
        c != EXCEPTION_STACK_OVERFLOW && c != EXCEPTION_PRIV_INSTRUCTION)
        return EXCEPTION_CONTINUE_SEARCH;
    if (count++ > 40) return EXCEPTION_CONTINUE_SEARCH;
    void* addr = e->ExceptionRecord->ExceptionAddress;
    HMODULE m = nullptr; char mod[MAX_PATH] = "?";
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)addr, &m))
        GetModuleFileNameA(m, mod, MAX_PATH);
    const char* base = strrchr(mod, '\\');
    Log("!! WYJATEK %08lx @ %p (%s+0x%lx) akcja=[%s] stan=[%s] adres=%p", c, addr, base ? base + 1 : mod,
        (unsigned long)((uintptr_t)addr - (uintptr_t)m), g_crumbDyn.c_str(), g_crumb,
        (void*)(c == EXCEPTION_ACCESS_VIOLATION ? e->ExceptionRecord->ExceptionInformation[1] : 0));
    {   // per-option event log for testers (crumb "row:<id>[:...]")
        std::string k = g_crumbDyn, id;
        if (k.rfind("row:", 0) == 0) { size_t p = k.find(':', 4); id = k.substr(4, p == std::string::npos ? std::string::npos : p - 4); }
        char b[160]; snprintf(b, sizeof b, "WYJĄTEK %08lx w %s+0x%lx", c, base ? base + 1 : mod, (unsigned long)((uintptr_t)addr - (uintptr_t)m));
        tst::Event(id, b);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
void CrashGuardInit() {
    std::string pend = DataPath("G1CrazyTrainer_pending.txt");
    std::string key;
    { std::ifstream pf(pend); if (pf) std::getline(pf, key); }
    if (!key.empty()) {
        Log("Poprzednia sesja padla podczas: %s", key.c_str());
        tst::NoteCrash(key);
        std::ofstream cf(DataPath("G1CrazyTrainer_crashes.txt"), std::ios::app);
        cf << key << "\n";
    }
    DeleteFileA(pend.c_str());
    // build 3 fixed the shared instance-argument bug -> forget crash marks recorded by older builds
    std::string ini = IniPath();
    if (GetPrivateProfileIntA("meta", "crashlistBuild", 0, ini.c_str()) < TRAINER_BUILD) {
        DeleteFileA(DataPath("G1CrazyTrainer_crashes.txt").c_str());
        WritePrivateProfileStringA("meta", "crashlistBuild", std::to_string(TRAINER_BUILD).c_str(), ini.c_str());
        Log("Lista crashy z poprzednich buildow wyczyszczona (wspolna przyczyna naprawiona)");
    }
    std::ifstream cf(DataPath("G1CrazyTrainer_crashes.txt"));
    std::string l;
    while (std::getline(cf, l)) if (!l.empty()) T.crashKeys.push_back(l);
    AddVectoredExceptionHandler(1, Veh);
}
bool Trainer::Crashed(const std::string& key) { return std::find(crashKeys.begin(), crashKeys.end(), key) != crashKeys.end(); }

// ==============================================================================
// self test of the shared engine-call layer (runs once per world)
// ==============================================================================
static void SelfTest(Snapshot& s) {
    std::vector<DiagLine> d;
    void* h = Player();
    int direct = NpcInstance(h);
    int viaScript = CallExt("Hlp_GetInstanceID", { Arg::Npc(h) });
    d.push_back({ "Argument NPC w funkcjach skryptu (Hlp_GetInstanceID)", direct == viaScript && direct > 0 });
    int g1v = CallExt("Npc_GetTrueGuild", { Arg::Npc(h) });
    d.push_back({ "Odczyt gildii przez skrypt", g1v == fld<int>(h, K.offGuild) });
    { ZStr z("test123"); d.push_back({ "Napisy silnika (zSTRING)", zs(z.s) == "test123" }); }
    zVEC3 g; bool ground = FindGround(Pos(h), 100.f, 2000.f, g);
    d.push_back({ "Raytrace (podłoże pod bohaterem)", ground && std::fabs(g.y - Pos(h).y) < 1000.f });
    d.push_back({ "Model bohatera (zCModel)", VobModel(h) != nullptr });
    d.push_back({ "Katalog: przedmioty/NPC", g_cat.items.size() > 50 && g_cat.npcs.size() > 50 });
    g_homeWorldOk = fld<void*>(h, 0xA8) == GameWorld();
    d.push_back({ "Obecność postaci w świecie (zCVob::homeWorld)", g_homeWorldOk });
    for (auto& x : d) Log("AUTOTEST %s: %s", x.ok ? "OK " : "BLAD", x.text.c_str());
    s.diag = d;
}

// ==============================================================================
// frame
// ==============================================================================
using fnRender = void(__fastcall*)(void*, void*);
static fnRender oRender = nullptr;
static void __fastcall hkRender(void* self, void* edx) {
    g_crumb = "tick"; T.lastTick = NowSec(); T.Tick();
    g_crumb = "render"; oRender(self, edx);
    g_crumb = "idle";
}
// DirectInput reads: while the menu is open (or Esc is still held after closing it) the game gets an
// empty keyboard/mouse state - typing in the menu's text boxes no longer reaches the game
using fnDIState = long(__stdcall*)(void*, unsigned long, void*);
using fnDIData = long(__stdcall*)(void*, unsigned long, void*, unsigned long*, unsigned long);
static fnDIState oDIState = nullptr; static fnDIData oDIData = nullptr;
static volatile bool g_blockInput = false;
// build 9 (#20): in no-clip the mouse turns the hero (like normal running) instead of orbiting the camera
volatile bool g_stealMouse = false;
volatile long g_mouseDx = 0, g_mouseDy = 0;
static long __stdcall hkDIState(void* self, unsigned long cb, void* data) {
    long r = oDIState(self, cb, data);
    if (g_blockInput && r == 0 && data) memset(data, 0, cb);
    else if (g_stealMouse && r == 0 && data && self == at<void*>(DI_MOUSE) && cb >= 8) {
        long* m = (long*)data; g_mouseDx += m[0]; g_mouseDy += m[1]; m[0] = 0; m[1] = 0;
    }
    return r;
}
static long __stdcall hkDIData(void* self, unsigned long cb, void* rg, unsigned long* n, unsigned long fl) {
    long r = oDIData(self, cb, rg, n, fl);
    if (g_blockInput && r == 0 && n) *n = 0;
    else if (g_stealMouse && r == 0 && n && rg && self == at<void*>(DI_MOUSE) && cb >= 8 && !(fl & 1 /*DIGDD_PEEK*/)) {
        for (unsigned long i = 0; i < *n; i++) {
            unsigned long* o = (unsigned long*)((char*)rg + i * cb);   // DIDEVICEOBJECTDATA: dwOfs, dwData, ...
            if (o[0] == 0) { g_mouseDx += (long)o[1]; o[1] = 0; }       // DIMOFS_X
            else if (o[0] == 4) { g_mouseDy += (long)o[1]; o[1] = 0; }  // DIMOFS_Y
        }
    }
    return r;
}
static void HookDirectInput() {
    static bool done = false;
    void* kbd = at<void*>(DI_KBD);
    if (done || !kbd) return;
    done = true;
    void** vt = *reinterpret_cast<void***>(kbd);
    MH_CreateHook(vt[9], (LPVOID)&hkDIState, (LPVOID*)&oDIState);
    MH_CreateHook(vt[10], (LPVOID)&hkDIData, (LPVOID*)&oDIData);
    MH_EnableHook(vt[9]); MH_EnableHook(vt[10]);
}
// CGameManager::Menu(int) - the game's own menu (Esc / main menu) runs inside this call
using fnMenu = void(__fastcall*)(void*, void*, int);
static fnMenu oMenu = nullptr;
static void __fastcall hkMenu(void* self, void* edx, int a) { T.inGameMenu = true; oMenu(self, edx, a); T.inGameMenu = false; }
void InstallGameHooks() {
    MH_CreateHook((LPVOID)F_OCGAME_RENDER, (LPVOID)&hkRender, (LPVOID*)&oRender);
    MH_CreateHook((LPVOID)0x4279F0, (LPVOID)&hkMenu, (LPVOID*)&oMenu);
    Player_Hooks(); Items_Hooks(); Npcs_Hooks(); Target_Hooks(); Chaos_Hooks(); World_Hooks(); Esp_Hooks(); Teleport_Hooks(); Marvin_Hooks(); Photo_Hooks();
    MH_EnableHook(MH_ALL_HOOKS);
}

void Trainer::Tick() {
    std::vector<std::pair<std::string, std::function<void()>>> q;
    { std::lock_guard<std::mutex> l(mtx); q.swap(queue); }
    bool inGame = InGame();
    if (inGame) K.Refresh();
    if (inGame) RefreshNpcSet();
    for (auto& f : q) Guard(f.first, f.second);

    // input ownership: menu owns everything, noclip owns the keyboard
    // re-asserted 4x per second only: doing it every frame (DirectInput unacquire) caused stutter
    static bool lastK = true, lastM = true; static double lastInput = 0;
    // after the menu closes the game gets the keyboard back only once Esc is released
    // (otherwise the still-pressed Esc opens the game's pause menu)
    static bool escHold = false;
    if (menuOpen) escHold = true;
    else if (escHold && !(GetAsyncKeyState(VK_ESCAPE) & 0x8000)) escHold = false;
    bool wantK = !menuOpen && !escHold && !((On("noclip") || On("glide")) && inGame), wantM = !menuOpen;
    // build 14 (UI-02): Win+Shift+S - the game holds mouse and keyboard through DirectInput, so the snipping overlay
    // could not be used. A pressed Windows key hands both back to Windows for 8 s and frees the cursor.
    static double winAt = -100;
    if (((GetAsyncKeyState(VK_LWIN) | GetAsyncKeyState(VK_RWIN)) & 0x8000) && On("winkeyfix")) {
        if (NowSec() - winAt > 8.0) { ClipCursor(nullptr); Log("klawisz Windows: oddaję mysz i klawiaturę systemowi na 8 s"); }
        winAt = NowSec();
    }
    if (NowSec() - winAt < 8.0) { wantK = false; wantM = false; ClipCursor(nullptr); }
    HookDirectInput();
    g_blockInput = menuOpen || escHold;
    if (wantK != lastK || wantM != lastM || (menuOpen && NowSec() - lastInput > 0.03) || ((!wantK || !wantM) && NowSec() - lastInput > 0.25)) {
        SetGameInput2(wantK, wantM); lastK = wantK; lastM = wantM; lastInput = NowSec();
    }

    Snapshot s;
    if (!inGame) {
        std::lock_guard<std::mutex> l(mtx); std::string mo = snap.marvinOut; auto dg = snap.diag; snap = s; snap.marvinOut = mo; snap.diag = dg;
        return;
    }
    static void* lastWorld = nullptr;
    static std::vector<DiagLine> diag;
    if (!g_cat.built) g_cat.Build();
    RefreshNpcSet();
    if (GameWorld() != lastWorld) {
        lastWorld = GameWorld();
        g_cat.BuildWaypoints();
        ClearWorldState();
        TrackMainHero();
        Player_World(); Items_World(); Npcs_World(); Target_World(); Chaos_World(); World_World(); Esp_World(); Teleport_World(); Marvin_World(); Photo_World();
        LoadPositions(WorldName());
        Snapshot t; SelfTest(t); diag = t.diag;
    }
    TrackMainHero();

    void* h = Player();
    bool paused = menuOpen && On("pausemenu");
    float frameSec = std::min(0.1f, at<float>(ZTIMER + 4) / 1000.f);
    if (!paused && frameSec > 0) gameClock += frameSec;
    TickCtx c{ h, Game(), std::min(0.1f, at<float>(ZTIMER + 4) / 1000.f), paused, s };
    struct { const char* n; void (*f)(TickCtx&); } mods[] = {
        { "player", Player_Tick }, { "world", World_Tick }, { "chaos", Chaos_Tick }, { "npcs", Npcs_Tick },
        { "target", Target_Tick }, { "items", Items_Tick }, { "teleport", Teleport_Tick }, { "marvin", Marvin_Tick }, { "esp", Esp_Tick }, { "photo", Photo_Tick } };
    g_crumb = "schedule"; RunSchedule();
    for (auto& m : mods) { g_crumb = m.n; m.f(c); }
    g_crumb = "tick";
    s.inGame = true;
    s.paused = paused;
    s.diag = diag;
    std::lock_guard<std::mutex> l(mtx);
    s.marvinOut = snap.marvinOut;
    snap = std::move(s);
}

// ==============================================================================
// rows, hotkeys
// ==============================================================================
void Trainer::Init() {
    rows.clear();
    Player_Register(); Items_Register(); Npcs_Register(); Target_Register(); Chaos_Register(); World_Register();
    Esp_Register(); Teleport_Register(); Marvin_Register(); Photo_Register();
    for (auto& r : rows) if (Crashed("row:" + r.id)) { r.status = ST_CRASH; r.hist = "crashowało w poprzedniej sesji (wykryte automatycznie)"; }
    // statuses confirmed by the tester's reports (build 9 + 10) - overrides what the modules registered
    static const char* okIds[] = { "hclock", "hcpocket", "hcsneak", "nohit", "noweapon", "mana", "ncignore", "kitwar", "kitarcher", "kitmage",
        "kitthief", "trevive", "trob", "tremove", "tcast", "tfreeze", "tspellimm", "killallimm", "allvsme", "thanos", "thanosimm", "meteor",
        "meteorimm", "rain", "rainimm", "qsallies", "espskel", "sprint", "espbox", "espchestinv", "arena", "marvin", "espitm", "espnpc",
        "ncmouse", "allykeep", "peace", "killall", "tpto", "tpull", "noclip", "speedallies", "arenareward", "hud", "hudalpha", "hudsize",
        "hudfps", "hudpos", "hudspeed", "hudtime", "hudworld", "hudstats", "hudtarget",
        "pvauto", "esptype", "rescuedialog", "rescuehero", "rescuetarget" };
    for (auto id : okIds) if (Row* r = Get(id)) { r->status = ST_OK; r->hist = "potwierdzone przez testera (build 9-13)"; }
    struct Bad { const char* id; const char* why; };
    static const Bad bad[] = { { "allyfightbreak", "build 13: krwiopijca dalej leciał do przeciwnika; build 14: potwory też, cel = ty, 8 s nie widzi innych" },
        { "allynotalk", "build 11: po pierwszej rozmowie towarzysz wracał do rutyny; build 12: okno dialogu się blokowało; build 13: drużyna nie jest ruszana w czasie rozmowy + Ratunek › Zamknij rozmowę" } };
    for (auto& b : bad) if (Row* r = Get(b.id)) { r->status = ST_BUG; r->hist = b.why; }
    tst::Load();   // tester history (also turns a crash from the previous session into a draft report)
}
void ApplyRow(Row& r) {
    Row* rp = &r;
    switch (r.kind) {
    case K_TOGGLE: case K_SLIDER:
        if (!r.on) {   // mutually exclusive features
            for (auto& id : r.excl) if (Row* o = T.Get(id)) if (o->on) {
                T.Toast("Nie możesz włączyć \"" + r.name + "\", bo masz włączone \"" + o->name + "\"");
                return;
            }
        }
        r.on = !r.on;
        if (r.act) T.Queue("row:" + r.id, [rp] { if (InGame()) rp->act(*rp); });
        T.Toast(r.name + ": " + (r.on ? "ON" : "OFF"));
        tst::Event(r.id, r.on ? (r.kind == K_SLIDER ? "włączono (wartość " + std::to_string(r.val).substr(0, 6) + ")" : std::string("włączono")) : std::string("wyłączono"));
        break;
    case K_SET:
        r.on = !r.on;
        T.Toast(r.on ? r.name + " = " + std::to_string((int)r.val) : r.name + ": przywrócono wartość gry");
        tst::Event(r.id, r.on ? "ustawiono " + std::to_string((int)r.val) : std::string("przywrócono wartość gry"));
        break;
    case K_ACTION: case K_CHOICE:
        if (r.act) T.Queue("row:" + r.id, [rp] { if (InGame()) rp->act(*rp); });
        tst::Event(r.id, "wykonano (wartość " + std::to_string(r.val).substr(0, 6) + ")" + (InGame() ? "" : " - poza grą, zignorowane"));
        break;
    case K_VALUE: break;
    }
    T.Save();
}
void Trainer::Hotkey(int vk) {
    if (vk == VK_CONTROL || vk == VK_MENU || vk == VK_SHIFT || vk == VK_LCONTROL || vk == VK_RCONTROL || vk == VK_LSHIFT || vk == VK_RSHIFT) return;
    int mods = ((GetKeyState(VK_CONTROL) & 0x8000) ? HK_CTRL : 0) | ((GetKeyState(VK_MENU) & 0x8000) ? HK_ALT : 0) | ((GetKeyState(VK_SHIFT) & 0x8000) ? HK_SHIFT : 0);
    for (auto& r : rows) if (r.hotkey && r.hotkey == (vk | mods) && r.kind != K_VALUE) ApplyRow(r);
}

// ==============================================================================
// persistence
// ==============================================================================
void Trainer::Load() {
    std::string ini = IniPath();
    for (auto& r : rows) {
        char b[64];
        GetPrivateProfileStringA("values", r.id.c_str(), "", b, sizeof b, ini.c_str());
        if (b[0]) r.val = std::max(r.vmin, std::min(r.vmax, (float)atof(b)));
        r.hotkey = GetPrivateProfileIntA("hotkeys", r.id.c_str(), r.hotkey, ini.c_str());
        if (r.kind == K_TOGGLE && (r.id == "pausemenu" || r.id.rfind("esp", 0) == 0 || r.id.rfind("ally", 0) == 0))
            r.on = GetPrivateProfileIntA("lastOn", r.id.c_str(), r.on ? 1 : 0, ini.c_str()) != 0;
    }
    char b[64];
    GetPrivateProfileStringA("menu", "accent", "", b, sizeof b, ini.c_str());
    if (b[0]) sscanf(b, "%f,%f,%f", &accent[0], &accent[1], &accent[2]);
    testerMode = GetPrivateProfileIntA("menu", "tester", 0, ini.c_str()) != 0;
    LoadSets(); LoadMacros();
}
void Trainer::Save() { dirty = true; }
extern unsigned g_espColors[7];
void Trainer::SaveNow() {
    // one file write instead of ~600 WritePrivateProfileString calls (each rewrites the whole file -> lag spike)
    dirty = false; lastSave = NowSec();
    std::string o;
    char b[160];
    o += "[meta]\r\n"; snprintf(b, sizeof b, "crashlistBuild=%d\r\n", TRAINER_BUILD); o += b;
    o += "[menu]\r\n"; snprintf(b, sizeof b, "accent=%.3f,%.3f,%.3f\r\ntester=%d\r\n", accent[0], accent[1], accent[2], testerMode ? 1 : 0); o += b;
    o += "[espcolors]\r\n"; for (int i = 0; i < 7; i++) { snprintf(b, sizeof b, "c%d=%u\r\n", i, g_espColors[i]); o += b; }
    o += "[values]\r\n";
    for (auto& r : rows) { snprintf(b, sizeof b, r.isFloat ? "%s=%.2f\r\n" : "%s=%.0f\r\n", r.id.c_str(), r.val); o += b; }
    o += "[hotkeys]\r\n";
    for (auto& r : rows) { snprintf(b, sizeof b, "%s=%d\r\n", r.id.c_str(), r.hotkey); o += b; }
    o += "[lastOn]\r\n";
    for (auto& r : rows) if (r.kind == K_TOGGLE || r.kind == K_SLIDER || r.kind == K_SET) { snprintf(b, sizeof b, "%s=%d\r\n", r.id.c_str(), r.on ? 1 : 0); o += b; }
    std::string ini = IniPath(), tmp = ini + ".tmp";
    if (FILE* f = fopen(tmp.c_str(), "wb")) {
        fwrite(o.data(), 1, o.size(), f); fclose(f);
        MoveFileExA(tmp.c_str(), ini.c_str(), MOVEFILE_REPLACE_EXISTING);
    }
}
void Trainer::RestoreToggles() {
    std::string ini = IniPath();
    int c = 0;
    for (auto& r : rows) {
        if (r.kind != K_TOGGLE && r.kind != K_SLIDER && r.kind != K_SET) continue;
        if (GetPrivateProfileIntA("lastOn", r.id.c_str(), 0, ini.c_str()) && !r.on) {
            r.on = true; c++;
            if (r.act && r.kind != K_SET) { Row* rp = &r; Queue("row:" + r.id, [rp] { if (InGame()) rp->act(*rp); }); }
        }
    }
    Toast("Przywrócono " + std::to_string(c) + " przełączników");
}
static std::vector<std::vector<std::string>> ReadTsv(const char* file) {
    std::vector<std::vector<std::string>> out;
    std::ifstream f(DataPath(file));
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::vector<std::string> cols; std::stringstream ss(line); std::string c;
        while (std::getline(ss, c, '\t')) cols.push_back(c);
        out.push_back(cols);
    }
    return out;
}
void Trainer::LoadPositions(const std::string& world) {
    std::vector<SavedPos> v;
    for (auto& c : ReadTsv("G1CrazyTrainer_pozycje.txt"))
        if (c.size() >= 5 && c[0] == world) v.push_back({ c[1], (float)atof(c[2].c_str()), (float)atof(c[3].c_str()), (float)atof(c[4].c_str()) });
    std::lock_guard<std::mutex> l(mtx);
    positions = v;
}
void Trainer::SavePositions(const std::string& world) {
    std::vector<std::string> keep;
    { std::ifstream f(DataPath("G1CrazyTrainer_pozycje.txt")); std::string line;
      while (std::getline(f, line)) if (!line.empty() && line.rfind(world + "\t", 0) != 0) keep.push_back(line); }
    std::ofstream o(DataPath("G1CrazyTrainer_pozycje.txt"));
    for (auto& k : keep) o << k << "\n";
    std::lock_guard<std::mutex> l(mtx);
    for (auto& p : positions) o << world << "\t" << p.name << "\t" << p.x << "\t" << p.y << "\t" << p.z << "\n";
}
void Trainer::LoadSets() {
    std::vector<ItemSet> keepPresets;
    for (auto& s : sets) if (s.preset) keepPresets.push_back(s);
    sets = keepPresets;
    for (auto& c : ReadTsv("G1CrazyTrainer_zestawy.txt")) {
        if (c.size() < 2) continue;
        ItemSet s; s.name = c[0];
        std::stringstream ss(c[1]); std::string it;
        while (std::getline(ss, it, ',')) {
            auto k = it.find(':');
            if (k != std::string::npos) s.items.push_back({ it.substr(0, k), atoi(it.substr(k + 1).c_str()) });
        }
        sets.push_back(s);
    }
}
void Trainer::SaveSets() {
    std::ofstream o(DataPath("G1CrazyTrainer_zestawy.txt"));
    for (auto& s : sets) {
        if (s.preset) continue;
        o << s.name << "\t";
        for (size_t i = 0; i < s.items.size(); i++) o << (i ? "," : "") << s.items[i].first << ":" << s.items[i].second;
        o << "\n";
    }
}
void Trainer::LoadMacros() {
    macros.clear();
    for (auto& c : ReadTsv("G1CrazyTrainer_makra.txt")) {
        if (c.size() < 3) continue;
        Macro m; m.name = c[0]; m.repeat = std::max(1, atoi(c[1].c_str())); m.cmds = c[2];
        for (auto& ch : m.cmds) if (ch == '\x1f') ch = '\n';
        macros.push_back(m);
    }
}
void Trainer::SaveMacros() {
    std::ofstream o(DataPath("G1CrazyTrainer_makra.txt"));
    for (auto m : macros) {
        for (auto& ch : m.cmds) if (ch == '\n') ch = '\x1f'; else if (ch == '\t') ch = ' ';
        o << m.name << "\t" << m.repeat << "\t" << m.cmds << "\n";
    }
}
