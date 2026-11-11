// Tester toolkit (see tester.h). Files next to the DLL (Gothic\system\Autorun):
//   G1CrazyTrainer_zgloszenia.txt  - every ticket and verdict ever written (append-only history)
//   G1CrazyTrainer_tester.txt      - tester nick
//   G1CrazyTrainer_znane.txt       - known issues shipped with the release (from GitHub)
//   G1CrazyTrainer_raporty\        - exported report packages (one file = everything to send)
#include "tester.h"
#include "trainer.h"
#include <windows.h>
#include <shellapi.h>
#include <psapi.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <deque>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>

namespace tst {

const char* kTypeNames[TT_COUNT] = { "Działa", "Częściowo", "Błąd", "Crash", "Propozycja", "Pytanie / inne" };
const char* kSeverityNames[3] = { "drobne", "przeszkadza", "blokuje grę" };
const char* kReproNames[4] = { "zawsze", "często", "czasem", "raz" };

namespace {
std::vector<Ticket> g_t;
std::vector<Known> g_known;
std::vector<EventRec> g_ev;           // ring (kept <= 3000)
std::mutex g_evMx;
bool g_loaded = false;
std::string g_nick;
std::string g_crashKey;            // set by NoteCrash before Load

const char* kRepo = "dawidseipold/gothic1-crazy-trainer";

std::string Stamp(bool compact) {
    SYSTEMTIME s; GetLocalTime(&s);
    char b[32];
    if (compact) snprintf(b, sizeof b, "%02d%02d%02d-%02d%02d%02d", s.wYear % 100, s.wMonth, s.wDay, s.wHour, s.wMinute, s.wSecond);
    else snprintf(b, sizeof b, "%04d-%02d-%02d %02d:%02d:%02d", s.wYear, s.wMonth, s.wDay, s.wHour, s.wMinute, s.wSecond);
    return b;
}
std::string Esc(const std::string& s) {
    std::string o;
    for (char c : s) { if (c == '\\') o += "\\\\"; else if (c == '\n') o += "\\n"; else if (c == '\r') continue; else o += c; }
    return o;
}
std::string Unesc(const std::string& s) {
    std::string o;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '\\' && i + 1 < s.size()) { char n = s[++i]; o += n == 'n' ? '\n' : n; }
        else o += s[i];
    }
    return o;
}
void WriteBlock(std::string& o, const Ticket& t) {
    auto kv = [&](const char* k, const std::string& v) { o += k; o += '='; o += Esc(v); o += '\n'; };
    o += "@@TICKET\n";
    kv("uid", t.uid); kv("target", t.target); kv("targetName", t.targetName);
    kv("type", kTypeNames[t.type]); kv("severity", kSeverityNames[t.severity]); kv("repro", kReproNames[t.repro]);
    kv("title", t.title); kv("steps", t.steps); kv("expected", t.expected); kv("actual", t.actual); kv("notes", t.notes);
    kv("build", std::to_string(t.build)); kv("time", t.time); kv("nick", t.nick);
    kv("verdict", t.verdict ? "1" : "0"); kv("sent", t.sent ? "1" : "0");
    kv("context", t.context);
    o += "@@END\n";
}
int IndexOf(const char* const* names, int n, const std::string& v, int def) {
    for (int i = 0; i < n; i++) if (v == names[i]) return i;
    return def;
}
std::vector<Ticket> ParseBlocks(std::istream& in) {
    std::vector<Ticket> out; Ticket cur; bool open = false; std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line == "@@TICKET") { cur = Ticket{}; open = true; continue; }
        if (line == "@@END") { if (open) out.push_back(cur); open = false; continue; }
        if (!open) continue;
        size_t e = line.find('='); if (e == std::string::npos) continue;
        std::string k = line.substr(0, e), v = Unesc(line.substr(e + 1));
        if (k == "uid") cur.uid = v; else if (k == "target") cur.target = v; else if (k == "targetName") cur.targetName = v;
        else if (k == "type") cur.type = IndexOf(kTypeNames, TT_COUNT, v, TT_BUG);
        else if (k == "severity") cur.severity = IndexOf(kSeverityNames, 3, v, 1);
        else if (k == "repro") cur.repro = IndexOf(kReproNames, 4, v, 0);
        else if (k == "title") cur.title = v; else if (k == "steps") cur.steps = v; else if (k == "expected") cur.expected = v;
        else if (k == "actual") cur.actual = v; else if (k == "notes") cur.notes = v; else if (k == "build") cur.build = atoi(v.c_str());
        else if (k == "time") cur.time = v; else if (k == "nick") cur.nick = v; else if (k == "verdict") cur.verdict = v == "1";
        else if (k == "sent") cur.sent = v == "1"; else if (k == "context") cur.context = v;
    }
    return out;
}
std::string FilePath() { return DataPath("G1CrazyTrainer_zgloszenia.txt"); }

