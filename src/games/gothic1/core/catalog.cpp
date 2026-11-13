#include "catalog.h"
#include "g1.h"
#include <algorithm>
#include <cstring>
#include <set>

using namespace g1;

Catalog g_cat;
const char* kItemCatNames[IC_COUNT] = { "Broń biała", "Łuki/kusze", "Amunicja", "Zbroje", "Runy", "Zwoje",
                                        "Mikstury", "Jedzenie", "Amulety/pierścienie", "Klucze", "Pisma", "Inne" };

std::string Cp1250ToUtf8(const std::string& s) {
    if (s.empty()) return s;
    int wn = MultiByteToWideChar(1250, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(wn, 0);
    MultiByteToWideChar(1250, 0, s.data(), (int)s.size(), &w[0], wn);
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), wn, nullptr, 0, nullptr, nullptr);
    std::string o(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.data(), wn, &o[0], n, nullptr, nullptr);
    return o;
}

const char* kNpcGroupNames[NG_COUNT] = { "Stary Obóz", "Nowy Obóz", "Bractwo", "Magowie", "Inni ludzie", "Zwierzęta",
                                         "Bestie", "Nieumarli", "Orkowie", "Demony i golemy" };

std::string GuildName(int g) {
    static const char* n[] = { "Brak", "Magnat", "Strażnik", "Cień", "Mag Ognia", "Kopacz", "Mag Wody", "Najemnik",
        "Szkodnik", "Chłop", "Kret", "Guru", "Nowicjusz", "Templariusz", "Nekromanta", "Kobieta", "-",
        "Jaszczur", "Ognisty jaszczur", "Goblin", "Troll", "Zębacz", "Pełzacz", "Chrząszcz", "Ścierwojad", "Demon",
        "Wilk", "Cieniostwór", "Krwiopijca", "Błotny wąż", "Zombie", "Ork-nieumarły", "Szkielet", "Pies orków",
        "Kretoszczur", "Golem", "Topielec", "-", "Ork-szaman", "Ork-wojownik", "Ork-zwiadowca", "Ork-niewolnik" };
    if (g >= 0 && g < (int)(sizeof(n) / sizeof(*n))) return n[g];
    return "Gildia " + std::to_string(g);
}

