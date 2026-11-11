// Trainer core: queue, crash guard, self test, persistence, frame dispatch to modules.
#include "trainer.h"
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
void gx::Guard(const std::string& key, const std::function<void()>& f) { Pending(key); Log("> %s", key.c_str()); f(); PendingDone(); }

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
    return EXCEPTION_CONTINUE_SEARCH;
}
void CrashGuardInit() {
    std::string pend = DataPath("G1CrazyTrainer_pending.txt");
    std::string key;
    { std::ifstream pf(pend); if (pf) std::getline(pf, key); }
    if (!key.empty()) {
        Log("Poprzednia sesja padla podczas: %s", key.c_str());
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
void InstallGameHooks() {
    MH_CreateHook((LPVOID)F_OCGAME_RENDER, (LPVOID)&hkRender, (LPVOID*)&oRender);
    Player_Hooks(); Items_Hooks(); Npcs_Hooks(); Target_Hooks(); Chaos_Hooks(); World_Hooks(); Esp_Hooks(); Teleport_Hooks(); Marvin_Hooks(); Photo_Hooks();
    MH_EnableHook(MH_ALL_HOOKS);
}

void Trainer::Tick() {
    std::vector<std::pair<std::string, std::function<void()>>> q;
    { std::lock_guard<std::mutex> l(mtx); q.swap(queue); }
    bool inGame = InGame();
    if (inGame) K.Refresh();
    for (auto& f : q) Guard(f.first, f.second);

    // input ownership: menu owns everything, noclip owns the keyboard
    // re-asserted 4x per second only: doing it every frame (DirectInput unacquire) caused stutter
    static bool lastK = true, lastM = true; static double lastInput = 0;
    bool wantK = !menuOpen && !(On("noclip") && inGame), wantM = !menuOpen;
    if (wantK != lastK || wantM != lastM || ((!wantK || !wantM) && NowSec() - lastInput > 0.25)) {
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
    Esp_Register(); Teleport_Register(); Marvin_Register();
    for (auto& r : rows) if (Crashed("row:" + r.id)) { r.status = ST_CRASH; r.hist = "crashowało w poprzedniej sesji (wykryte automatycznie)"; }
}
void ApplyRow(Row& r) {
    Row* rp = &r;
    switch (r.kind) {
    case K_TOGGLE: case K_SLIDER:
        r.on = !r.on;
        if (r.act) T.Queue("row:" + r.id, [rp] { if (InGame()) rp->act(*rp); });
        T.Toast(r.name + ": " + (r.on ? "ON" : "OFF"));
        break;
    case K_SET:
        r.on = !r.on;
        T.Toast(r.on ? r.name + " = " + std::to_string((int)r.val) : r.name + ": przywrócono wartość gry");
        break;
    case K_ACTION: case K_CHOICE:
        if (r.act) T.Queue("row:" + r.id, [rp] { if (InGame()) rp->act(*rp); });
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
void Trainer::SaveNow() {
    dirty = false; lastSave = NowSec();
    std::string ini = IniPath();
    for (auto& r : rows) {
        char b[64];
        snprintf(b, sizeof b, r.isFloat ? "%.2f" : "%.0f", r.val);
        WritePrivateProfileStringA("values", r.id.c_str(), b, ini.c_str());
        snprintf(b, sizeof b, "%d", r.hotkey);
        WritePrivateProfileStringA("hotkeys", r.id.c_str(), b, ini.c_str());
        if (r.kind == K_TOGGLE || r.kind == K_SLIDER || r.kind == K_SET)
            WritePrivateProfileStringA("lastOn", r.id.c_str(), r.on ? "1" : "0", ini.c_str());
    }
    char b[64]; snprintf(b, sizeof b, "%.3f,%.3f,%.3f", accent[0], accent[1], accent[2]);
    WritePrivateProfileStringA("menu", "accent", b, ini.c_str());
    WritePrivateProfileStringA("menu", "tester", testerMode ? "1" : "0", ini.c_str());
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