// build <= 6 kept one status + note per option in G1CrazyTrainer_testy.txt -> turn it into history entries once
void MigrateOld() {
    std::string p = DataPath("G1CrazyTrainer_testy.txt");
    std::ifstream f(p);
    if (!f) return;
    int build = 0; std::string line;
    std::vector<Ticket> add;
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.rfind("# build", 0) == 0) { build = atoi(line.c_str() + 7); continue; }
        size_t a = line.find('\t'); if (a == std::string::npos) continue;
        size_t b = line.find('\t', a + 1);
        int st = atoi(line.substr(a + 1, b == std::string::npos ? std::string::npos : b - a - 1).c_str());
        std::string note = b == std::string::npos ? "" : line.substr(b + 1);
        if (!st && note.empty()) continue;
        Ticket t; t.target = line.substr(0, a);
        if (Row* r = T.Get(t.target)) t.targetName = r->name;
        t.type = st == 1 ? TT_OK : st == 2 ? TT_PARTIAL : st == 3 ? TT_BUG : TT_QUESTION;
        t.verdict = st != 0; t.actual = note; t.title = note.empty() ? kTypeNames[t.type] : note.substr(0, 80);
        t.build = build ? build : TRAINER_BUILD - 1; t.time = "(z checklisty build " + std::to_string(t.build) + ")";
        t.nick = g_nick.empty() ? "?" : g_nick; t.sent = true;   // the developer already read these
        t.uid = t.nick + "-old-" + t.target;
        add.push_back(t);
    }
    f.close();
    for (auto& t : add) g_t.push_back(t);
    Save();
    MoveFileExA(p.c_str(), DataPath("G1CrazyTrainer_testy_stare.txt").c_str(), MOVEFILE_REPLACE_EXISTING);
}
void LoadKnown() {
    g_known.clear();
    std::ifstream f(DataPath("G1CrazyTrainer_znane.txt"));
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> c; std::stringstream ss(line); std::string x;
        while (std::getline(ss, x, '\t')) c.push_back(x);
        if (c.size() < 5) continue;
        Known k; k.target = c[0]; k.state = c[1]; k.issue = atoi(c[2].c_str()); k.build = atoi(c[3].c_str()); k.title = c[4];
        g_known.push_back(k);
    }
}
std::string UrlEnc(const std::string& s) {
    std::string o; char b[4];
    for (unsigned char c : s) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') o += (char)c;
        else { snprintf(b, sizeof b, "%%%02X", c); o += b; }
    }
    return o;
}
std::string Clip(const std::string& s, size_t n) { return s.size() <= n ? s : s.substr(0, n) + "\n[...ucięte - pełne dane są w raporcie]"; }
std::string ReadTail(const std::string& path, int lines, std::string* bangs) {
    FILE* f = fopen(path.c_str(), "rb"); if (!f) return "(brak pliku)";
    std::deque<std::string> q; char buf[2048];
    while (fgets(buf, sizeof buf, f)) {
        std::string l(buf);
        if (bangs && (l.rfind("!!", 0) == 0 || l.find("!! ") != std::string::npos || l.find("padla") != std::string::npos)) *bangs += l;
        q.push_back(l); if ((int)q.size() > lines) q.pop_front();
    }
    fclose(f);
    std::string o; for (auto& l : q) o += l;
    return o;
}
std::string ReadAll(const std::string& path) {
    std::ifstream f(path, std::ios::binary); if (!f) return "(brak pliku)";
    std::stringstream ss; ss << f.rdbuf(); return ss.str();
}
}