namespace {
struct Op { int op; int a; int b; };

struct Assign { int field; int idx; int val; int strSym; };  // strSym >= 0 -> string constant

// Reads a Daedalus function body, following CALLs (prototype/default functions), and
// records constant assignments to class fields: e.g. name = "Gomez", damage[2] = 40.
void ScanBody(int pc, const std::set<int>& fields, std::vector<Assign>& out, int depth, std::set<int>& seen) {
    if (depth > 3 || pc < 0 || seen.count(pc)) return;
    seen.insert(pc);
    const uint8_t* code = Bytecode();
    if (!code) return;
    Op prev[2] = { {-1, 0, 0}, {-1, 0, 0} };
    int p = pc;
    for (int n = 0; n < 8000; n++) {
        int op = code[p];
        Op cur{ op, 0, 0 };
        switch (op) {
        case 61: case 62: case 64: case 65: case 67: case 75: case 76: case 80:
            memcpy(&cur.a, code + p + 1, 4); p += 5; break;
        case 245: memcpy(&cur.a, code + p + 1, 4); cur.b = code[p + 5]; p += 6; break;
        default: p += 1; break;
        }
        if (op == 61) ScanBody(cur.a, fields, out, depth + 1, seen);
        if ((op == 9 || op == 70) && (prev[1].op == 65 || prev[1].op == 245) && fields.count(prev[1].a)) {
            int idx = prev[1].op == 245 ? prev[1].b : 0;
            const Op& src = prev[0];
            if (src.op == 64) out.push_back({ prev[1].a, idx, src.a, -1 });
            else if (src.op == 65) {
                Sym* s = SymAt(src.a);
                if (s && s->type() == T_INT && s->ele() <= 1) out.push_back({ prev[1].a, idx, s->content, -1 });
                else if (s && s->type() == T_STRING) out.push_back({ prev[1].a, idx, 0, src.a });
            }
        }
        prev[0] = prev[1]; prev[1] = cur;
        if (op == 60) break;
    }
}

std::string StrOfSym(int sym) {
    Sym* s = SymAt(sym);
    if (!s || !s->content) return "";
    return zs(*reinterpret_cast<zSTRING*>(s->content));
}

bool Has(const std::string& id, const char* k) { return id.find(k) != std::string::npos; }
int G(const char* n, int d) { return ConstInt(n, d); }

std::string ItemSub(const ItemEntry& e) {
    const std::string& id = e.id;
    int f = e.flags;
    switch (e.cat) {
    case IC_MELEE:
        if (f & G("ITEM_2HD_AXE", 1 << 17)) return "Topory 2H";
        if (f & G("ITEM_2HD_SWD", 1 << 16)) return "Miecze 2H";
        if (f & G("ITEM_AXE", 1 << 15)) return "Topory/maczugi 1H";
        if (f & G("ITEM_SWD", 1 << 14)) return "Miecze 1H";
        return Has(id, "2H") ? "Miecze 2H" : "Inne 1H";
    case IC_RANGED: return (f & G("ITEM_CROSSBOW", 1 << 20)) || Has(id, "CROSSBOW") ? "Kusze" : "Łuki";
    case IC_AMMO: return Has(id, "BOLT") ? "Bełty" : "Strzały";
    case IC_ARMOR:
        if (Has(id, "GRD") || Has(id, "STT") || Has(id, "EBR") || Has(id, "VLK")) return "Stary Obóz";
        if (Has(id, "SLD") || Has(id, "ORG") || Has(id, "SFB") || Has(id, "BAU")) return "Nowy Obóz";
        if (Has(id, "NOV") || Has(id, "TPL") || Has(id, "GUR")) return "Bractwo";
        if (Has(id, "KDF") || Has(id, "KDW") || Has(id, "DMB") || Has(id, "MAGE") || Has(id, "XARDAS")) return "Magowie";
        return "Inne";
    case IC_RUNE: case IC_SCROLL:
        if (Has(id, "FIRE") || Has(id, "PYRO")) return "Ogień";
        if (Has(id, "ICE")) return "Lód";
        if (Has(id, "THUNDER") || Has(id, "LIGHTNING") || Has(id, "CHAIN")) return "Błyskawice";
        if (Has(id, "SUMMON") || Has(id, "ARMY")) return "Przyzwania";
        if (Has(id, "TRF") || Has(id, "TRANSFORM")) return "Przemiany";
        if (Has(id, "HEAL") || Has(id, "LIGHT") || Has(id, "TELEPORT")) return "Użytkowe";
        return "Inne";
    case IC_POTION:
        if (Has(id, "HEALTH") || Has(id, "HEAL")) return Has(id, "ELIXIER") ? "Trwałe" : "Lecznicze";
        if (Has(id, "MANA")) return Has(id, "ELIXIER") ? "Trwałe" : "Mana";
        if (Has(id, "ELIXIER") || Has(id, "MASTER") || Has(id, "STRENGTH") || Has(id, "DEX")) return "Trwałe";
        return "Inne";
    case IC_FOOD:
        if (Has(id, "PLANT") || Has(id, "BERR") || Has(id, "MUSHROOM") || Has(id, "HERB")) return "Rośliny i grzyby";
        if (Has(id, "BEER") || Has(id, "WINE") || Has(id, "BOOZE") || Has(id, "RICE")) return "Alkohole";
        if (Has(id, "MEAT") || Has(id, "MUTTON") || Has(id, "HAM") || Has(id, "SAUSAGE") || Has(id, "FISH")) return "Mięso";
        return "Inne";
    case IC_MAGIC: return (f & G("ITEM_RING", 1 << 11)) || Has(id, "RING") ? "Pierścienie" : "Amulety";
    case IC_DOCS:
        if (Has(id, "MAP")) return "Mapy";
        if (Has(id, "BOOK") || Has(id, "BUCH")) return "Księgi";
        return "Listy i inne";
    case IC_MISC:
        if (Has(id, "NUGGET") || Has(id, "ORE")) return "Ruda";
        if (Has(id, "JOINT") || Has(id, "SWAMPHERB")) return "Bagienne ziele";
        if (id.rfind("ITAT", 0) == 0) return "Trofea";
        if (Has(id, "STUFF") || Has(id, "LOCKPICK") || Has(id, "HAMMER") || Has(id, "PAN") || Has(id, "TORCH")) return "Narzędzia";
        if (f & G("ITEM_MISSION", 1 << 12)) return "Questowe";
        return "Różne";
    }
    return "";
}

int NpcGroupOf(int g) {
    auto is = [&](const char* n) { return g == ConstInt(n, -99); };
    if (is("GIL_EBR") || is("GIL_GRD") || is("GIL_STT") || is("GIL_VLK")) return NG_OLDCAMP;
    if (is("GIL_SLD") || is("GIL_ORG") || is("GIL_BAU") || is("GIL_SFB")) return NG_NEWCAMP;
    if (is("GIL_GUR") || is("GIL_NOV") || is("GIL_TPL")) return NG_SWAMP;
    if (is("GIL_KDF") || is("GIL_KDW") || is("GIL_DMB")) return NG_MAGES;
    if (g <= ConstInt("GIL_SEPERATOR_HUM", 16)) return NG_OTHERHUM;
    if (is("GIL_ZOMBIE") || is("GIL_UNDEADORC") || is("GIL_SKELETON")) return NG_UNDEAD;
    if (g > ConstInt("GIL_SEPERATOR_ORC", 37) || is("GIL_ORCDOG")) return NG_ORCS;
    if (is("GIL_DEMON") || is("GIL_GOLEM")) return NG_MAGIC;
    if (is("GIL_TROLL") || is("GIL_SHADOWBEAST") || is("GIL_SNAPPER") || is("GIL_LURKER") || is("GIL_SWAMPSHARK") ||
        is("GIL_MINECRAWLER")) return NG_BEASTS;
    return NG_ANIMALS;
}

const char* AtrName(int a) {
    switch (a) { case 0: return "HP"; case 1: return "max HP"; case 2: return "Mana"; case 3: return "max mana";
                 case 4: return "Siła"; case 5: return "Zręczność"; default: return "atr"; }
}
std::string ItemInfo(const ItemEntry& e) {
    char b[256]; std::string s;
    if (e.cat == IC_MELEE || e.cat == IC_RANGED || e.cat == IC_AMMO) {
        if (e.damage) { snprintf(b, sizeof b, "Obr. %d", e.damage); s += b; }
        if (e.range && e.cat == IC_MELEE) { snprintf(b, sizeof b, "  ·  zasięg %d", e.range); s += b; }
    } else if (e.cat == IC_ARMOR) {
        // protection indices: 1 blunt 2 edge 3 fire 4 fly 5 magic 6 point
        snprintf(b, sizeof b, "Broń %d · Strzały %d · Ogień %d · Magia %d", std::max(e.prot[1], e.prot[2]), e.prot[6], e.prot[3], e.prot[5]);
        s += b;
    }
    for (int i = 0; i < 3; i++)
        if (e.condVal[i] > 0) { snprintf(b, sizeof b, "%swym. %s %d", s.empty() ? "" : "  ·  ", AtrName(e.condAtr[i]), e.condVal[i]); s += b; }
    if (s.empty())
        for (int i = 0; i < 6; i++)
            if (!e.text[i].empty() && e.count[i]) {
                std::string t = e.text[i]; while (!t.empty() && (t.back() == ' ' || t.back() == ':')) t.pop_back();
                snprintf(b, sizeof b, "%s%s: %d", s.empty() ? "" : "  ·  ", t.c_str(), e.count[i]); s += b;
            }
    return s;
}

Sym* RootClass(Sym* s) {
    int g = 0;
    while (s && s->parent && g++ < 16) s = s->parent;
    return s;
}
}

