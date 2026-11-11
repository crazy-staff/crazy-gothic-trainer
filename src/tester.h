// Tester toolkit: everything a tester needs to report problems without talking to the developer.
//  - tickets: append-only log (nothing is overwritten - every verdict and report stays in history)
//  - events:  automatic per-option usage log (applied, toggled, crashed) attached to tickets
//  - known:   known issues shipped with the release (G1CrazyTrainer_znane.txt, generated from GitHub)
//  - export:  one report file per session/package + GitHub issue links prefilled from the form
#pragma once
#include <string>
#include <vector>

namespace tst {

enum TicketType { TT_OK, TT_PARTIAL, TT_BUG, TT_CRASH, TT_IDEA, TT_QUESTION, TT_COUNT };
extern const char* kTypeNames[TT_COUNT];      // Działa, Częściowo, Błąd, Crash, Propozycja, Pytanie / inne
extern const char* kSeverityNames[3];          // drobne, przeszkadza, blokuje grę
extern const char* kReproNames[4];             // zawsze, często, czasem, raz

struct Ticket {
    std::string uid;            // NICK-yymmdd-hhmmss(-n)
    std::string target;         // option id ("rain"), menu issue ("UI-07") or "" = general
    std::string targetName;     // human name at the time of writing
    int type = TT_BUG, severity = 1, repro = 0;
    std::string title, steps, expected, actual, notes;
    std::string context;        // filled automatically: world, position, active options, recent events
    int build = 0; std::string time, nick;
    bool sent = false;          // included in an exported report or opened on GitHub
    bool verdict = false;       // quick checklist verdict (Działa / Częściowo / Nie działa)
};

struct Known {                  // one known issue (from GitHub) for an option
    std::string target, state, title; int issue = 0; int build = 0;
};

struct EventRec { double t; std::string stamp, target, text; };

// profile
std::string& Nick();
void SetNick(const std::string& n);

// tickets
void NoteCrash(const std::string& key);          // called by the crash guard before Load
void Load();                                     // once; also migrates the old G1CrazyTrainer_testy.txt
const std::vector<Ticket>& Tickets();
Ticket& Add(Ticket t);                           // fills uid/time/build/nick, appends to the file
void Save();                                     // rewrite (after editing/sent flags)
void Remove(const std::string& uid);
int LastVerdict(const std::string& target);      // -1 none, else TT_OK/TT_PARTIAL/TT_BUG (this build first)
int CountFor(const std::string& target, bool openOnly);
std::vector<const Ticket*> For(const std::string& target);

// known issues + developer history
const std::vector<Known>& KnownIssues();
std::vector<const Known*> KnownFor(const std::string& target);

// automatic event log
void Event(const std::string& target, const std::string& text);
std::vector<EventRec> EventsFor(const std::string& target, int max);

// context snapshot for a new ticket (game state, options switched on, recent events)
std::string BuildContext(const std::string& target);

// export
std::string ExportReport(bool onlyUnsent, std::string* outPath);   // writes Raporty\..., returns text
std::string ShortText(const Ticket& t);           // compact text for Discord / clipboard
std::string GithubUrl(const Ticket& t);           // issue form, prefilled
void OpenUrl(const std::string& url);
void OpenFolder(const std::string& path);
bool CopyToClipboard(const std::string& utf8);
std::string SystemInfo();

}