// ----------------------------------------------------------------- profile
std::string& Nick() { return g_nick; }
void SetNick(const std::string& n) {
    g_nick.clear();
    for (char c : n) if (isalnum((unsigned char)c) || c == '_' || c == '-' || (unsigned char)c >= 0x80) g_nick += c;
    if (g_nick.size() > 24) g_nick.resize(24);
    if (FILE* f = fopen(DataPath("G1CrazyTrainer_tester.txt").c_str(), "wb")) { fputs(g_nick.c_str(), f); fclose(f); }
}

// ----------------------------------------------------------------- tickets
void NoteCrash(const std::string& key) { g_crashKey = key; }
void Load() {
    if (g_loaded) return;
    g_loaded = true;
    { std::ifstream nf(DataPath("G1CrazyTrainer_tester.txt")); std::getline(nf, g_nick); }
    { std::ifstream f(FilePath()); g_t = ParseBlocks(f); }
    MigrateOld();
    LoadKnown();
    if (!g_crashKey.empty()) {   // the previous session died during an action: prepare a crash report draft
        Ticket t; t.type = TT_CRASH; t.severity = 2; t.repro = 3;
        std::string k = g_crashKey;
        if (k.rfind("row:", 0) == 0) { size_t e = k.find(':', 4); t.target = k.substr(4, e == std::string::npos ? std::string::npos : e - 4); }
        if (Row* r = T.Get(t.target)) t.targetName = r->name;
        t.title = "Gra się wyłączyła podczas: " + (t.targetName.empty() ? k : t.targetName);
        t.notes = "Wykryte automatycznie przy starcie gry (akcja w toku: " + k + "). Dopisz, co robiłeś.";
        Add(t);
        g_crashKey.clear();
    }
}
const std::vector<Ticket>& Tickets() { return g_t; }
Ticket& Add(Ticket t) {
    std::string base = (g_nick.empty() ? std::string("anon") : g_nick) + "-" + Stamp(true);
    t.uid = base; int n = 2;
    while (std::any_of(g_t.begin(), g_t.end(), [&](const Ticket& x) { return x.uid == t.uid; })) t.uid = base + "-" + std::to_string(n++);
    t.time = Stamp(false); t.build = TRAINER_BUILD; t.nick = g_nick.empty() ? "anon" : g_nick;
    g_t.push_back(t);
    std::string o; WriteBlock(o, t);
    if (FILE* f = fopen(FilePath().c_str(), "ab")) { fwrite(o.data(), 1, o.size(), f); fclose(f); }
    Event(t.target, std::string("zgłoszenie: ") + kTypeNames[t.type] + (t.title.empty() ? "" : " - " + t.title));
    return g_t.back();
}
void Save() {
    std::string o = "# Gothic 1 Crazy Trainer - historia zgłoszeń testera (nie edytuj ręcznie)\n";
    for (auto& t : g_t) WriteBlock(o, t);
    std::string p = FilePath(), tmp = p + ".tmp";
    if (FILE* f = fopen(tmp.c_str(), "wb")) { fwrite(o.data(), 1, o.size(), f); fclose(f); MoveFileExA(tmp.c_str(), p.c_str(), MOVEFILE_REPLACE_EXISTING); }
}
void Remove(const std::string& uid) {
    g_t.erase(std::remove_if(g_t.begin(), g_t.end(), [&](const Ticket& t) { return t.uid == uid; }), g_t.end());
    Save();
}
int LastVerdict(const std::string& target) {
    int best = -1, bestBuild = -1;
    for (auto& t : g_t) if (t.target == target && (t.type <= TT_BUG || t.type == TT_CRASH) && t.build >= bestBuild) {
        bestBuild = t.build; best = t.type == TT_CRASH ? TT_BUG : t.type;
    }
    return bestBuild == TRAINER_BUILD ? best : -1;   // verdicts from older builds do not count for this one
}
int CountFor(const std::string& target, bool openOnly) {
    int n = 0;
    for (auto& t : g_t) if (t.target == target && !(openOnly && (t.type == TT_OK || t.sent))) n++;
    return n;
}
std::vector<const Ticket*> For(const std::string& target) {
    std::vector<const Ticket*> v;
    for (auto& t : g_t) if (t.target == target) v.push_back(&t);
    std::reverse(v.begin(), v.end());
    return v;
}
const std::vector<Known>& KnownIssues() { return g_known; }
std::vector<const Known*> KnownFor(const std::string& target) {
    std::vector<const Known*> v;
    for (auto& k : g_known) if (k.target == target) v.push_back(&k);
    return v;
}

