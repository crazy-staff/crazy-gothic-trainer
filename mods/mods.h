#pragma once
// Feature modules. Each module owns its rows, per-frame logic and hooks, so a change in
// one module cannot silently alter another. Shared game helpers live in core/common.
#include "../trainer.h"
#include "../core/common.h"
#include "../core/catalog.h"

struct TickCtx { void* hero; void* game; float dt; bool paused; Snapshot& s; };

#define DECL_MODULE(N) void N##_Register(); void N##_Tick(TickCtx&); void N##_Hooks(); void N##_World();
DECL_MODULE(Player)
DECL_MODULE(Items)
DECL_MODULE(Npcs)
DECL_MODULE(Target)
DECL_MODULE(Chaos)
DECL_MODULE(World)
DECL_MODULE(Esp)
DECL_MODULE(Teleport)
DECL_MODULE(Marvin)

// helpers for building rows
Row MkRow(const char* id, const char* name, const char* note, const char* cat, const char* sub, Kind k, float v = 0,
          float mn = 0, float mx = 0, std::function<void(Row&)> act = nullptr, bool flt = false);
void St(Status s, const char* hist);       // status of the last added row
void NoVal();
void Opts(std::vector<std::pair<int, std::string>> o);
void Icon(const char* utf8);
void Hidden();
std::function<void(Row&)> NeedT(std::function<void(void*)> f);

// Fork Awesome glyphs (utf8)
#define ICN_BOLT "\xef\x83\xa7"
#define ICN_USER "\xef\x80\x87"
#define ICN_USERS "\xef\x83\x80"
#define ICN_EXCHANGE "\xef\x83\xac"
#define ICN_MAGIC "\xef\x83\x90"
#define ICN_FIRE "\xef\x81\xad"
#define ICN_SNOW "\xef\x8b\x9c"
#define ICN_TRASH "\xef\x87\xb8"
#define ICN_HEART "\xef\x80\x84"
#define ICN_ROCKET "\xef\x84\xb5"
#define ICN_SIGNIN "\xef\x82\x90"
#define ICN_MAGNET "\xef\x81\xb6"
#define ICN_BAG "\xef\x8a\x90"
#define ICN_KILL "\xef\x88\xb5"
#define ICN_REFRESH "\xef\x80\xa1"
#define ICN_ARROWS "\xef\x82\xb2"
#define ICN_EXPAND "\xef\x81\xa5"
#define ICN_BOOKMARK "\xef\x80\xae"
#define ICN_TIMES "\xef\x80\x8d"
#define ICN_SEARCH "\xef\x80\x82"
#define ICN_COG "\xef\x80\x93"
#define ICN_EYE "\xef\x81\xae"
#define ICN_HOME "\xef\x80\x95"
#define ICN_PLAY "\xef\x81\x8b"
#define ICN_PLUS "\xef\x81\xa7"
#define ICN_MAP "\xef\x81\x81"
#define ICN_SHIELD "\xef\x84\xb2"
#define ICN_CUBE "\xef\x86\xb2"
#define ICN_CROSSHAIR "\xef\x81\x9b"
#define ICN_GHOST "\xef\x81\xb0"
#define ICN_CLOCK "\xef\x80\x97"
#define ICN_TERMINAL "\xef\x84\xa0"
#define ICN_PAW "\xef\x86\xb0"
#define ICN_STAR "\xef\x80\x85"
#define ICN_FLASK "\xef\x83\x83"