int NpcGroupOfGuild(int g) { return NpcGroupOf(g); }

void Catalog::Build() {
    items.clear(); npcs.clear();
    Sym* clsItem = SymByName("C_ITEM"); Sym* clsNpc = SymByName("C_NPC");
    auto F = [](const char* n) { return SymIndex(n); };
    int fName = F("C_ITEM.NAME"), fMain = F("C_ITEM.MAINFLAG"), fVal = F("C_ITEM.VALUE"), fFlags = F("C_ITEM.FLAGS"),
        fDmgT = F("C_ITEM.DAMAGETOTAL"), fDmg = F("C_ITEM.DAMAGE"), fRange = F("C_ITEM.RANGE"), fProt = F("C_ITEM.PROTECTION"),
        fCondA = F("C_ITEM.COND_ATR"), fCondV = F("C_ITEM.COND_VALUE"), fText = F("C_ITEM.TEXT"), fCount = F("C_ITEM.COUNT"),
        fDesc = F("C_ITEM.DESCRIPTION");
    int nName = F("C_NPC.NAME"), nGuild = F("C_NPC.GUILD"), nLevel = F("C_NPC.LEVEL");
    std::set<int> itemFields{ fName, fMain, fVal, fFlags, fDmgT, fDmg, fRange, fProt, fCondA, fCondV, fText, fCount, fDesc };
    std::set<int> npcFields{ nName, nGuild, nLevel };
    int sepHum = ConstInt("GIL_SEPERATOR_HUM", 16);
    const int KAT_NF = ConstInt("ITEM_KAT_NF", 2), KAT_FF = ConstInt("ITEM_KAT_FF", 4),
              KAT_MUN = ConstInt("ITEM_KAT_MUN", 8), KAT_ARMOR = ConstInt("ITEM_KAT_ARMOR", 16),
              KAT_FOOD = ConstInt("ITEM_KAT_FOOD", 32), KAT_DOCS = ConstInt("ITEM_KAT_DOCS", 64),
              KAT_POT = ConstInt("ITEM_KAT_POTIONS", 128), KAT_RUNE = ConstInt("ITEM_KAT_RUNE", 512),
              KAT_MAGIC = ConstInt("ITEM_KAT_MAGIC", (int)0x80000000);
    int n = SymCount();
    for (int i = 0; i < n; i++) {
        Sym* s = SymAt(i);
        if (!s || s->type() != T_INST || !s->name.ptr || s->name.ptr[0] == '\xFF') continue;
        if (s->content <= 0) continue;
        std::string id = zs(s->name);
        if (id.find('.') != std::string::npos) continue;
        Sym* root = RootClass(s);
        if (root != clsItem && root != clsNpc) continue;
        std::vector<Assign> out; std::set<int> seen;
        const std::set<int>& fields = root == clsItem ? itemFields : npcFields;
        if (s->parent && s->parent->type() == T_PROTO) ScanBody(s->parent->content, fields, out, 0, seen);
        ScanBody(s->content, fields, out, 0, seen);
        if (root == clsItem) {
            ItemEntry e; e.sym = i; e.id = id; e.mainflag = 0; e.value = 0; e.cat = IC_MISC;
            std::string desc;
            for (auto& a : out) {
                std::string str = a.strSym >= 0 ? Cp1250ToUtf8(StrOfSym(a.strSym)) : std::string();
                if (a.field == fName && a.strSym >= 0) e.name = str;
                else if (a.field == fDesc && a.strSym >= 0) desc = str;
                else if (a.field == fMain) e.mainflag = a.val;
                else if (a.field == fVal) e.value = a.val;
                else if (a.field == fFlags) e.flags = a.val;
                else if (a.field == fDmgT) e.damage = a.val;
                else if (a.field == fDmg && a.strSym < 0) { e.damage = std::max(e.damage, 0) + 0; e.damageArr[a.idx & 7] = a.val; }
                else if (a.field == fRange) e.range = a.val;
                else if (a.field == fProt && a.strSym < 0) e.prot[a.idx & 7] = a.val;
                else if (a.field == fCondA) e.condAtr[a.idx % 3] = a.val;
                else if (a.field == fCondV) e.condVal[a.idx % 3] = a.val;
                else if (a.field == fText && a.strSym >= 0) e.text[a.idx % 6] = str;
                else if (a.field == fCount) e.count[a.idx % 6] = a.val;
            }
            if (!e.damage) for (int k = 0; k < 8; k++) e.damage += e.damageArr[k];
            if (e.name.empty() || e.name == "Runa" || e.name == "Zwój" || e.name == "Napój" || e.name == "Mikstura") {
                if (!desc.empty() && desc != e.name) e.name = e.name.empty() ? desc : e.name + ": " + desc;
            }
            std::string up = id;
            if (e.mainflag == KAT_NF) e.cat = IC_MELEE;
            else if (e.mainflag == KAT_FF) e.cat = IC_RANGED;
            else if (e.mainflag == KAT_MUN) e.cat = IC_AMMO;
            else if (e.mainflag == KAT_ARMOR) e.cat = IC_ARMOR;
            else if (e.mainflag == KAT_RUNE) e.cat = up.rfind("ITARSCROLL", 0) == 0 ? IC_SCROLL : IC_RUNE;
            else if (e.mainflag == KAT_POT) e.cat = IC_POTION;
            else if (e.mainflag == KAT_FOOD) e.cat = IC_FOOD;
            else if (e.mainflag == KAT_MAGIC) e.cat = IC_MAGIC;
            else if (e.mainflag == KAT_DOCS) e.cat = IC_DOCS;
            else if (up.rfind("ITKE", 0) == 0) e.cat = IC_KEY;
            if (e.name.empty()) e.name = id;
            e.sub = ItemSub(e);
            e.info = ItemInfo(e);
            items.push_back(e);
        } else {
            NpcEntry e{ i, id, "", 0, 0, false };
            for (auto& a : out) {
                if (a.field == nName && a.strSym >= 0 && a.idx == 0) e.name = Cp1250ToUtf8(StrOfSym(a.strSym));
                else if (a.field == nGuild) e.guild = a.val;
                else if (a.field == nLevel) e.level = a.val;
            }
            if (id == "PC_HERO") continue;
            e.monster = e.guild > sepHum;
            e.group = NpcGroupOf(e.guild);
            if (e.name.empty()) e.name = id;
            npcs.push_back(e);
        }
    }
    auto byName = [](auto& a, auto& b) { return a.name < b.name; };
    std::sort(items.begin(), items.end(), byName);
    std::sort(npcs.begin(), npcs.end(), byName);
    built = true;
    BuildWaypoints();
}

void Catalog::BuildWaypoints() {
    wps.clear();
    void* w = GameWorld();
    builtForWorld = w;
    if (!w) return;
    void* wn = fld<void*>(w, W_WAYNET);
    if (!wn) return;
    // zCListSort embedded at +0x28: {compare, data, next}
    void* node = fld<void*>(wn, WN_WPLIST + 8);
    int guard = 0;
    while (node && guard++ < 100000) {
        void* wp = fld<void*>(node, 4);
        if (wp) {
            const float* p = &fld<float>(wp, WP_POS);
            wps.push_back({ zs(fld<zSTRING>(wp, WP_NAME)), p[0], p[1], p[2] });
        }
        node = fld<void*>(node, 8);
    }
    std::sort(wps.begin(), wps.end(), [](auto& a, auto& b) { return a.name < b.name; });
}