// ----------------------------------------------------------------- events
void Event(const std::string& target, const std::string& text) {
    std::unique_lock<std::mutex> l(g_evMx, std::try_to_lock);   // try: also called from the crash handler
    if (!l.owns_lock()) return;
    g_ev.push_back({ NowSec(), Stamp(false), target, text });
    if (g_ev.size() > 3000) g_ev.erase(g_ev.begin(), g_ev.begin() + 500);
}
std::vector<EventRec> EventsFor(const std::string& target, int max) {
    std::lock_guard<std::mutex> l(g_evMx);
    std::vector<EventRec> v;
    for (auto it = g_ev.rbegin(); it != g_ev.rend() && (int)v.size() < max; ++it) if (target.empty() || it->target == target) v.push_back(*it);
    std::reverse(v.begin(), v.end());
    return v;
}

// ----------------------------------------------------------------- context / export
std::string SystemInfo() {
    std::string o; char b[512];
    typedef LONG(WINAPI * RtlGetVersionFn)(OSVERSIONINFOW*);
    OSVERSIONINFOW vi{}; vi.dwOSVersionInfoSize = sizeof vi;
    if (auto f = (RtlGetVersionFn)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlGetVersion")) f(&vi);
    snprintf(b, sizeof b, "Windows %lu.%lu build %lu\n", vi.dwMajorVersion, vi.dwMinorVersion, vi.dwBuildNumber); o += b;
    HKEY k; char cpu[128] = "?"; DWORD sz = sizeof cpu;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &k) == ERROR_SUCCESS) {
        RegQueryValueExA(k, "ProcessorNameString", nullptr, nullptr, (BYTE*)cpu, &sz); RegCloseKey(k);
    }
    o += std::string("CPU: ") + cpu + "\n";
    DISPLAY_DEVICEA dd{}; dd.cb = sizeof dd;
    for (DWORD i = 0; EnumDisplayDevicesA(nullptr, i, &dd, 0); i++) if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) { o += std::string("GPU: ") + dd.DeviceString + "\n"; break; }
    MEMORYSTATUSEX ms{}; ms.dwLength = sizeof ms; GlobalMemoryStatusEx(&ms);
    snprintf(b, sizeof b, "RAM: %.1f GB\nEkran: %dx%d\n", ms.ullTotalPhys / 1073741824.0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)); o += b;
    char exe[MAX_PATH]; GetModuleFileNameA(nullptr, exe, MAX_PATH); o += std::string("Gra: ") + exe + "\n";
    // modules loaded from the game folder (Union plugins, renderer, other Autorun DLLs) - most conflicts come from these
    std::string gameDir = exe; gameDir = gameDir.substr(0, gameDir.find_last_of("\\/") + 1);
    std::string gd = gameDir; for (auto& c : gd) c = (char)tolower((unsigned char)c);
    std::string root = gd.size() > 7 ? gd.substr(0, gd.size() - 7) : gd;   // ...\Gothic\ (exe lives in system\)
    HMODULE mods[512]; DWORD need = 0;
    if (K32EnumProcessModules(GetCurrentProcess(), mods, sizeof mods, &need)) {
        o += "Moduły z folderu gry:";
        for (DWORD i = 0; i < need / sizeof(HMODULE) && i < 512; i++) {
            char p[MAX_PATH]; GetModuleFileNameA(mods[i], p, MAX_PATH);
            std::string lp = p; for (auto& c : lp) c = (char)tolower((unsigned char)c);
            if (lp.rfind(root, 0) == 0) { const char* n = strrchr(p, '\\'); o += std::string(" ") + (n ? n + 1 : p); }
        }
        o += "\n";
    }
    return o;
}
std::string BuildContext(const std::string& target) {
    std::string o; char b[256];
    const Snapshot& s = T.snap;
    snprintf(b, sizeof b, "Build %d  ·  świat: %s  ·  pozycja %.0f %.0f %.0f  ·  dzień %d %02d:%02d\n", TRAINER_BUILD,
             s.world.empty() ? "-" : s.world.c_str(), s.px, s.py, s.pz, s.day, s.hour, s.minute); o += b;
    snprintf(b, sizeof b, "Postać: %s  HP %d/%d  poziom %d%s\n", s.bodyName.c_str(), s.hp, s.hpmax, s.level, s.swapped ? "  (przejęte ciało)" : ""); o += b;
    if (Row* r = T.Get(target)) {
        snprintf(b, sizeof b, "Opcja: %s (%s › %s)  stan: %s  wartość: %g\n", r->name.c_str(), r->cat.c_str(), r->sub.c_str(), r->on ? "włączona" : "wyłączona", r->val); o += b;
    }
    std::string on;
    for (auto& r : T.rows) if (r.on && (r.kind == K_TOGGLE || r.kind == K_SLIDER || r.kind == K_SET)) { if (!on.empty()) on += ", "; on += r.id; }
    o += "Włączone: " + (on.empty() ? std::string("nic") : on) + "\n";
    auto ev = EventsFor(target, 12);
    if (!target.empty() && !ev.empty()) { o += "Ostatnie użycia tej opcji:\n"; for (auto& e : ev) o += "  " + e.stamp.substr(11) + "  " + e.text + "\n"; }
    auto all = EventsFor("", 10);
    if (!all.empty()) { o += "Ostatnie zdarzenia:\n"; for (auto& e : all) o += "  " + e.stamp.substr(11) + "  [" + e.target + "] " + e.text + "\n"; }
    return o;
}
std::string ShortText(const Ticket& t) {
    std::string o = "**[" + std::string(kTypeNames[t.type]) + "] " + (t.title.empty() ? t.targetName : t.title) + "**  (build " + std::to_string(t.build) + ", " + t.nick + ", " + t.uid + ")\n";
    if (!t.target.empty()) o += "Opcja: " + t.targetName + " `" + t.target + "`\n";
    if (t.type == TT_BUG || t.type == TT_CRASH || t.type == TT_PARTIAL)
        o += std::string("Waga: ") + kSeverityNames[t.severity] + "  ·  Powtarzalność: " + kReproNames[t.repro] + "\n";
    if (!t.steps.empty()) o += "Kroki:\n" + t.steps + "\n";
    if (!t.expected.empty()) o += "Oczekiwane: " + t.expected + "\n";
    if (!t.actual.empty()) o += "Faktycznie: " + t.actual + "\n";
    if (!t.notes.empty()) o += "Uwagi: " + t.notes + "\n";
    return o;
}
std::string GithubUrl(const Ticket& t) {
    // GitHub issue forms accept ?<field id>=value to prefill fields (ids are in .github/ISSUE_TEMPLATE/*.yml)
    const char* tpl = t.type == TT_CRASH ? "crash.yml" : t.type == TT_IDEA ? "propozycja.yml" : "blad.yml";
    std::string title = (t.type == TT_IDEA ? "[Propozycja] " : t.type == TT_CRASH ? "[Crash] " : "") +
                        (t.title.empty() ? t.targetName : t.title);
    std::string u = std::string("https://github.com/") + kRepo + "/issues/new?template=" + tpl + "&title=" + UrlEnc(title);
    auto f = [&](const char* id, const std::string& v, size_t mx) { if (!v.empty()) u += std::string("&") + id + "=" + UrlEnc(Clip(v, mx)); };
    f("opcja", t.target.empty() ? std::string("ogólne") : t.targetName + " (" + t.target + ")", 200);
    f("build", std::to_string(t.build), 10);
    f("tester", t.nick, 40);
    f("uid", t.uid, 60);
    if (t.type == TT_IDEA) { f("opis", t.steps.empty() ? t.notes : t.steps + (t.notes.empty() ? "" : "\n\n" + t.notes), 2500); f("dlaczego", t.expected, 1200); }
    else {
        f("kroki", t.steps, 1500); f("oczekiwane", t.expected, 600); f("faktyczne", t.actual + (t.notes.empty() ? "" : "\n\n" + t.notes), 1500);
        f("waga", kSeverityNames[t.severity], 30); f("powtarzalnosc", kReproNames[t.repro], 30);
    }
    f("kontekst", t.context, 1800);
    return u;
}
std::string ExportReport(bool onlyUnsent, std::string* outPath) {
    std::string o;
    o += "@@G1CT-REPORT v1\n";
    o += "build=" + std::to_string(TRAINER_BUILD) + "\ntester=" + (g_nick.empty() ? std::string("anon") : g_nick) + "\ndate=" + Stamp(false) + "\n";
    o += "@@SYSTEM\n" + SystemInfo();
    int n = 0;
    std::string blocks;
    for (auto& t : g_t) if (!onlyUnsent || !t.sent) { WriteBlock(blocks, t); n++; }
    o += "@@TICKETS " + std::to_string(n) + "\n" + blocks;
    o += "@@VERDICTS (ostatnia ocena każdej opcji w tym buildzie)\n";
    for (auto& r : T.rows) { int v = LastVerdict(r.id); if (v >= 0) o += r.id + "\t" + kTypeNames[v] + "\n"; }
    o += "@@EVENTS\n";
    for (auto& e : EventsFor("", 600)) o += e.stamp + "\t" + e.target + "\t" + e.text + "\n";
    std::string bangs;
    std::string tail = ReadTail(DataPath("G1CrazyTrainer.log"), 400, &bangs);
    o += "@@ERRORS (wyjątki i crashe z całego logu)\n" + bangs;
    o += "@@CRASHES (G1CrazyTrainer_crashes.txt)\n" + ReadAll(DataPath("G1CrazyTrainer_crashes.txt")) + "\n";
    o += "@@LOG (ostatnie 400 linii)\n" + tail;
    o += "@@SETTINGS\n" + ReadAll(IniPath()) + "\n@@END-REPORT\n";
    std::string dir = DataPath("G1CrazyTrainer_raporty");
    CreateDirectoryA(dir.c_str(), nullptr);
    std::string path = dir + "\\raport_" + (g_nick.empty() ? std::string("anon") : g_nick) + "_" + Stamp(true) + ".txt";
    if (FILE* f = fopen(path.c_str(), "wb")) { fwrite(o.data(), 1, o.size(), f); fclose(f); }
    if (outPath) *outPath = path;
    bool changed = false;
    for (auto& t : g_t) if (!t.sent) { t.sent = true; changed = true; }
    if (changed) Save();
    Event("", "wyeksportowano raport (" + std::to_string(n) + " zgłoszeń)");
    return o;
}
void OpenUrl(const std::string& url) { ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL); }
void OpenFolder(const std::string& path) {
    std::string args = "/select,\"" + path + "\"";
    ShellExecuteA(nullptr, "open", "explorer.exe", args.c_str(), nullptr, SW_SHOWNORMAL);
}
bool CopyToClipboard(const std::string& utf8) {
    int wn = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (wn <= 0 || !OpenClipboard(nullptr)) return false;
    EmptyClipboard();
    HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, wn * sizeof(wchar_t));
    if (h) {
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, (wchar_t*)GlobalLock(h), wn);
        GlobalUnlock(h);
        SetClipboardData(CF_UNICODETEXT, h);
    }
    CloseClipboard();
    return h != nullptr;
}

}
