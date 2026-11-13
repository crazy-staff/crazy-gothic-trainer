// Menu (Dear ImGui over GD3D11), build 5.
// Layout: sidebar (sections -> subsections, Konfiguracja at the bottom), content = cards in two
// columns. Rounding scale: window 16, card 12, row/panel item 8, button/input 6, small 4.
#include "ui.h"
#include "trainer.h"
#include "portrait.h"
#include "gfx.h"
#include "core/catalog.h"
#include "core/common.h"
#include "mods/mods.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "tester.h"
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <climits>

extern unsigned g_espColors[7];

#define ICO_CHEVR "\xef\x81\x94"
#define ICO_CHEVD "\xef\x81\xb8"

static inline void PF(ImFont* f) { ImGui::PushFont(f, f->LegacySize); }
namespace {
const ImVec4 C_WIN(0.075f, 0.075f, 0.10f, 0.97f), C_SIDE(0.055f, 0.055f, 0.075f, 1), C_CARD(0.105f, 0.105f, 0.14f, 1),
    C_CARD2(0.13f, 0.13f, 0.17f, 1), C_HOVER(0.16f, 0.155f, 0.22f, 1), C_TEXT(0.92f, 0.92f, 0.96f, 1), C_DIM(0.55f, 0.55f, 0.63f, 1),
    C_FAINT(0.36f, 0.36f, 0.43f, 1), C_ON(0.45f, 0.92f, 0.55f, 1), C_TRACK(0.22f, 0.22f, 0.29f, 1), C_BTN(0.15f, 0.15f, 0.20f, 1),
    C_BTNH(0.21f, 0.20f, 0.29f, 1), C_LINE(0.17f, 0.17f, 0.22f, 1), C_HP(0.92f, 0.36f, 0.40f, 1), C_MANA(0.40f, 0.62f, 1.0f, 1),
    C_ST_OK(0.45f, 0.92f, 0.55f, 1), C_ST_BUG(0.98f, 0.80f, 0.30f, 1), C_ST_CRASH(1.0f, 0.40f, 0.40f, 1), C_ST_NEW(0.42f, 0.70f, 1.0f, 1),
    C_G_MOVE(0.42f, 0.70f, 1.0f, 1), C_G_ATK(1.0f, 0.45f, 0.45f, 1), C_G_CTRL(0.75f, 0.55f, 1.0f, 1), C_G_OTHER(0.55f, 0.85f, 0.65f, 1);
ImVec4 C_ACC(0.66f, 0.55f, 1.00f, 1), C_ACCD(0.25f, 0.20f, 0.45f, 1);
inline ImU32 U32(const ImVec4& c, float a = -1) { return ImGui::GetColorU32(ImVec4(c.x, c.y, c.z, a < 0 ? c.w : a)); }
inline ImVec4 Mix(const ImVec4& a, const ImVec4& b, float t) { return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, 1); }

ImFont *fReg, *fBold, *fBig, *fTitle, *fSmall, *fIcon;
float S = 1.f;
bool iconsOk = false;
// rounding scale (nested elements get the smaller radius)
float g_round = 1.f, g_baseS = 1.f;   // build 16: menu customisation (Konfiguracja › Wygląd menu)
float R_WIN() { return 16 * S * g_round; } float R_CARD() { return 12 * S * g_round; } float R_ITEM() { return 8 * S * g_round; } float R_BTN() { return 6 * S * g_round; } float R_SM() { return 4 * S * g_round; }
const float PAD = 6;   // inner padding of a card (rows are inset by this on every side)

struct Sec { const char* name; const char* icon; std::vector<const char*> subs; };
const std::vector<Sec> kSecs = {
    { "Postać", ICN_USER, { "Informacje", "Wygląd", "Cheaty", "Ruch", "Walka" } },
    { "Ekwipunek", ICN_BAG, { "Spawner", "Zestawy", "Zbieranie" } },
    { "Postacie", ICN_USERS, { "Cel", "Drużyna", "Otoczenie", "Spawner" } },
    { "Fun", ICN_FIRE, { "Walka", "Magia", "Chaos" } },
    { "Świat", ICN_MAP, { "Czas i fabuła", "Teleport" } },
    { "Wizualia", ICN_EYE, { "ESP postaci", "ESP przedmiotów", "Widżety", "Celownik", "Grafika", "Kolory" } },
    { "Narzędzia", ICN_TERMINAL, { "Konsola", "Kompendium", "Diagnostyka" } },
};
int curSec = 0, curSub = 0; bool cfgPage = false;
// build 15: every page keeps what was typed in its search box (Konfiguracja: "Pamiętaj wyszukiwanie")
std::map<int, std::string> g_searchMem;
void SwapSearch(int newSec, int newSub);
Row* editRow = nullptr; char editBuf[32]; bool editFirst = false;
void SwapSearch(int newSec, int newSub) {
    extern char search[96];
    g_searchMem[curSec * 100 + curSub] = search;
    search[0] = 0;
    if (!gx::On("keepsearch")) { g_searchMem.clear(); return; }
    auto it = g_searchMem.find(newSec * 100 + newSub);
    if (it != g_searchMem.end()) snprintf(search, sizeof search, "%s", it->second.c_str());
}
Row* bindRow = nullptr; bool bindKeys[256];
char search[96] = "", gsearch[96] = "";
int itemCat = -1, npcGroup = -1; std::string itemSub;
char consoleBuf[2048] = "", scriptBuf[96] = "", posName[64] = "", macroName[64] = "", setName[64] = "", allySearch[64] = "", nameBuf[64] = "";
int macroRepeat = 1, allyFilter = 0, editPos = -1; SavedPos editPosV; char editPosName[64];
float manX = 0, manY = 0, manZ = 0;
std::set<uintptr_t> allyPick;
std::map<std::string, std::pair<double, int>> photoAsked;
bool testsPage = false;   // kept false: tests live in their own window since build 7
bool testsWin = false;
int lookSig = 0; double lookAt = 0; bool lookDirty = false;

std::string KeyOnly(int vk) {
    UINT sc = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
    switch (vk) {
    case VK_INSERT: case VK_DELETE: case VK_HOME: case VK_END: case VK_PRIOR: case VK_NEXT:
    case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN: sc |= 0x100; break;
    }
    char b[64];
    if (vk == VK_XBUTTON1) return "Mysz 4";
    if (vk == VK_XBUTTON2) return "Mysz 5";
    if (vk == VK_MBUTTON) return "Środkowy";
    if (GetKeyNameTextA((LONG)(sc << 16), b, sizeof b) > 0) return b;
    snprintf(b, sizeof b, "VK %d", vk);
    return b;
}
std::string KeyName(int hk) {
    if (!hk) return "";
    std::string s;
    if (hk & HK_CTRL) s += "Ctrl+";
    if (hk & HK_ALT) s += "Alt+";
    if (hk & HK_SHIFT) s += "Shift+";
    return s + KeyOnly(hk & 0xFFFF);
}
float TextW(const char* t, ImFont* f) { PF(f); float w = ImGui::CalcTextSize(t).x; ImGui::PopFont(); return w; }
float TextH(ImFont* f) { return f->LegacySize; }
ImDrawList* DL() { return ImGui::GetWindowDrawList(); }
void Text(ImVec2 p, ImFont* f, const ImVec4& c, const char* t) { PF(f); DL()->AddText(p, U32(c), t); ImGui::PopFont(); }
void TextClip(ImVec2 p, ImFont* f, const ImVec4& c, const std::string& t, float maxX) {
    DL()->PushClipRect(ImVec2(p.x, p.y - 2), ImVec2(maxX, p.y + 40 * S), true);
    Text(p, f, c, t.c_str());
    DL()->PopClipRect();
}
// text vertically centred on cy
void TextV(float x, float cy, ImFont* f, const ImVec4& c, const char* t) { PF(f); ImVec2 ts = ImGui::CalcTextSize(t); DL()->AddText(ImVec2(x, cy - ts.y * 0.5f), U32(c), t); ImGui::PopFont(); }
// icon glyph centred exactly on its visible bounds (not the advance width)
void IconAt(ImDrawList* dl, ImVec2 c, const char* glyph, ImU32 col, float size = 0) {
    if (!iconsOk || !glyph || !*glyph) return;
    if (size <= 0) size = fIcon->LegacySize;
    unsigned int cp = 0; ImTextCharFromUtf8(&cp, glyph, nullptr);
    ImFontBaked* bk = fIcon->GetFontBaked(size);
    const ImFontGlyph* g = bk ? bk->FindGlyph((ImWchar)cp) : nullptr;
    if (!g) { dl->AddText(fIcon, size, ImVec2(c.x - size * 0.5f, c.y - size * 0.5f), col, glyph); return; }
    float gx = (g->X0 + g->X1) * 0.5f, gy = (g->Y0 + g->Y1) * 0.5f;
    dl->AddText(fIcon, size, ImVec2(std::floor(c.x - gx + 0.5f), std::floor(c.y - gy + 0.5f)), col, glyph);
}
std::string Lower(std::string s) { for (auto& c : s) c = (char)tolower((unsigned char)c); return s; }
bool MatchQ(const char* q, const std::string& a, const std::string& b) {
    if (!q[0]) return true;
    std::string ql = Lower(q);
    return Lower(a).find(ql) != std::string::npos || Lower(b).find(ql) != std::string::npos;
}

// ------------------------------------------------------------------ tooltips
void PushTipStyle() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12 * S, 9 * S));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, R_ITEM());
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6 * S, 4 * S));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(C_ACC.x, C_ACC.y, C_ACC.z, 0.15f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.09f, 0.09f, 0.12f, 0.98f));
}
void PopTipStyle() { ImGui::PopStyleColor(2); ImGui::PopStyleVar(4); }
void Tip(const char* fmt, ...) {
    char b[1024]; va_list a; va_start(a, fmt); vsnprintf(b, sizeof b, fmt, a); va_end(a);
    PushTipStyle();
    if (ImGui::BeginTooltip()) { ImGui::PushTextWrapPos(380 * S); ImGui::TextUnformatted(b); ImGui::PopTextWrapPos(); ImGui::EndTooltip(); }
    PopTipStyle();
}
// title + key/value lines
void TipKV(const std::string& title, const std::vector<std::pair<std::string, std::string>>& kv, const ImVec4* titleCol = nullptr) {
    PushTipStyle();
    if (ImGui::BeginTooltip()) {
        PF(fBold); ImGui::TextColored(titleCol ? *titleCol : C_TEXT, "%s", title.c_str()); ImGui::PopFont();
        ImGui::Dummy(ImVec2(0, 2 * S));
        float kw = 0; for (auto& p : kv) kw = std::max(kw, ImGui::CalcTextSize(p.first.c_str()).x);
        for (auto& p : kv) {
            ImGui::TextColored(C_DIM, "%s", p.first.c_str()); ImGui::SameLine(kw + 26 * S);
            ImGui::TextColored(C_TEXT, "%s", p.second.c_str());
        }
        ImGui::EndTooltip();
    }
    PopTipStyle();
}

// ------------------------------------------------------------------ small widgets
float ButtonW(const char* label, ImFont* f = nullptr) {
    PF(f ? f : fBold); float w = ImGui::CalcTextSize(label, nullptr, true).x; ImGui::PopFont(); return w + 22 * S;
}
float BtnH() { return 28 * S; }
bool Button(const char* id, const char* label, ImVec2 pos, bool primary = true, ImFont* f = nullptr, float minW = 0, const ImVec4* tint = nullptr) {
    if (!f) f = fBold;
    PF(f); ImVec2 ts = ImGui::CalcTextSize(label, nullptr, true); ImGui::PopFont();
    ImVec2 sz(std::max(minW, ts.x + 22 * S), BtnH());
    ImGui::SetCursorScreenPos(pos);
    bool c = ImGui::InvisibleButton(id, sz);
    bool hov = ImGui::IsItemHovered(), act = ImGui::IsItemActive();
    ImVec4 base = tint ? *tint : C_ACC;
    ImVec4 bg = primary ? (act ? Mix(base, C_WIN, 0.25f) : hov ? Mix(base, ImVec4(1, 1, 1, 1), 0.12f) : base) : (hov ? C_BTNH : C_BTN);
    DL()->AddRectFilled(pos, ImVec2(pos.x + sz.x, pos.y + sz.y), U32(bg), R_BTN());
    PF(f);
    DL()->AddText(nullptr, 0, ImVec2(std::floor(pos.x + (sz.x - ts.x) * 0.5f), std::floor(pos.y + (sz.y - ts.y) * 0.5f)),
                  primary ? IM_COL32(18, 16, 28, 255) : U32(C_TEXT), label, ImGui::FindRenderedTextEnd(label));
    ImGui::PopFont();
    return c;
}
bool IconButton(const char* id, const char* glyph, const char* tip, ImVec2 pos, float size, const ImVec4& col, bool active = false, const ImVec4* tint = nullptr) {
    ImGui::SetCursorScreenPos(pos);
    bool c = ImGui::InvisibleButton(id, ImVec2(size, size));
    bool hov = ImGui::IsItemHovered();
    ImVec4 bg = tint ? (hov ? Mix(C_CARD2, *tint, 0.35f) : Mix(C_CARD2, *tint, 0.14f)) : (active ? C_ACCD : (hov ? C_BTNH : C_BTN));
    DL()->AddRectFilled(pos, ImVec2(pos.x + size, pos.y + size), U32(bg), R_BTN());
    IconAt(DL(), ImVec2(pos.x + size * 0.5f, pos.y + size * 0.5f), glyph, U32(tint ? *tint : (hov || active ? C_ACC : col)));
    if (hov && tip) Tip("%s", tip);
    return c;
}
bool Switch(const char* id, bool v, ImVec2 pos) {
    float w = 36 * S, h = 20 * S;
    ImGui::SetCursorScreenPos(pos);
    bool c = ImGui::InvisibleButton(id, ImVec2(w, h));
    bool hov = ImGui::IsItemHovered();
    DL()->AddRectFilled(pos, ImVec2(pos.x + w, pos.y + h), U32(v ? C_ACC : (hov ? Mix(C_TRACK, C_TEXT, 0.12f) : C_TRACK)), h * 0.5f);
    float kx = v ? pos.x + w - h * 0.5f : pos.x + h * 0.5f;
    DL()->AddCircleFilled(ImVec2(kx, pos.y + h * 0.5f), h * 0.36f, v ? IM_COL32(255, 255, 255, 255) : U32(C_DIM), 16);
    return c;
}
bool Check(const char* id, bool v, ImVec2 pos, float sz = 0) {
    if (sz <= 0) sz = 18 * S;
    ImGui::SetCursorScreenPos(pos);
    bool c = ImGui::InvisibleButton(id, ImVec2(sz, sz));
    bool hov = ImGui::IsItemHovered();
    DL()->AddRectFilled(pos, ImVec2(pos.x + sz, pos.y + sz), U32(v ? C_ACC : (hov ? C_BTNH : C_TRACK)), R_SM());
    if (v) {
        ImVec2 a(pos.x + sz * 0.24f, pos.y + sz * 0.52f), b(pos.x + sz * 0.43f, pos.y + sz * 0.70f), d(pos.x + sz * 0.77f, pos.y + sz * 0.30f);
        DL()->AddLine(a, b, IM_COL32(20, 18, 30, 255), 2.f * S); DL()->AddLine(b, d, IM_COL32(20, 18, 30, 255), 2.f * S);
    }
    return c;
}
void Pill(ImVec2 p, const char* t, const ImVec4& c, float* w = nullptr) {
    PF(fSmall); ImVec2 ts = ImGui::CalcTextSize(t);
    float h = ts.y + 6 * S;
    DL()->AddRectFilled(p, ImVec2(p.x + ts.x + 14 * S, p.y + h), U32(c, 0.16f), h * 0.5f);
    DL()->AddText(ImVec2(p.x + 7 * S, p.y + 3 * S), U32(c), t); ImGui::PopFont();
    if (w) *w = ts.x + 14 * S;
}
bool Chip(const char* label, bool active, const char* icon = nullptr) {
    PF(active ? fBold : fReg);
    ImVec2 ts = ImGui::CalcTextSize(label, nullptr, true);
    float iw = icon && iconsOk ? 20 * S : 0;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImVec2 sz(ts.x + 22 * S + iw, 30 * S);
    float right = ImGui::GetWindowPos().x + ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x;
    if (p.x + sz.x > right) { ImGui::NewLine(); p = ImGui::GetCursorScreenPos(); }
    bool c = ImGui::InvisibleButton(label, sz);
    bool hov = ImGui::IsItemHovered();
    DL()->AddRectFilled(p, ImVec2(p.x + sz.x, p.y + sz.y), U32(active ? C_ACCD : (hov ? C_BTNH : C_BTN)), sz.y * 0.5f);
    ImVec4 col = active ? C_ACC : (hov ? C_TEXT : C_DIM);
    if (iw > 0) IconAt(DL(), ImVec2(p.x + 11 * S + 8 * S, p.y + sz.y * 0.5f), icon, U32(col), 13 * S);
    DL()->AddText(nullptr, 0, ImVec2(p.x + 11 * S + iw, std::floor(p.y + (sz.y - ts.y) * 0.5f)), U32(col), label, ImGui::FindRenderedTextEnd(label));
    ImGui::PopFont();
    ImGui::SameLine(0, 6 * S);
    return c;
}
// "x" inside the right end of the last text field: clears it (shown only when there is text)
bool ClearX(char* buf) {
    if (!buf[0]) return false;
    ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    float r = 9 * S;
    ImVec2 c(b.x - r - 6 * S, (a.y + b.y) * 0.5f);
    // manual hit test: the text field was submitted first and would steal the hover from a button
    bool hov = ImGui::IsMouseHoveringRect(ImVec2(c.x - r, c.y - r), ImVec2(c.x + r, c.y + r));
    bool hit = hov && ImGui::IsMouseClicked(0);
    if (hov) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    DL()->AddCircleFilled(c, r, U32(hov ? C_ACC : C_CARD), 16);
    IconAt(DL(), c, ICN_TIMES, U32(hov ? ImVec4(1, 1, 1, 1) : C_DIM), 10 * S);
    if (hit) { buf[0] = 0; ImGui::ClearActiveID(); return true; }
    return false;
}
void InputBox(const char* id, char* buf, size_t n, const char* hint, float w) {
    ImGui::SetNextItemWidth(w);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, C_CARD2);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10 * S, 7 * S));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, R_BTN());
    ImGui::InputTextWithHint(id, hint, buf, n);
    ImGui::PopStyleVar(2); ImGui::PopStyleColor();
    ClearX(buf);
}
void NumField(const char* id, int* v, int mn, int mx, float w) {
    ImGui::SetNextItemWidth(w);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, C_CARD2); ImGui::PushStyleColor(ImGuiCol_Text, C_ACC);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8 * S, 7 * S));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, R_BTN());
    ImGui::InputInt(id, v, 0, 0);
    ImGui::PopStyleVar(2); ImGui::PopStyleColor(2);
    *v = std::max(mn, std::min(mx, *v));
}
void StatusDot(ImVec2 c, const Row& r) {
    ImVec4 col; const char* t;
    switch (r.status) { case ST_OK: col = C_ST_OK; t = "OK - sprawdzone"; break; case ST_BUG: col = C_ST_BUG; t = "BUG - działa źle"; break;
                        case ST_CRASH: col = C_ST_CRASH; t = "CRASH - crashowało"; break; default: col = C_ST_NEW; t = "TEST - nowe / do sprawdzenia"; }
    DL()->AddCircleFilled(c, 4 * S, U32(col), 12);
    if (ImGui::IsMouseHoveringRect(ImVec2(c.x - 7 * S, c.y - 7 * S), ImVec2(c.x + 7 * S, c.y + 7 * S))) {
        PushTipStyle();
        if (ImGui::BeginTooltip()) {
            PF(fBold); ImGui::TextColored(col, "%s", t); ImGui::PopFont();
            if (!r.hist.empty()) { ImGui::PushTextWrapPos(360 * S); ImGui::TextColored(C_DIM, "%s", r.hist.c_str()); ImGui::PopTextWrapPos(); }
            ImGui::EndTooltip();
        }
        PopTipStyle();
    }
}

// ------------------------------------------------------------------ values
void BeginEdit(Row& r) { editRow = &r; editFirst = true; snprintf(editBuf, sizeof editBuf, r.isFloat ? "%.2f" : "%.0f", r.val); }
void EditField(Row& r, ImVec2 pos, float w) {
    ImGui::SetCursorScreenPos(pos);
    ImGui::SetNextItemWidth(w);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, C_ACCD);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, R_BTN());
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8 * S, 4 * S));
    if (editFirst) ImGui::SetKeyboardFocusHere();
    bool done = ImGui::InputText("##ed", editBuf, sizeof editBuf,
        ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
    ImGui::PopStyleVar(2); ImGui::PopStyleColor();
    if (done || (!ImGui::IsItemActive() && !editFirst)) {
        if (!ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            r.val = std::max(r.vmin, std::min(r.vmax, (float)atof(editBuf)));
            if (r.live) ApplyRow(r);
        }
        editRow = nullptr; T.Save();
    }
    editFirst = false;
}
// dropdown with padded, rounded options
void Combo(Row& r, ImVec2 pos, float w) {
    std::string cur;
    for (auto& o : r.options) if (o.first == (int)r.val) cur = o.second;
    ImGui::SetCursorScreenPos(pos);
    ImGui::SetNextItemWidth(w);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, C_CARD2); ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, C_BTNH);
    ImGui::PushStyleColor(ImGuiCol_Button, C_CARD2); ImGui::PushStyleColor(ImGuiCol_ButtonHovered, C_BTNH);
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0)); ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0)); ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.10f, 0.10f, 0.135f, 0.99f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(C_ACC.x, C_ACC.y, C_ACC.z, 0.15f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10 * S, 5 * S));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, R_BTN());
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, R_ITEM());
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6 * S, 6 * S));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 2 * S));
    if (ImGui::BeginCombo("##cmb", cur.c_str(), ImGuiComboFlags_HeightLarge)) {
        float iw = ImGui::GetContentRegionAvail().x;
        for (auto& o : r.options) {
            bool sel = o.first == (int)r.val;
            ImVec2 p = ImGui::GetCursorScreenPos();
            float h = 28 * S;
            bool hv = ImGui::IsMouseHoveringRect(p, ImVec2(p.x + iw, p.y + h)) && ImGui::IsWindowHovered();
            if (sel || hv) ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + iw, p.y + h), U32(sel ? C_ACCD : C_HOVER), R_BTN());
            ImGui::PushID(o.first);
            if (ImGui::Selectable("##o", sel, 0, ImVec2(iw, h))) { r.val = (float)o.first; T.Save(); }
            ImGui::PopID();
            PF(sel ? fBold : fReg);
            ImVec2 ts = ImGui::CalcTextSize(o.second.c_str());
            ImGui::GetWindowDrawList()->AddText(ImVec2(p.x + 10 * S, std::floor(p.y + (h - ts.y) * 0.5f)), U32(sel ? C_ACC : C_TEXT), o.second.c_str());
            ImGui::PopFont();
            if (sel && ImGui::IsWindowAppearing()) ImGui::SetScrollHereY();
        }
        ImGui::EndCombo();
    }
    ImGui::PopStyleVar(6); ImGui::PopStyleColor(9);
}
bool UseSlider(const Row& r) { return r.isFloat || (r.vmax - r.vmin) <= 1000.f; }
bool HasVal(const Row& r) {
    switch (r.kind) {
    case K_SLIDER: case K_SET: case K_VALUE: return true;
    case K_ACTION: return !r.noValue;
    default: return false;
    }
}
std::string ValStr(const Row& r) {
    char b[32];
    if (r.isFloat) snprintf(b, sizeof b, r.vmax <= 10 ? "%.2f" : "%.1f", r.val); else snprintf(b, sizeof b, "%.0f", r.val);
    std::string s = b;
    if (r.note == "m" || r.note == "metry") s += " m";
    else if (r.note == "m/s (Shift x4)") s += " m/s";
    else if (r.note == "%" || r.id.rfind("hc", 0) == 0) s += "%";
    else if (r.kind == K_SLIDER && r.isFloat) s = "x" + s;
    return s;
}
// small reset icon, shown only when the value differs from its default
float ResetIcon(Row& r, float right, float cy) {
    if (!HasVal(r) || !r.options.empty() || std::fabs(r.val - r.def) < 1e-4f) return 0;
    float sz = 22 * S;
    ImVec2 p(right - sz, cy - sz * 0.5f);
    ImGui::SetCursorScreenPos(p);
    if (ImGui::InvisibleButton("##rst", ImVec2(sz, sz))) { r.val = r.def; if (r.live) ApplyRow(r); T.Save(); }
    bool hov = ImGui::IsItemHovered();
    if (hov) { DL()->AddRectFilled(p, ImVec2(p.x + sz, p.y + sz), U32(C_BTNH), R_SM()); char b[64]; snprintf(b, sizeof b, r.isFloat ? "Przywróć domyślne (%.2f)" : "Przywróć domyślne (%.0f)", r.def); Tip("%s", b); }
    IconAt(DL(), ImVec2(p.x + sz * 0.5f, p.y + sz * 0.5f), ICN_UNDO, U32(hov ? C_ACC : C_DIM), 12 * S);
    return sz + 4 * S;
}
void NumberBox(Row& r, float vx, float cy, float vw) {
    std::string v = ValStr(r);
    float h = 28 * S;
    if (editRow == &r) { EditField(r, ImVec2(vx, cy - h * 0.5f), vw); return; }
    ImGui::SetCursorScreenPos(ImVec2(vx, cy - h * 0.5f));
    if (ImGui::InvisibleButton("##num", ImVec2(vw, h))) BeginEdit(r);
    bool hv = ImGui::IsItemHovered();
    DL()->AddRectFilled(ImVec2(vx, cy - h * 0.5f), ImVec2(vx + vw, cy + h * 0.5f), U32(hv ? C_BTNH : C_CARD2), R_BTN());
    PF(fBold); ImVec2 ts = ImGui::CalcTextSize(v.c_str()); DL()->AddText(ImVec2(vx + vw - ts.x - 10 * S, std::floor(cy - ts.y * 0.5f)), U32(C_ACC), v.c_str()); ImGui::PopFont();
    if (hv) Tip("kliknij, aby wpisać");
}
// value as plain text (like "1.00 Frames"); click = type exact value
float ValueText(Row& r, float right, float cy) {
    std::string v = ValStr(r);
    float w = 70 * S;
    if (editRow == &r) { EditField(r, ImVec2(right - w, cy - 12 * S), w); return w + 8 * S; }
    PF(fReg); ImVec2 ts = ImGui::CalcTextSize(v.c_str()); ImGui::PopFont();
    ImVec2 p(right - ts.x, std::floor(cy - ts.y * 0.5f));
    ImGui::SetCursorScreenPos(ImVec2(p.x - 4 * S, cy - 11 * S));
    if (ImGui::InvisibleButton("##vl", ImVec2(ts.x + 8 * S, 22 * S))) BeginEdit(r);
    bool hv = ImGui::IsItemHovered();
    if (hv) Tip("kliknij, aby wpisać dokładną wartość");
    Text(p, fReg, hv ? C_TEXT : C_DIM, v.c_str());
    return ts.x + 12 * S;
}
// track only (value is shown in the first line); live rows apply when released
void SliderTrack(Row& r, float x0, float x1, float y) {
    float t = r.vmax > r.vmin ? (r.val - r.vmin) / (r.vmax - r.vmin) : 0;
    t = std::max(0.f, std::min(1.f, t));
    float kx = x0 + (x1 - x0) * t;
    DL()->AddRectFilled(ImVec2(x0, y - 2.5f * S), ImVec2(x1, y + 2.5f * S), U32(C_TRACK), 3 * S);
    if (kx > x0) DL()->AddRectFilled(ImVec2(x0, y - 2.5f * S), ImVec2(kx, y + 2.5f * S), U32(C_ACC), 3 * S);
    ImGui::SetCursorScreenPos(ImVec2(x0 - 8 * S, y - 10 * S));
    ImGui::InvisibleButton("##trk", ImVec2(x1 - x0 + 16 * S, 20 * S));
    bool hov = ImGui::IsItemHovered() || ImGui::IsItemActive();
    DL()->AddCircleFilled(ImVec2(kx, y), (hov ? 7.5f : 6.5f) * S, IM_COL32(255, 255, 255, 255), 20);
    if (ImGui::IsItemActive()) {
        float nt = std::max(0.f, std::min(1.f, (ImGui::GetIO().MousePos.x - x0) / (x1 - x0)));
        float nv = r.vmin + nt * (r.vmax - r.vmin);
        r.val = r.isFloat ? std::round(nv * 100.f) / 100.f : std::round(nv);
    }
    if (ImGui::IsItemDeactivated()) { if (r.live) ApplyRow(r); T.Save(); }
}
bool HasValueLine(const Row& r) {
    switch (r.kind) {
    case K_SLIDER: case K_SET: return UseSlider(r);
    case K_ACTION: return !r.noValue && UseSlider(r);
    case K_VALUE: return r.options.empty() && UseSlider(r);
    default: return false;
    }
}
// build 13: a little more room above the slider when the row has a button (screenshot: Broń jednoręczna)
bool RowHasButton(const Row& r) { return r.kind == K_ACTION || r.kind == K_CHOICE; }
float RowH(const Row& r) { return HasValueLine(r) ? (RowHasButton(r) ? 70 * S : 62 * S) : 42 * S; }

// hotkey chip: visible when bound, while recording, or when the row is hovered
float HotkeyChip(Row& r, float right, float cy, bool rowHov) {
    if (r.kind == K_VALUE) return 0;
    bool rec = bindRow == &r;
    std::string kn;
    if (rec) {
        int mods = ((GetKeyState(VK_CONTROL) & 0x8000) ? HK_CTRL : 0) | ((GetKeyState(VK_MENU) & 0x8000) ? HK_ALT : 0) | ((GetKeyState(VK_SHIFT) & 0x8000) ? HK_SHIFT : 0);
        kn = "Nagrywanie... ";
        if (mods & HK_CTRL) kn += "Ctrl+";
        if (mods & HK_ALT) kn += "Alt+";
        if (mods & HK_SHIFT) kn += "Shift+";
    } else kn = KeyName(r.hotkey);
    if (kn.empty()) { if (!rowHov) return 0; kn = "+ klawisz"; }
    PF(fSmall); ImVec2 ts = ImGui::CalcTextSize(kn.c_str()); ImGui::PopFont();
    ImVec2 sz(ts.x + 16 * S, 22 * S);
    ImVec2 p(right - sz.x, std::floor(cy - sz.y * 0.5f));
    ImGui::SetCursorScreenPos(p);
    if (ImGui::InvisibleButton("##hk", sz)) { bindRow = rec ? nullptr : &r; memset(bindKeys, 1, sizeof bindKeys); }
    bool hov = ImGui::IsItemHovered();
    if (hov && !rec) Tip("Klik = nagraj skrót (może być z Ctrl/Alt/Shift)\nBackspace = usuń, Esc = anuluj");
    bool set = r.hotkey != 0 || rec;
    DL()->AddRectFilled(p, ImVec2(p.x + sz.x, p.y + sz.y), U32(rec ? C_ACCD : (hov ? C_BTNH : C_CARD2)), R_SM());
    if (set) DL()->AddRect(p, ImVec2(p.x + sz.x, p.y + sz.y), U32(C_ACC, rec ? 0.9f : 0.45f), R_SM(), 1.f * S);
    Text(ImVec2(std::floor(p.x + (sz.x - ts.x) * 0.5f), std::floor(p.y + (sz.y - ts.y) * 0.5f)), fSmall, rec ? C_ON : (set ? C_ACC : C_DIM), kn.c_str());
    return sz.x + 8 * S;
}
// recording a shortcut: poll the keyboard directly (works regardless of window messages)
void PollBinding() {
    if (!bindRow) return;
    for (int vk = 1; vk < 256; vk++) {
        bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
        bool was = bindKeys[vk];
        bindKeys[vk] = down;
        if (!down || was) continue;
        if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_CONTROL || vk == VK_MENU || vk == VK_SHIFT || vk == VK_LCONTROL || vk == VK_RCONTROL ||
            vk == VK_LSHIFT || vk == VK_RSHIFT || vk == VK_LMENU || vk == VK_RMENU || vk == VK_LWIN || vk == VK_RWIN || vk == VK_INSERT) continue;
        if (vk == VK_ESCAPE) { bindRow = nullptr; return; }
        if (vk == VK_BACK || vk == VK_DELETE) { bindRow->hotkey = 0; bindRow = nullptr; T.Save(); return; }
        int mods = ((GetAsyncKeyState(VK_CONTROL) & 0x8000) ? HK_CTRL : 0) | ((GetAsyncKeyState(VK_MENU) & 0x8000) ? HK_ALT : 0) | ((GetAsyncKeyState(VK_SHIFT) & 0x8000) ? HK_SHIFT : 0);
        int hk = vk | mods;
        for (auto& o : T.rows) if (&o != bindRow && o.hotkey == hk) { o.hotkey = 0; T.Toast("Skrót przeniesiony z: " + o.name); }
        bindRow->hotkey = hk; bindRow = nullptr; T.Save();
        return;
    }
}

// one setting inside a card: line 1 = label ... [hotkey][reset][value][control], line 2 = slider
void OpenTicketFor(const std::string& id, int type);
void CardRow(Row& r, float x, float y, float w, bool last) {
    ImGui::PushID(r.id.c_str());
    float h = RowH(r);
    float x0 = x + PAD * S, x1 = x + w - PAD * S;
    bool hov = ImGui::IsMouseHoveringRect(ImVec2(x0, y), ImVec2(x1, y + h)) && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    if (hov) DL()->AddRectFilled(ImVec2(x0, y), ImVec2(x1, y + h), U32(C_HOVER, 0.6f), R_ITEM());
    float lx = x0 + 12 * S, cy = y + 21 * S;
    if (T.testerMode) {
        StatusDot(ImVec2(lx + 3 * S, cy), r); lx += 14 * S;
        // ticket button: opens the tester centre with a form for this option
        ImVec2 fb(lx, cy - 10 * S);
        ImGui::SetCursorScreenPos(fb);
        bool fc = ImGui::InvisibleButton("##tkt", ImVec2(20 * S, 20 * S));
        bool fh = ImGui::IsItemHovered();
        int nt = tst::CountFor(r.id, false), nk = (int)tst::KnownFor(r.id).size();
        IconAt(DL(), ImVec2(fb.x + 10 * S, cy), ICN_FLAG, U32(fh ? C_ACC : nk ? C_ST_BUG : nt ? C_DIM : C_FAINT), 11 * S);
        if (fh) Tip("Zgłoś problem / pomysł do tej opcji\n%d wpisów w historii, %d znanych problemów", nt, nk);
        if (fc) OpenTicketFor(r.id, tst::TT_BUG);
        lx += 22 * S;
    }
    if (!r.icon.empty() && iconsOk) { IconAt(DL(), ImVec2(lx + 8 * S, cy), r.icon.c_str(), U32(C_ACC), 14 * S); lx += 26 * S; }
    PF(fReg); ImVec2 ns = ImGui::CalcTextSize(r.name.c_str()); ImGui::PopFont();
    float right = x1 - 12 * S;
    float used = 0;
    switch (r.kind) {
    case K_TOGGLE: case K_SLIDER: case K_SET:
        used = 36 * S;
        if (Switch("##sw", r.on, ImVec2(right - used, cy - 10 * S))) ApplyRow(r);
        break;
    case K_ACTION:
        if (!r.live) { const char* lbl = r.noValue ? "Wykonaj" : (r.id == "ore" || r.id == "xp" || r.id == "lp" ? "Dodaj" : "Wykonaj");
                       used = ButtonW(lbl); if (Button("##do", lbl, ImVec2(right - used, cy - BtnH() * 0.5f))) ApplyRow(r); }
        break;
    case K_CHOICE: {
        const char* lbl = "Ustaw";
        float bw = ButtonW(lbl), cw = std::min(w * 0.42f, 190 * S);
        if (Button("##do", lbl, ImVec2(right - bw, cy - BtnH() * 0.5f))) ApplyRow(r);
        Combo(r, ImVec2(right - bw - 8 * S - cw, cy - 13 * S), cw);
        used = bw + 8 * S + cw;
    } break;
    case K_VALUE:
        if (!r.options.empty()) { float cw = std::min(w * 0.5f, 230 * S); Combo(r, ImVec2(right - cw, cy - 13 * S), cw); used = cw; }
        break;
    }
    float vr = right - used - (used > 0 ? 10 * S : 0);
    if (HasVal(r) && r.options.empty()) {
        if (UseSlider(r)) vr -= ValueText(r, vr, cy);
        else { NumberBox(r, vr - 92 * S, cy, 92 * S); vr -= 100 * S; }
    }
    vr -= ResetIcon(r, vr, cy);
    vr -= HotkeyChip(r, vr, cy, hov);
    float labelMax = vr - 6 * S;
    TextClip(ImVec2(lx, std::floor(cy - ns.y * 0.5f)), fReg, r.kind == K_SET && r.on ? C_ON : C_TEXT, r.name, labelMax);
    if (ImGui::IsMouseHoveringRect(ImVec2(lx, y), ImVec2(std::min(lx + ns.x, labelMax), y + 36 * S)) && (!r.note.empty() || r.kind == K_SET)) {
        std::string tip = r.note;
        if (r.kind == K_SET) tip += std::string(tip.empty() ? "" : "\n") + "Włączone = ta wartość nadpisuje grę; wyłączone = wartość z gry.";
        if (r.live) tip += std::string(tip.empty() ? "" : "\n") + "Zmienia się od razu po puszczeniu suwaka.";
        Tip("%s", tip.c_str());
    }
    if (HasValueLine(r)) SliderTrack(r, x0 + 12 * S, right, y + (RowHasButton(r) ? 53 : 45) * S);
    if (!last) DL()->AddLine(ImVec2(x0 + 12 * S, y + h), ImVec2(x1 - 12 * S, y + h), U32(C_LINE));
    ImGui::PopID();
}

// ------------------------------------------------------------------ cards and masonry layout
// tester mode: small flag that opens a ticket form for any part of the menu (cards, sections, subcategories)
void OpenTicketFor(const std::string& id, int type);
bool testsDrawing = false;   // the tester window itself has no flags
void TicketFlag(const char* target, ImVec2 p) {
    ImGui::SetCursorScreenPos(p);
    ImGui::PushID(target);
    bool c = ImGui::InvisibleButton("##tf", ImVec2(22 * S, 22 * S));
    bool h = ImGui::IsItemHovered();
    ImGui::PopID();
    IconAt(DL(), ImVec2(p.x + 11 * S, p.y + 11 * S), ICN_FLAG, U32(h ? C_ACC : C_FAINT), 11 * S);
    if (h) Tip("Zgłoś problem / pomysł: %s", target);
    if (c) OpenTicketFor(target, 2);
}
struct CardSpec { std::string title; std::vector<Row*> rows; std::function<float(float, float, float)> custom; float customH = 0; bool full = false; Row* headRow = nullptr;
                  std::vector<std::string> tabs; std::vector<Row*> tabMaster; int* tabSel = nullptr; };   // build 16: mode switch (No-clip | Ślizg)
float CardHeight(const CardSpec& c) {
    float h = (c.title.empty() ? 0 : 44 * S) + PAD * S + (c.tabs.empty() ? 0 : 46 * S);
    for (auto* r : c.rows) h += RowH(*r);
    return h + c.customH + PAD * S;
}
void DrawCard(CardSpec& c, float x, float y, float w) {
    float h = CardHeight(c);
    DL()->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), U32(C_CARD), R_CARD());
    float yy = y + PAD * S;
    if (!c.title.empty()) {
        float tr = x + w - 12 * S;
        if (T.testerMode) { tr -= 26 * S; TicketFlag(("karta:" + c.title).c_str(), ImVec2(x + w - 34 * S, y + 12 * S)); }
        if (c.headRow) {   // build 14: one-click action of the card as a button in its header
            float bw = ButtonW(c.headRow->name.c_str());
            tr -= bw + 8 * S;
            ImGui::PushID(c.headRow->id.c_str());
            if (Button("##hb", c.headRow->name.c_str(), ImVec2(tr + 8 * S, y + (44 * S - BtnH()) * 0.5f), true)) ApplyRow(*c.headRow);
            ImGui::PopID();
            tr -= 8 * S;
        }
        TextClip(ImVec2(x + 18 * S, y + 14 * S), fBold, C_TEXT, c.title, tr);
        yy = y + 44 * S;
        DL()->AddLine(ImVec2(x + 18 * S, yy - 1), ImVec2(x + w - 18 * S, yy - 1), U32(C_LINE));
        yy += PAD * S;
    }
    if (!c.tabs.empty() && c.tabSel) {   // segmented control; a dot marks the mode that is switched on
        float sx = x + 18 * S, sw = w - 36 * S, sh = 34 * S, segW = sw / c.tabs.size();
        DL()->AddRectFilled(ImVec2(sx, yy), ImVec2(sx + sw, yy + sh), U32(C_CARD2), R_BTN());
        for (size_t i = 0; i < c.tabs.size(); i++) {
            ImVec2 a(sx + segW * i + 3 * S, yy + 3 * S), b(sx + segW * (i + 1) - 3 * S, yy + sh - 3 * S);
            bool sel = (int)i == *c.tabSel;
            ImGui::SetCursorScreenPos(a);
            ImGui::PushID((c.title + std::to_string(i)).c_str());
            if (ImGui::InvisibleButton("##seg", ImVec2(b.x - a.x, b.y - a.y))) *c.tabSel = (int)i;
            bool hv = ImGui::IsItemHovered();
            ImGui::PopID();
            if (sel || hv) DL()->AddRectFilled(a, b, U32(sel ? C_ACCD : C_HOVER, sel ? 0.9f : 0.6f), R_BTN());
            float tw = TextW(c.tabs[i].c_str(), fReg);
            float tx = (a.x + b.x) * 0.5f - tw * 0.5f;
            Row* m = i < c.tabMaster.size() ? c.tabMaster[i] : nullptr;
            if (m && m->on) { DL()->AddCircleFilled(ImVec2(tx - 10 * S, (a.y + b.y) * 0.5f), 4 * S, U32(C_ON), 12); }
            TextV(tx, (a.y + b.y) * 0.5f, sel ? fBold : fReg, sel ? C_TEXT : C_DIM, c.tabs[i].c_str());
        }
        yy += 46 * S;
    }
    for (size_t i = 0; i < c.rows.size(); i++) { CardRow(*c.rows[i], x, yy, w, i + 1 == c.rows.size() && !c.custom); yy += RowH(*c.rows[i]); }
    if (c.custom) c.custom(x, yy, w);
}
void Masonry(std::vector<CardSpec>& cards, float w) {
    if (cards.empty()) return;
    ImVec2 p = ImGui::GetCursorScreenPos();
    float gap = 12 * S;
    bool two = w > 560 * S;
    float cw = two ? (w - gap) * 0.5f : w;
    float colY[2] = { p.y, p.y };
    for (auto& c : cards) {
        if (c.full || !two) {
            float yy = std::max(colY[0], colY[1]);
            DrawCard(c, p.x, yy, w);
            colY[0] = colY[1] = yy + CardHeight(c) + gap;
            continue;
        }
        int col = colY[1] < colY[0] - 1 ? 1 : 0;
        DrawCard(c, p.x + col * (cw + gap), colY[col], cw);
        colY[col] += CardHeight(c) + gap;
    }
    ImGui::SetCursorScreenPos(ImVec2(p.x, std::max(colY[0], colY[1])));
    ImGui::Dummy(ImVec2(w, 0));
}
std::vector<CardSpec> CardsFor(const std::string& cat, const std::string& sub, const char* q = nullptr) {
    std::vector<CardSpec> out;
    for (auto& r : T.rows) {
        if (r.hidden) continue;
        if (q) { if (!(MatchQ(q, r.name, r.note) || MatchQ(q, r.cat, r.sub) || MatchQ(q, r.group, r.id))) continue; }
        else if (r.cat != cat || r.sub != sub) continue;
        std::string title = q ? r.cat + "  ›  " + r.sub : r.group;
        auto it = std::find_if(out.begin(), out.end(), [&](const CardSpec& c) { return c.title == title; });
        if (it == out.end()) { out.push_back({ title, {}, nullptr, 0, false }); it = out.end() - 1; }
        it->rows.push_back(&r);
    }
    return out;
}
CardSpec* FindCard(std::vector<CardSpec>& v, const char* title) { for (auto& c : v) if (c.title == title) return &c; return nullptr; }
// build 16: modes that exclude each other share one card with a switch on top (rows of the chosen mode below);
// the first row of each mode is its on/off switch. Rows are taken out of the cards they were in.
std::map<std::string, int> g_tabSel;
void TabbedCard(std::vector<CardSpec>& cards, const char* title, std::vector<std::pair<const char*, std::vector<const char*>>> tabs, bool full = false) {
    std::set<std::string> ids; for (auto& t : tabs) for (auto id : t.second) ids.insert(id);
    int at = -1;
    for (int i = 0; i < (int)cards.size(); i++) {
        auto& rs = cards[i].rows; size_t before = rs.size();
        rs.erase(std::remove_if(rs.begin(), rs.end(), [&](Row* r) { return ids.count(r->id) != 0; }), rs.end());
        if (rs.size() != before && at < 0) at = i;
    }
    CardSpec c{ title, {}, nullptr, 0, full };
    auto it = g_tabSel.find(title);
    if (it == g_tabSel.end()) {   // first time: open the mode that is on
        int sel = 0; for (int i = 0; i < (int)tabs.size(); i++) if (Row* m = T.Get(tabs[i].second[0])) if (m->on) sel = i;
        it = g_tabSel.emplace(title, sel).first;
    }
    c.tabSel = &it->second;
    for (auto& t : tabs) { c.tabs.push_back(t.first); c.tabMaster.push_back(T.Get(t.second[0])); }
    int sel = std::max(0, std::min((int)tabs.size() - 1, *c.tabSel));
    for (auto id : tabs[sel].second) if (Row* r = T.Get(id)) c.rows.push_back(r);
    cards.erase(std::remove_if(cards.begin(), cards.end(), [](const CardSpec& k) { return k.rows.empty() && !k.custom && k.tabs.empty(); }), cards.end());
    if (at < 0 || at > (int)cards.size()) at = (int)cards.size();
    cards.insert(cards.begin() + at, c);
}
// build 17: several titled groups of checkmarks inside ONE existing card (everything about a feature in one place)
void ChecksSections(std::vector<CardSpec>& cards, const char* title, std::vector<std::pair<const char*, std::vector<const char*>>> secs) {
    float h = 6 * S; for (auto& sc : secs) h += 34 * S + (sc.second.size() + 1) / 2 * 34 * S;
    auto fn = [secs, h](float x, float y, float cw) -> float {
        float yy = y + 6 * S;
        for (auto& sc : secs) {
            DL()->AddLine(ImVec2(x + 18 * S, yy), ImVec2(x + cw - 18 * S, yy), U32(C_LINE));
            Text(ImVec2(x + 18 * S, yy + 8 * S), fBold, C_TEXT, sc.first);
            yy += 34 * S;
            int i = 0;
            for (auto id : sc.second) {
                Row* r = T.Get(id); if (!r) continue;
                float cx = x + 18 * S + (i % 2) * (cw - 36 * S) * 0.5f, cy = yy + (i / 2) * 34 * S;
                ImGui::PushID(id);
                if (Check("##c", r->on, ImVec2(cx, cy))) { r->on = !r->on; tst::Event(r->id, r->on ? "włączono" : "wyłączono"); T.Save(); }
                ImGui::PopID();
                Text(ImVec2(cx + 28 * S, cy + 1 * S), fReg, r->on ? C_TEXT : C_DIM, r->name.c_str());
                i++;
            }
            yy += (i + 1) / 2 * 34 * S;
        }
        return h;
    };
    if (CardSpec* c = FindCard(cards, title)) { c->customH = h; c->custom = fn; return; }
    CardSpec cs{ title, {}, nullptr, h, false }; cs.custom = fn; cards.push_back(cs);
}
// build 15: a card of checkmarks for hidden toggle rows (2 columns); added to an existing card of that title or as a new one
void ChecksCard(std::vector<CardSpec>& cards, const char* title, std::vector<const char*> ids, const char* header = nullptr) {
    int n = 0; for (auto id : ids) if (T.Get(id)) n++;
    float hh = header ? 34 * S : 8 * S;
    float h = hh + (n + 1) / 2 * 34 * S + 8 * S;
    auto fn = [ids, header, hh, h](float x, float y, float cw) -> float {
        if (header) Text(ImVec2(x + 18 * S, y + 10 * S), fBold, C_TEXT, header);
        int i = 0;
        for (auto id : ids) {
            Row* r = T.Get(id); if (!r) continue;
            float cx = x + 18 * S + (i % 2) * (cw - 36 * S) * 0.5f, cy = y + hh + (i / 2) * 34 * S;
            ImGui::PushID(id);
            if (Check("##c", r->on, ImVec2(cx, cy))) { r->on = !r->on; tst::Event(r->id, r->on ? "włączono" : "wyłączono"); T.Save(); }
            if (T.testerMode) {}
            ImGui::PopID();
            Text(ImVec2(cx + 28 * S, cy + 1 * S), fReg, r->on ? C_TEXT : C_DIM, r->name.c_str());
            if (ImGui::IsMouseHoveringRect(ImVec2(cx, cy), ImVec2(cx + (cw - 36 * S) * 0.5f, cy + 24 * S)) && !r->note.empty()) Tip("%s", r->note.c_str());
            i++;
        }
        return h;
    };
    if (CardSpec* c = FindCard(cards, title)) { c->customH = h; c->custom = fn; return; }
    CardSpec cs{ title, {}, nullptr, h, false }; cs.custom = fn; cards.push_back(cs);
}
void SectionLabel(const char* t) {
    ImGui::Dummy(ImVec2(0, 6 * S));
    ImVec2 sp = ImGui::GetCursorScreenPos();
    Text(sp, fBig, C_TEXT, t);
    if (T.testerMode && !testsDrawing) TicketFlag(("sekcja:" + std::string(t)).c_str(), ImVec2(sp.x + TextW(t, fBig) + 10 * S, sp.y + 2 * S));
    ImGui::SetCursorScreenPos(sp);
    ImGui::Dummy(ImVec2(0, 30 * S));
}
void BeginPanel(const char* id, const char* title, float w) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, C_CARD);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, R_CARD());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18 * S, 14 * S));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8 * S, 8 * S));
    ImGui::BeginChild(id, ImVec2(w, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding);
    if (title) { PF(fBold); ImGui::TextColored(C_TEXT, "%s", title); ImGui::PopFont(); }
}
// build 9: panels end exactly after their last item (#11). A trailing NewLine() after a button row used to add
// an empty line; a SetCursorScreenPos() without an item afterwards tripped ImGui's boundary check (#5).
void EndPanel() {
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    if (win->DC.CurrLineSize.y > 0) ImGui::NewLine();   // finish a SameLine() row
    float sp = ImGui::GetStyle().ItemSpacing.y;
    if (win->DC.IsSetPos || win->DC.CursorPos.y - sp > win->DC.CursorMaxPos.y + 0.5f) {
        ImGui::SetCursorScreenPos(ImVec2(win->DC.CursorPos.x, std::max(win->DC.CursorMaxPos.y, win->DC.CursorPos.y - sp)));
        ImGui::Dummy(ImVec2(0, 0));
    }
    ImGui::EndChild(); ImGui::PopStyleVar(3); ImGui::PopStyleColor(); ImGui::Dummy(ImVec2(0, 4 * S)); }
void ListRowBg(ImVec2 p, float w, float h, int i) {
    bool hov = ImGui::IsMouseHoveringRect(p, ImVec2(p.x + w, p.y + h)) && ImGui::IsWindowHovered();
    DL()->AddRectFilled(p, ImVec2(p.x + w, p.y + h), U32(hov ? C_HOVER : (i & 1 ? C_CARD : C_CARD2)), R_ITEM());
}

// ------------------------------------------------------------------ portraits
void AskPhoto(uintptr_t ptr, const std::string& key) {
    if (!ptr || key.empty() || PortraitFor(key)) return;
    double now = NowSec();
    auto& a = photoAsked[key];
    if (a.second >= 2 || now - a.first < 6.0) return;   // two tries per character, then give up
    a.first = now; a.second++;
    Act_Photo(ptr);
}
// portrait: transparent background, faded bottom, no frame; placeholder = faint icon
void Avatar(ImVec2 c, float r, bool monster, uintptr_t ptr, const std::string& key, bool ask = false) {
    if (void* tex = PortraitFor(key)) {
        DL()->AddImage((ImTextureID)(intptr_t)tex, ImVec2(c.x - r, c.y - r), ImVec2(c.x + r, c.y + r));
        return;
    }
    DL()->AddCircleFilled(c, r * 0.82f, U32(C_CARD2), 40);
    IconAt(DL(), c, monster ? ICN_PAW : ICN_USER, U32(C_FAINT), r * 0.75f);
    if (ask) AskPhoto(ptr, key);
}
void HpBar(float x0, float x1, float y, int v, int m, const ImVec4& c, float th = 5) {
    float f = m > 0 ? std::max(0.f, std::min(1.f, (float)v / m)) : 0;
    DL()->AddRectFilled(ImVec2(x0, y), ImVec2(x1, y + th * S), U32(C_TRACK), th * 0.5f * S);
    if (f > 0) DL()->AddRectFilled(ImVec2(x0, y), ImVec2(x0 + (x1 - x0) * f, y + th * S), U32(c), th * 0.5f * S);
}
// segmented control: equal padding inside, small gap between segments
int Segmented(const char* id, const std::vector<const char*>& opts, int cur, ImVec2 p) {
    float h = 32 * S, gap = 4 * S, pad = 4 * S;
    float total = pad * 2 + gap * (opts.size() - 1);
    for (auto o : opts) total += TextW(o, fBold) + 32 * S;
    DL()->AddRectFilled(p, ImVec2(p.x + total, p.y + h + pad * 2), U32(C_CARD2), R_ITEM());
    float x = p.x + pad;
    int res = cur;
    for (int i = 0; i < (int)opts.size(); i++) {
        float bw = TextW(opts[i], fBold) + 32 * S;
        ImGui::PushID(id); ImGui::PushID(i);
        ImGui::SetCursorScreenPos(ImVec2(x, p.y + pad));
        if (ImGui::InvisibleButton("##seg", ImVec2(bw, h))) res = i;
        bool hv = ImGui::IsItemHovered(), act = i == cur;
        if (act || hv) DL()->AddRectFilled(ImVec2(x, p.y + pad), ImVec2(x + bw, p.y + pad + h), U32(act ? C_ACCD : C_HOVER), R_BTN());
        PF(act ? fBold : fReg); ImVec2 ts = ImGui::CalcTextSize(opts[i]); ImGui::PopFont();
        Text(ImVec2(std::floor(x + (bw - ts.x) * 0.5f), std::floor(p.y + pad + (h - ts.y) * 0.5f)), act ? fBold : fReg, act ? C_ACC : (hv ? C_TEXT : C_DIM), opts[i]);
        ImGui::PopID(); ImGui::PopID();
        x += bw + gap;
    }
    return res;
}
float SegmentedW(const std::vector<const char*>& opts) { float t = 8 * S + 4 * S * (opts.size() - 1); for (auto o : opts) t += TextW(o, fBold) + 32 * S; return t; }
// "Kogo edytujesz" for the Postać pages (+ which target when "Cel")
void SubjectBar(const Snapshot& s, float w) {
    Row* r = T.Get("subject");
    if (!r) return;
    ImVec2 p = ImGui::GetCursorScreenPos();
    float h = 76 * S;
    DL()->AddRectFilled(p, ImVec2(p.x + w, p.y + h), U32(C_CARD), R_CARD());
    Avatar(ImVec2(p.x + 18 * S + 28 * S, p.y + h * 0.5f), 28 * S, false, s.subjectPtr, s.subjectInst);
    Text(ImVec2(p.x + 86 * S, p.y + 18 * S), fSmall, C_DIM, "Edytujesz");
    std::vector<const char*> opts = { "Obecna postać", "Bezimienny", "Cel" };
    float sw = SegmentedW(opts);
    float sx = p.x + w - 18 * S - sw;
    TextClip(ImVec2(p.x + 86 * S, p.y + 36 * S), fBold, C_TEXT, s.subjectName.empty() ? "-" : s.subjectName, sx - 12 * S);
    int nv = Segmented("subj", opts, (int)r->val, ImVec2(sx, p.y + 18 * S));
    if (nv != (int)r->val) { r->val = (float)nv; T.Save(); }
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + 10 * S)); ImGui::Dummy(ImVec2(w, 0));
    if ((int)r->val == 2) {   // which target
        ImVec2 q = ImGui::GetCursorScreenPos();
        float hh = 52 * S;
        DL()->AddRectFilled(q, ImVec2(q.x + w, q.y + hh), U32(C_CARD), R_CARD());
        TextV(q.x + 18 * S, q.y + hh * 0.5f, fReg, C_TEXT, "Który cel");
        std::string cur = s.activeSaved >= 0 && s.activeSaved < (int)s.saved.size() ? s.saved[s.activeSaved].name + "  (zapisany)" : (s.tValid ? s.tName + "  (pod wzrokiem)" : "pod wzrokiem - brak");
        float cw = std::min(w * 0.55f, 360 * S);
        ImGui::SetCursorScreenPos(ImVec2(q.x + w - 18 * S - cw, q.y + (hh - 28 * S) * 0.5f));
        ImGui::SetNextItemWidth(cw);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, C_CARD2); ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, R_BTN());
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10 * S, 5 * S));
        if (ImGui::BeginCombo("##tgtpick", cur.c_str())) {
            if (ImGui::Selectable("Postać pod wzrokiem", s.activeSaved < 0)) Act_SelectTargetFor(-1);
            for (int i = 0; i < (int)s.saved.size(); i++) {
                ImGui::PushID(i);
                if (ImGui::Selectable((s.saved[i].name + "  ·  " + s.saved[i].inst).c_str(), s.activeSaved == i)) Act_SelectTargetFor(i);
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
        ImGui::PopStyleVar(2); ImGui::PopStyleColor();
        ImGui::SetCursorScreenPos(ImVec2(q.x, q.y + hh + 10 * S)); ImGui::Dummy(ImVec2(w, 0));
    }
}

// ================================================================== pages
std::vector<std::string> SubsOf(int cat) {
    std::vector<std::string> subs;
    for (auto& e : g_cat.items) if (e.cat == cat && std::find(subs.begin(), subs.end(), e.sub) == subs.end()) subs.push_back(e.sub);
    std::sort(subs.begin(), subs.end());
    return subs;
}
const ItemEntry* ItemById(const std::string& id) { for (auto& x : g_cat.items) if (x.id == id) return &x; return nullptr; }
const char* CatIcon(int c) {
    switch (c) {
    case IC_MELEE: return ICN_GAVEL; case IC_RANGED: return ICN_CROSSHAIR; case IC_AMMO: return ICN_ARROW; case IC_ARMOR: return ICN_SHIELD;
    case IC_RUNE: return ICN_DIAMOND; case IC_SCROLL: return ICN_FILE; case IC_POTION: return ICN_FLASK; case IC_FOOD: return ICN_CUTLERY;
    case IC_MAGIC: return ICN_RING; case IC_KEY: return ICN_KEY; case IC_DOCS: return ICN_BOOK; default: return ICN_CUBE;
    }
}
int ReqOf(const ItemEntry& e, int atr) { for (int i = 0; i < 3; i++) if (e.condAtr[i] == atr && e.condVal[i] > 0) return e.condVal[i]; return 0; }
int ProtOf(const ItemEntry& e) { int m = 0; for (int i = 0; i < 8; i++) m = std::max(m, e.prot[i]); return m; }
// ---- item stats (every number the scripts give an item becomes a filterable stat)
std::vector<std::vector<std::pair<std::string, int>>> g_istat;
void BuildItemStats() {
    if (g_istat.size() == g_cat.items.size()) return;
    g_istat.clear();
    static const char* prot[8] = { nullptr, "Ochrona: obuchowe", "Ochrona: ostrza", "Ochrona: ogień", nullptr, "Ochrona: magia", "Ochrona: strzały", nullptr };
    for (auto& e : g_cat.items) {
        std::vector<std::pair<std::string, int>> kv;
        auto add = [&](const std::string& k, int v) { for (auto& p : kv) if (p.first == k) return; kv.push_back({ k, v }); };
        add("Wartość", e.value);
        add("Cena sprzedaży", e.value / 2);
        if (e.damage) add("Obrażenia", e.damage);
        if (e.range) add("Zasięg", e.range);
        for (int i = 0; i < 8; i++) if (prot[i] && e.prot[i]) add(prot[i], e.prot[i]);
        for (int i = 0; i < 3; i++) if (e.condVal[i] > 0) {
            const char* n = e.condAtr[i] == 4 ? "Wymagana siła" : e.condAtr[i] == 5 ? "Wymagana zręczność" : e.condAtr[i] == 3 ? "Wymagana mana" : "Wymaganie";
            add(n, e.condVal[i]);
        }
        for (int k = 0; k < 6; k++) {
            if (e.text[k].empty() || !e.count[k]) continue;
            std::string t = e.text[k];
            while (!t.empty() && (t.back() == ':' || t.back() == ' ')) t.pop_back();
            if (!t.empty()) add(t, e.count[k]);
        }
        g_istat.push_back(kv);
    }
}
bool StatOf(size_t i, const std::string& k, int& v) { for (auto& p : g_istat[i]) if (p.first == k) { v = p.second; return true; } return false; }
struct StatFilter { std::string stat; int mn = 0, mx = 0; };
std::vector<StatFilter> g_ifilters;
std::string g_isort = "Nazwa"; bool g_isortDesc = false;
void PopupStyle() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16 * S, 14 * S));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, R_ITEM());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8 * S, 8 * S));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(C_ACC.x, C_ACC.y, C_ACC.z, 0.15f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.09f, 0.09f, 0.12f, 0.99f));
}
void PopupStyleEnd() { ImGui::PopStyleColor(2); ImGui::PopStyleVar(4); }
// generic string dropdown
bool StrCombo(const char* id, const std::vector<std::string>& opts, std::string& cur, float w, const char* placeholder = "-") {
    bool ch = false;
    ImGui::SetNextItemWidth(w);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, C_CARD2); ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, R_BTN());
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10 * S, 7 * S));
    if (ImGui::BeginCombo(id, cur.empty() ? placeholder : cur.c_str(), ImGuiComboFlags_HeightLarge)) {
        for (auto& o : opts) { if (ImGui::Selectable(o.c_str(), o == cur)) { cur = o; ch = true; } }
        ImGui::EndCombo();
    }
    ImGui::PopStyleVar(2); ImGui::PopStyleColor();
    return ch;
}
void AskItemIcon(const ItemEntry& e) {
    std::string key = "I_" + e.id;
    if (PortraitFor(key)) return;
    double now = NowSec();
    auto& a = photoAsked[key];
    if (a.second >= 1 || now - a.first < 6.0) return;
    a.first = now; a.second++;
    Act_PhotoItem(e.sym);
}
// ---- Ekwipunek / Spawner
void ItemSpawner(float w) {
    if (!g_cat.built) { ImGui::TextColored(C_DIM, "Wczytaj grę, żeby zobaczyć listę przedmiotów."); return; }
    BuildItemStats();
    if (Row* ore = T.Get("ore")) { std::vector<CardSpec> c{ { "", { ore }, nullptr, 0, true } }; Masonry(c, w); }
    // stats available in the current category / subcategory
    std::vector<std::string> stats;
    for (size_t i = 0; i < g_cat.items.size(); i++) {
        auto& e = g_cat.items[i];
        if ((itemCat >= 0 && e.cat != itemCat) || (!itemSub.empty() && e.sub != itemSub)) continue;
        for (auto& p : g_istat[i]) if (std::find(stats.begin(), stats.end(), p.first) == stats.end()) stats.push_back(p.first);
    }
    BeginPanel("##ispf", nullptr, w);
    float fbw = 2 * (ImGui::GetFontSize() + 14 * S) + 8 * S, catW = 210 * S, amtW = 90 * S;
    InputBox("##isearch", search, sizeof search, "Szukaj: nazwa albo ID (np. miecz, ITMW_)...", w - 36 * S - catW - amtW - fbw - TextW("Ilość", fReg) - 5 * ImGui::GetStyle().ItemSpacing.x);
    ImGui::SameLine();
    {   // category dropdown with icons
        std::string cur = itemCat < 0 ? "Wszystkie kategorie" : kItemCatNames[itemCat];
        ImGui::SetNextItemWidth(catW);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, C_CARD2); ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, R_BTN());
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10 * S, 7 * S));
        if (ImGui::BeginCombo("##icat", cur.c_str(), ImGuiComboFlags_HeightLarge)) {
            if (ImGui::Selectable("     Wszystkie kategorie", itemCat < 0)) { itemCat = -1; itemSub.clear(); }
            for (int c = 0; c < IC_COUNT; c++) {
                ImVec2 ip = ImGui::GetCursorScreenPos();
                ImGui::PushID(c);
                if (ImGui::Selectable((std::string("     ") + kItemCatNames[c]).c_str(), itemCat == c)) { itemCat = c; itemSub.clear(); }
                ImGui::PopID();
                IconAt(ImGui::GetWindowDrawList(), ImVec2(ip.x + 9 * S, ip.y + ImGui::GetTextLineHeight() * 0.5f), CatIcon(c), U32(C_ACC), 12 * S);
            }
            ImGui::EndCombo();
        }
        ImGui::PopStyleVar(2); ImGui::PopStyleColor();
    }
    ImGui::SameLine(); ImGui::AlignTextToFramePadding(); ImGui::TextColored(C_DIM, "Ilość"); ImGui::SameLine();
    NumField("##amt", &T.itemAmount, 1, 100000, amtW);
    ImGui::SameLine();
    // build 7: sort and filters are icon-only buttons (tooltip says what they do)
    ImVec2 fp = ImGui::GetCursorScreenPos();
    int nf = (int)g_ifilters.size() + (itemSub.empty() ? 0 : 1);
    float ib = ImGui::GetFontSize() + 14 * S, iy = fp.y;   // same height as the search box
    bool sorted = g_isort != "Nazwa" || g_isortDesc;
    if (IconButton("##isortb", ICN_SORT, ("Sortowanie: " + g_isort + (g_isortDesc ? " (malejąco)" : " (rosnąco)")).c_str(), ImVec2(fp.x, iy), ib, C_TEXT, sorted))
        ImGui::OpenPopup("##isortp");
    char ft[48]; snprintf(ft, sizeof ft, nf ? "Filtry (aktywne: %d)" : "Filtry", nf);
    if (IconButton("##flt", ICN_FILTER, ft, ImVec2(fp.x + ib + 8 * S, iy), ib, C_TEXT, nf > 0)) ImGui::OpenPopup("##ifilters");
    if (nf) {   // count badge
        ImVec2 bc(fp.x + ib * 2 + 8 * S - 3 * S, iy + 4 * S);
        DL()->AddCircleFilled(bc, 7 * S, U32(C_ACC), 14);
        char nb[8]; snprintf(nb, sizeof nb, "%d", nf);
        PF(fSmall); ImVec2 ts = ImGui::CalcTextSize(nb); DL()->AddText(ImVec2(bc.x - ts.x * 0.5f, bc.y - ts.y * 0.5f), IM_COL32_WHITE, nb); ImGui::PopFont();
    }
    ImGui::SetCursorScreenPos(ImVec2(fp.x, fp.y + BtnH()));
    PopupStyle();
    ImGui::SetNextWindowSize(ImVec2(260 * S, 0));
    if (ImGui::BeginPopup("##isortp")) {
        PF(fBold); ImGui::TextColored(C_TEXT, "Sortuj według"); ImGui::PopFont();
        std::vector<std::string> so = { "Nazwa" }; for (auto& st : stats) so.push_back(st);
        for (auto& o : so) if (ImGui::Selectable(o.c_str(), g_isort == o)) g_isort = o;
        ImGui::Separator();
        if (ImGui::Selectable(ICN_SORT "  Rosnąco", !g_isortDesc, ImGuiSelectableFlags_NoAutoClosePopups)) g_isortDesc = false;
        if (ImGui::Selectable(ICN_SORT "  Malejąco", g_isortDesc, ImGuiSelectableFlags_NoAutoClosePopups)) g_isortDesc = true;
        ImGui::EndPopup();
    }
    PopupStyleEnd();
    ImGui::SetCursorScreenPos(ImVec2(fp.x, fp.y + BtnH()));
    PopupStyle();
    ImGui::SetNextWindowSize(ImVec2(520 * S, 0));
    if (ImGui::BeginPopup("##ifilters")) {
        PF(fBold); ImGui::TextColored(C_TEXT, "Filtry"); ImGui::PopFont();
        if (itemCat >= 0) {
            auto subs = SubsOf(itemCat);
            if (subs.size() > 1) {
                ImGui::AlignTextToFramePadding(); ImGui::TextColored(C_DIM, "Podkategoria"); ImGui::SameLine(140 * S);
                std::vector<std::string> o = { "wszystkie" }; o.insert(o.end(), subs.begin(), subs.end());
                std::string cur = itemSub.empty() ? "wszystkie" : itemSub;
                if (StrCombo("##isub", o, cur, 340 * S)) itemSub = cur == "wszystkie" ? "" : cur;
            }
        } else { PF(fSmall); ImGui::TextColored(C_DIM, "Wybierz kategorię, żeby filtrować po jej statystykach."); ImGui::PopFont(); }
        int del = -1;
        for (int i = 0; i < (int)g_ifilters.size(); i++) {
            auto& f = g_ifilters[i];
            ImGui::PushID(i + 6100);
            StrCombo("##st", stats, f.stat, 210 * S, "statystyka"); ImGui::SameLine();
            ImGui::AlignTextToFramePadding(); ImGui::TextColored(C_DIM, "od"); ImGui::SameLine(); NumField("##mn", &f.mn, -100000, 1000000, 70 * S); ImGui::SameLine();
            ImGui::AlignTextToFramePadding(); ImGui::TextColored(C_DIM, "do"); ImGui::SameLine(); NumField("##mx", &f.mx, 0, 1000000, 70 * S); ImGui::SameLine();
            if (IconButton("##rm", ICN_TIMES, "Usuń filtr", ImGui::GetCursorScreenPos(), 30 * S, C_DIM)) del = i;
            ImGui::NewLine();
            ImGui::PopID();
        }
        if (del >= 0) g_ifilters.erase(g_ifilters.begin() + del);
        if (g_ifilters.empty()) { PF(fSmall); ImGui::TextColored(C_DIM, "Brak filtrów statystyk. \"do\" = 0 oznacza bez górnej granicy."); ImGui::PopFont(); }
        ImVec2 bp = ImGui::GetCursorScreenPos();
        if (Button("##addf", "+ Dodaj filtr", bp, false)) g_ifilters.push_back({ stats.empty() ? "Wartość" : stats[0], 0, 0 });
        if (Button("##clrf", "Wyczyść", ImVec2(bp.x + ButtonW("+ Dodaj filtr") + 8 * S, bp.y), false)) { g_ifilters.clear(); itemSub.clear(); }
        ImGui::SetCursorScreenPos(ImVec2(bp.x, bp.y + BtnH() + 6 * S)); ImGui::Dummy(ImVec2(10, 0));
        ImGui::EndPopup();
    }
    PopupStyleEnd();
    // active stat filters as removable chips (the subcategory lives only inside the filter popup)
    if (!g_ifilters.empty()) ImGui::NewLine();
    for (int i = 0; i < (int)g_ifilters.size(); i++) {
        auto& f = g_ifilters[i];
        char b[160]; snprintf(b, sizeof b, f.mx > 0 ? "%s %d–%d  ×##f%d" : "%s ≥ %d  ×##f%d", f.stat.c_str(), f.mn, f.mx, i);
        if (f.mx <= 0) snprintf(b, sizeof b, "%s ≥ %d  ×##f%d", f.stat.c_str(), f.mn, i);
        if (Chip(b, true)) { g_ifilters.erase(g_ifilters.begin() + i); break; }
    }
    EndPanel();
    std::vector<int> v;
    for (size_t i = 0; i < g_cat.items.size(); i++) {
        const ItemEntry& e = g_cat.items[i];
        if (itemCat >= 0 && e.cat != itemCat) continue;
        if (!itemSub.empty() && e.sub != itemSub) continue;
        if (!(MatchQ(search, e.name, e.id) || MatchQ(search, e.info, e.sub))) continue;
        bool ok = true;
        for (auto& f : g_ifilters) { int x; if (!StatOf(i, f.stat, x) || x < f.mn || (f.mx > 0 && x > f.mx)) { ok = false; break; } }
        if (ok) v.push_back((int)i);
    }
    if (g_isort != "Nazwa") {
        std::stable_sort(v.begin(), v.end(), [](int a, int b) {
            int x = INT_MIN, y = INT_MIN; StatOf(a, g_isort, x); StatOf(b, g_isort, y);
            return g_isortDesc ? x > y : x < y; });
    } else if (g_isortDesc) std::reverse(v.begin(), v.end());
    PF(fSmall); ImGui::TextColored(C_DIM, "%d przedmiotów  ·  + dodaje do edytora zestawu (Ekwipunek › Zestawy)", (int)v.size()); ImGui::PopFont();
    float h = 58 * S;
    ImGuiListClipper clip; clip.Begin((int)v.size(), h + 4 * S);
    while (clip.Step())
        for (int k = clip.DisplayStart; k < clip.DisplayEnd; k++) {
            const ItemEntry& e = g_cat.items[v[k]];
            ImVec2 p = ImGui::GetCursorScreenPos();
            ListRowBg(p, w, h, k);
            float ic = 44 * S;
            ImVec2 ip(p.x + 8 * S, p.y + (h - ic) * 0.5f);
            if (void* tex = PortraitFor("I_" + e.id)) DL()->AddImage((ImTextureID)(intptr_t)tex, ip, ImVec2(ip.x + ic, ip.y + ic));
            else {
                DL()->AddRectFilled(ip, ImVec2(ip.x + ic, ip.y + ic), U32(C_ACCD, 0.5f), R_BTN());
                IconAt(DL(), ImVec2(ip.x + ic * 0.5f, ip.y + ic * 0.5f), CatIcon(e.cat), U32(C_ACC), 15 * S);
                AskItemIcon(e);
            }
            float x = p.x + 62 * S;
            if (T.Crashed("item:" + e.id)) { float pw; Pill(ImVec2(x, p.y + 8 * S), "CRASH", C_ST_CRASH, &pw); x += pw + 6 * S; }
            float bx = p.x + w - 10 * S;
            ImGui::PushID(e.sym);
            float b3 = ButtonW("Celowi", fReg), b2 = ButtonW("Pod nogi", fReg), b1 = ButtonW("Do ekwipunku", fReg), by = p.y + (h - BtnH()) * 0.5f;
            if (IconButton("##addset", ICN_PLUS, "Dodaj do edytowanego zestawu", ImVec2(bx - BtnH(), by), BtnH(), C_ACC)) {
                bool found = false;
                for (auto& it : T.editSet.items) if (it.first == e.id) { it.second += T.itemAmount; found = true; }
                if (!found) T.editSet.items.push_back({ e.id, T.itemAmount });
                T.Toast("Do zestawu: " + e.name);
            }
            bx -= BtnH() + 8 * S;
            if (Button("##tgt", "Celowi", ImVec2(bx - b3, by), false, fReg)) Act_GiveItemTarget(e.sym, T.itemAmount);
            bx -= b3 + 6 * S;
            if (Button("##drop", "Pod nogi", ImVec2(bx - b2, by), false, fReg)) Act_DropItem(e.sym, T.itemAmount);
            bx -= b2 + 6 * S;
            if (Button("##inv", "Do ekwipunku", ImVec2(bx - b1, by), true, fReg)) Act_GiveItem(e.sym, T.itemAmount);
            bx -= b1 + 10 * S;
            ImGui::PopID();
            TextClip(ImVec2(x, p.y + 10 * S), fBold, C_TEXT, e.name, bx);
            char sub[300];
            snprintf(sub, sizeof sub, "%s%s%s  ·  %d rudy", e.sub.empty() ? kItemCatNames[e.cat] : e.sub.c_str(), e.info.empty() ? "" : "  ·  ", e.info.c_str(), e.value / 2);
            TextClip(ImVec2(p.x + 62 * S, p.y + 31 * S), fSmall, C_DIM, sub, bx);
            if (ImGui::IsMouseHoveringRect(p, ImVec2(bx, p.y + h)) && ImGui::IsWindowHovered()) {
                std::vector<std::pair<std::string, std::string>> kv = { { "Kategoria", e.sub.empty() ? kItemCatNames[e.cat] : e.sub }, { "ID", e.id } };
                for (auto& st : g_istat[v[k]]) kv.push_back({ st.first, std::to_string(st.second) });
                TipKV(e.name, kv);
            }
            ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + 4 * S)); ImGui::Dummy(ImVec2(w, 0));
        }
}
// ---- Ekwipunek / Zestawy
void SetsPage(float w) {
    SectionLabel("Moje zestawy");
    BeginPanel("##setedit", "Edytor zestawu", w);
    InputBox("##setname", setName, sizeof setName, "nazwa zestawu...", w * 0.45f);
    ImGui::SameLine();
    ImVec2 bp = ImGui::GetCursorScreenPos();
    if (Button("##savez", "Zapisz zestaw", bp) && setName[0] && !T.editSet.items.empty()) {
        T.editSet.name = setName;
        bool replaced = false;
        for (auto& s : T.sets) if (s.name == T.editSet.name) { s = T.editSet; replaced = true; }
        if (!replaced) T.sets.push_back(T.editSet);
        T.SaveSets(); T.editSet = ItemSet(); setName[0] = 0;
        T.Toast("Zapisano zestaw");
    }
    if (Button("##clearz", "Wyczyść", ImVec2(bp.x + ButtonW("Zapisz zestaw") + 8 * S, bp.y), false)) T.editSet = ItemSet();
    ImGui::SetCursorScreenPos(ImVec2(ImGui::GetWindowPos().x + 18 * S, bp.y + 40 * S));
    if (T.editSet.items.empty()) { PF(fSmall); ImGui::TextColored(C_DIM, "Pusty. Dodawaj przedmioty przyciskiem  +  w Ekwipunek › Spawner (ilość z pola Ilość)."); ImGui::PopFont(); }
    for (int i = 0; i < (int)T.editSet.items.size(); i++) {
        auto& it = T.editSet.items[i];
        ImGui::PushID(i + 8000);
        const ItemEntry* e = ItemById(it.first);
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(C_TEXT, "%s", e ? e->name.c_str() : it.first.c_str());
        ImGui::SameLine(w * 0.55f);
        NumField("##cnt", &it.second, 1, 100000, 80 * S); ImGui::SameLine();
        bool rm = IconButton("##rm", ICN_TIMES, "Usuń z zestawu", ImGui::GetCursorScreenPos(), 28 * S, C_ST_CRASH);
        ImGui::NewLine();
        ImGui::PopID();
        if (rm) { T.editSet.items.erase(T.editSet.items.begin() + i); break; }
    }
    EndPanel();
    std::vector<int> idx;
    for (int i = 0; i < (int)T.sets.size(); i++) if (!T.sets[i].preset) idx.push_back(i);
    if (idx.empty()) { PF(fSmall); ImGui::TextColored(C_DIM, "Nie masz jeszcze zapisanych zestawów."); ImGui::PopFont(); return; }
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    float gap = 12 * S, cw = (w - gap) * 0.5f, ch = 112 * S;
    int del = -1;
    for (size_t k = 0; k < idx.size(); k++) {
        ItemSet& s = T.sets[idx[k]];
        float x = p0.x + (k % 2) * (cw + gap), y = p0.y + (k / 2) * (ch + gap);
        DL()->AddRectFilled(ImVec2(x, y), ImVec2(x + cw, y + ch), U32(C_CARD), R_CARD());
        char cnt[32]; snprintf(cnt, sizeof cnt, "%d pozycji", (int)s.items.size());
        float tw = TextW(cnt, fSmall);
        Pill(ImVec2(x + cw - tw - 32 * S, y + 14 * S), cnt, C_ACC);
        TextClip(ImVec2(x + 18 * S, y + 14 * S), fBold, C_TEXT, s.name, x + cw - tw - 40 * S);
        std::string list;
        for (size_t j = 0; j < s.items.size() && j < 6; j++) {
            const ItemEntry* e = ItemById(s.items[j].first);
            if (!list.empty()) list += ", ";
            list += (e ? e->name : s.items[j].first) + " x" + std::to_string(s.items[j].second);
        }
        if (s.items.size() > 6) list += " ...";
        DL()->PushClipRect(ImVec2(x, y), ImVec2(x + cw - 14 * S, y + ch - 44 * S), true);
        PF(fSmall); DL()->AddText(nullptr, 0, ImVec2(x + 18 * S, y + 40 * S), U32(C_DIM), list.c_str(), nullptr, cw - 36 * S); ImGui::PopFont();
        DL()->PopClipRect();
        ImGui::PushID(idx[k] + 7000);
        float by = y + ch - BtnH() - 14 * S;
        if (Button("##give", "Daj", ImVec2(x + 18 * S, by))) Act_GiveSet(s);
        if (Button("##edit", "Edytuj", ImVec2(x + 18 * S + ButtonW("Daj") + 6 * S, by), false)) { T.editSet = s; snprintf(setName, sizeof setName, "%s", s.name.c_str()); }
        if (IconButton("##del", ICN_TRASH, "Usuń zestaw", ImVec2(x + cw - 18 * S - BtnH(), by), BtnH(), C_ST_CRASH)) del = idx[k];
        ImGui::PopID();
    }
    ImGui::SetCursorScreenPos(ImVec2(p0.x, p0.y + ((idx.size() + 1) / 2) * (ch + gap)));
    ImGui::Dummy(ImVec2(w, 0));
    if (del >= 0) { T.sets.erase(T.sets.begin() + del); T.SaveSets(); }
}
const char* GroupIcon(int g) {
    switch (g) {
    case NG_ANIMALS: return ICN_PAW;
    case NG_BEASTS: return "\xef\x86\x88";   // bug
    case NG_UNDEAD: return ICN_GHOST;
    case NG_ORCS: return ICN_GAVEL;
    case NG_MAGIC: return ICN_MAGIC;
    case NG_MAGES: return ICN_FLASK;
    default: return ICN_USER;
    }
}
// ---- Postacie / Spawner
void NpcSpawner(const Snapshot& snap, float w) {
    if (!g_cat.built) { ImGui::TextColored(C_DIM, "Wczytaj grę, żeby zobaczyć listę."); return; }
    // build 9 (#18): everything spawned in this world - preview, go to, bring, remove (copies only)
    if (!snap.spawned.empty()) {
        SectionLabel("Zrespione");
        float h = 54 * S;
        int i = 0;
        for (auto& n : snap.spawned) {
            ImVec2 p = ImGui::GetCursorScreenPos();
            DL()->AddRectFilled(p, ImVec2(p.x + w, p.y + h), U32(i & 1 ? C_CARD : C_CARD2), R_ITEM());
            ImGui::PushID((int)(n.ptr & 0x7FFFFFFF) + 77);
            Avatar(ImVec2(p.x + 12 * S + 20 * S, p.y + h * 0.5f), 21 * S, n.monster, n.ptr, n.inst);
            float bs = 30 * S, bx = p.x + w - 12 * S - bs;
            if (IconButton("##rm", ICN_TRASH, "Usuń tę postać", ImVec2(bx, p.y + (h - bs) * 0.5f), bs, C_TEXT, false, &C_G_ATK)) Act_RemoveSpawned(n.ptr);
            bx -= bs + 6 * S;
            if (IconButton("##go", ICN_SIGNIN, "Teleportuj mnie do niej", ImVec2(bx, p.y + (h - bs) * 0.5f), bs, C_TEXT, false, &C_G_MOVE)) Act_PartyCmd(n.ptr, 4);
            bx -= bs + 6 * S;
            if (IconButton("##sv", ICN_BOOKMARK, "Zapisz jako cel", ImVec2(bx, p.y + (h - bs) * 0.5f), bs, C_TEXT, false, &C_G_CTRL)) Act_SaveTargetPtr(n.ptr);
            TextClip(ImVec2(p.x + 62 * S, p.y + 9 * S), fBold, n.dead ? C_ST_CRASH : C_TEXT, n.name + (n.dead ? "  (martwy)" : ""), bx - 10 * S);
            char b[200]; snprintf(b, sizeof b, "%s  ·  %.0f m  ·  %d/%d HP", n.inst.c_str(), n.dist, n.hp, n.hpmax);
            TextClip(ImVec2(p.x + 62 * S, p.y + 30 * S), fSmall, C_DIM, b, bx - 10 * S);
            ImGui::PopID();
            ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + 4 * S)); ImGui::Dummy(ImVec2(w, 0));
            i++;
        }
        if (Button("##rmall", ICN_TRASH "  Usuń wszystkie zrespione", ImGui::GetCursorScreenPos(), false)) Act_RemoveSpawned(0);
        ImGui::Dummy(ImVec2(0, 4 * S));
        SectionLabel("Lista postaci");
    }
    BeginPanel("##nspf", nullptr, w);
    InputBox("##nsearch", search, sizeof search, "Szukaj: Gomez, troll, ORCWARRIOR...", w - 36 * S - TextW("Ile", fReg) - 80 * S - 3 * ImGui::GetStyle().ItemSpacing.x);
    ImGui::SameLine(); ImGui::AlignTextToFramePadding(); ImGui::TextColored(C_DIM, "Ile"); ImGui::SameLine();
    NumField("##cnt", &T.spawnCount, 1, 30, 80 * S);
    ImGui::AlignTextToFramePadding(); ImGui::TextColored(C_DIM, "Tryb:"); ImGui::SameLine();
    const char* modes[] = { "Wrogi (od razu atakuje)", "Sojusznik", "Zamrożony", "Neutralny" };
    for (int m = 0; m < 4; m++) if (Chip(modes[m], T.spawnMode == m)) T.spawnMode = m;
    ImGui::NewLine();
    ImGui::AlignTextToFramePadding(); ImGui::TextColored(C_DIM, "Gdy postać już jest w grze:"); ImGui::SameLine();
    if (Chip("Przenieś oryginał##sc0", T.spawnClone == 0)) T.spawnClone = 0;
    if (Chip("Utwórz kopię##sc1", T.spawnClone == 1)) T.spawnClone = 1;
    ImGui::NewLine();
    if (Chip("Wszyscy##g", npcGroup < 0)) npcGroup = -1;
    for (int g = 0; g < NG_COUNT; g++) if (Chip(kNpcGroupNames[g], npcGroup == g, GroupIcon(g))) npcGroup = g;
    EndPanel();
    std::vector<const NpcEntry*> v;
    for (auto& e : g_cat.npcs) if ((npcGroup < 0 || e.group == npcGroup) && MatchQ(search, e.name, e.id)) v.push_back(&e);
    PF(fSmall); ImGui::TextColored(C_DIM, "%d instancji", (int)v.size()); ImGui::PopFont();
    if (v.empty() && search[0]) {   // not in the list (#19): try the script instance by name, like the game console does
        std::string id = search; for (auto& ch : id) ch = (char)toupper((unsigned char)ch);
        std::string lbl = "Spróbuj zrespić instancję " + id;
        if (Button("##raw", lbl.c_str(), ImGui::GetCursorScreenPos())) Act_SpawnByName(id, T.spawnCount, T.spawnMode);
        ImGui::Dummy(ImVec2(0, 2 * S));
        PF(fSmall); ImGui::TextColored(C_FAINT, "Jeśli to zadziała, zgłoś brakującą postać - dodam ją do listy."); ImGui::PopFont();
    }
    float h = 50 * S;
    ImGuiListClipper clip; clip.Begin((int)v.size(), h + 4 * S);
    while (clip.Step())
        for (int i = clip.DisplayStart; i < clip.DisplayEnd; i++) {
            const NpcEntry& e = *v[i];
            ImVec2 p = ImGui::GetCursorScreenPos();
            ListRowBg(p, w, h, i);
            float ic = 34 * S;
            DL()->AddRectFilled(ImVec2(p.x + 9 * S, p.y + (h - ic) * 0.5f), ImVec2(p.x + 9 * S + ic, p.y + (h + ic) * 0.5f), U32(C_ACCD, 0.7f), R_BTN());
            if (void* tex = PortraitFor(e.id)) DL()->AddImage((ImTextureID)(intptr_t)tex, ImVec2(p.x + 9 * S, p.y + (h - ic) * 0.5f), ImVec2(p.x + 9 * S + ic, p.y + (h + ic) * 0.5f));
            else IconAt(DL(), ImVec2(p.x + 9 * S + ic * 0.5f, p.y + h * 0.5f), GroupIcon(e.group), U32(C_ACC), 14 * S);
            float x = p.x + 54 * S;
            if (T.Crashed("npc:" + e.id)) { float pw; Pill(ImVec2(x, p.y + 7 * S), "CRASH", C_ST_CRASH, &pw); x += pw + 6 * S; }
            ImGui::PushID(e.sym);
            float bw = ButtonW("Spawn");
            if (Button("##sp", "Spawn", ImVec2(p.x + w - bw - 10 * S, p.y + (h - BtnH()) * 0.5f))) Act_SpawnNpc(e.sym, T.spawnCount, T.spawnMode);
            ImGui::PopID();
            Text(ImVec2(x, p.y + 8 * S), fBold, C_TEXT, e.name.c_str());
            char sub[220]; snprintf(sub, sizeof sub, "%s  ·  %s  ·  poziom %d  ·  %s", kNpcGroupNames[e.group], GuildName(e.guild).c_str(), e.level, e.id.c_str());
            Text(ImVec2(p.x + 54 * S, p.y + 28 * S), fSmall, C_DIM, sub);
            ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + 4 * S)); ImGui::Dummy(ImVec2(w, 0));
        }
}
// ---- Postacie / Cel
struct ActGroup { const char* label; const ImVec4* col; std::vector<const char*> ids; };
void TargetPage(const Snapshot& s, float w) {
    static const std::vector<ActGroup> groups = {
        { "Kontrola", &C_G_CTRL, { "swap", "mind" } },
        { "Przemieszczanie", &C_G_MOVE, { "tpull", "tpto", "yeet" } },
        { "Atak", &C_G_ATK, { "tkill", "tcast", "tfreeze" } },
        { "Inne", &C_G_OTHER, { "trevive", "trob", "tphoto", "tremove" } },
    };
    ImVec2 p = ImGui::GetCursorScreenPos();
    // build 9 (#12): actions on the right, one group per line (room for more actions later)
    float bs = 34 * S, gap = 6 * S, rowH = bs + 12 * S;
    float labW = 0; size_t maxN = 0;
    for (auto& g : groups) { labW = std::max(labW, TextW(g.label, fSmall)); maxN = std::max(maxN, g.ids.size()); }
    labW += 14 * S;
    float gw = labW + maxN * (bs + gap) - gap;
    float h = std::max(200 * S, 20 * S + groups.size() * rowH + BtnH() + 30 * S);
    ImU32 l = U32(Mix(C_CARD, C_ACC, 0.18f)), r = U32(C_CARD);
    DL()->AddRectFilled(p, ImVec2(p.x + w, p.y + h), r, R_CARD());
    DL()->AddRectFilledMultiColor(ImVec2(p.x + R_CARD(), p.y), ImVec2(p.x + w * 0.5f, p.y + h), l, r, r, l);
    DL()->AddRectFilled(p, ImVec2(p.x + R_CARD() + 1, p.y + h), l, R_CARD(), ImDrawFlags_RoundTopLeft | ImDrawFlags_RoundBottomLeft);
    // left: avatar + info
    float ar = 52 * S;
    ImVec2 ac(p.x + 22 * S + ar, p.y + 22 * S + ar);
    Avatar(ac, ar, s.tMonster, s.tValid ? s.tPtr : 0, s.tValid ? s.tInst : std::string());
    float ix = p.x + 44 * S + ar * 2;
    float gx = p.x + w - 20 * S - gw, gy = p.y + 18 * S;
    float infoMax = gx - 16 * S;
    if (!s.tValid) {
        Text(ImVec2(ix, p.y + 24 * S), fBig, C_TEXT, "Brak celu");
        TextClip(ImVec2(ix, p.y + 56 * S), fSmall, C_DIM, "Spójrz na postać albo wybierz zapisany cel.", infoMax);
    } else {
        PF(fBig); float nw = ImGui::CalcTextSize(s.tName.c_str()).x; ImGui::PopFont();
        TextClip(ImVec2(ix, p.y + 20 * S), fBig, C_TEXT, s.tName, infoMax);
        float pw = 0;
        if (ix + nw + 130 * S < infoMax) {
            Pill(ImVec2(ix + nw + 10 * S, p.y + 24 * S), s.tSaved ? "ZAPISANY" : "POD WZROKIEM", s.tSaved ? C_ON : C_DIM, &pw);
            if (s.tDead) Pill(ImVec2(ix + nw + 16 * S + pw, p.y + 24 * S), "MARTWY", C_ST_CRASH);
        }
        TextClip(ImVec2(ix, p.y + 52 * S), fSmall, C_ACC, s.tInst, infoMax);
        char b[260];
        snprintf(b, sizeof b, "%s  ·  %s  ·  poziom %d", s.tGroup.c_str(), s.tGuild.c_str(), s.tLevel);
        TextClip(ImVec2(ix, p.y + 72 * S), fReg, C_DIM, b, infoMax);
        TextClip(ImVec2(ix, p.y + 94 * S), fSmall, C_DIM, s.tPlace, infoMax);
        snprintf(b, sizeof b, "X %.0f   Y %.0f   Z %.0f", s.tx, s.ty, s.tz);
        TextClip(ImVec2(ix, p.y + 112 * S), fSmall, C_FAINT, b, infoMax);
        float bx1 = ix + std::max(60 * S, std::min(240 * S, infoMax - ix - 90 * S));
        HpBar(ix, bx1, p.y + 142 * S, s.tHp, s.tHpMax, C_HP, 6);
        snprintf(b, sizeof b, "%d / %d HP", s.tHp, s.tHpMax);
        Text(ImVec2(bx1 + 10 * S, p.y + 136 * S), fSmall, C_TEXT, b);
    }
    DL()->AddLine(ImVec2(gx - 12 * S, p.y + 16 * S), ImVec2(gx - 12 * S, p.y + 16 * S + groups.size() * rowH - 12 * S), U32(C_LINE));
    for (auto& g : groups) {
        PF(fSmall); float lh = ImGui::GetTextLineHeight(); ImGui::PopFont();
        Text(ImVec2(gx, gy + (bs - lh) * 0.5f), fSmall, *g.col, g.label);
        for (size_t k = 0; k < g.ids.size(); k++) {
            Row* rw = T.Get(g.ids[k]);
            if (!rw) continue;
            ImGui::PushID(g.ids[k]);
            ImVec2 bp(gx + labW + k * (bs + gap), gy);
            std::string tip = rw->name + (rw->note.empty() ? "" : "\n" + rw->note) + (rw->hotkey ? "\nSkrót: " + KeyName(rw->hotkey) : "");
            // build 13: actions follow the target's state (more of these later)
            if (rw->id == "mind" && s.tValid && s.tAlly) {
                if (IconButton("##ta", "\xef\x88\xb5", "Wyrzuć z drużyny", bp, bs, C_TEXT, false, &C_G_ATK)) Act_PartyCmd(s.tPtr, 5);
            } else if (IconButton("##ta", rw->icon.c_str(), tip.c_str(), bp, bs, C_TEXT, false, g.col)) ApplyRow(*rw);
            ImGui::PopID();
        }
        gy += rowH;
    }
    // buttons: one row, bottom right
    float by = p.y + h - BtnH() - 18 * S;
    float bw1 = ButtonW("Zapisz cel pod wzrokiem"), bw2 = ButtonW("Użyj NPC pod wzrokiem");
    float bx = p.x + w - 20 * S - bw1;
    if (Button("##tsave", "Zapisz cel pod wzrokiem", ImVec2(bx, by))) { if (Row* rw = T.Get("tsave")) ApplyRow(*rw); }
    float bx2 = bx;
    if (s.activeSaved >= 0) { bx2 -= bw2 + 8 * S; if (Button("##tw", "Użyj NPC pod wzrokiem", ImVec2(bx2, by), false)) { if (Row* rw = T.Get("tforget")) ApplyRow(*rw); } }
    if (s.tValid) {
        const char* el = "Edytuj w Postać";
        bx2 -= ButtonW(el) + 8 * S;
        if (Button("##editp", el, ImVec2(bx2, by), false)) { if (Row* r = T.Get("subject")) { r->val = 2; T.Save(); } curSec = 0; curSub = 0; cfgPage = false; testsPage = false; }
    }
    if (s.tValid && s.tInst2 == "1") Pill(ImVec2(ix, by + 6 * S), "ZAMROŻONY", C_G_MOVE);
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + 12 * S)); ImGui::Dummy(ImVec2(w, 0));

    if (!s.saved.empty()) {
        SectionLabel("Zapisane cele");
        // build 9 (#13): a list (one target per line) with the same details as the old tiles
        float ch = 64 * S;
        for (size_t k = 0; k < s.saved.size(); k++) {
            const SavedInfo& si = s.saved[k];
            ImVec2 rp = ImGui::GetCursorScreenPos();
            float x = rp.x, y = rp.y;
            bool act = (int)k == s.activeSaved;
            ImGui::PushID((int)k + 6000);
            float bs = 30 * S;
            if (ImGui::InvisibleButton("##sel", ImVec2(w - bs - 24 * S, ch))) Act_SelectSavedPtr(si.ptr);
            bool hov = ImGui::IsItemHovered();
            DL()->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + ch), U32(act ? Mix(C_CARD, C_ACC, 0.14f) : hov ? C_HOVER : C_CARD), R_ITEM());
            if (act) DL()->AddRectFilled(ImVec2(x, y + 10 * S), ImVec2(x + 3 * S, y + ch - 10 * S), U32(C_ACC), 1.5f * S);
            Avatar(ImVec2(x + 14 * S + 22 * S, y + ch * 0.5f), 22 * S, si.monster, si.ptr, si.inst);
            float tx = x + 70 * S;
            float c2 = x + w * 0.46f, c3 = x + w - bs - 36 * S - 170 * S;
            TextClip(ImVec2(tx, y + 11 * S), fBold, si.dead ? C_ST_CRASH : C_TEXT, si.name + (si.dead ? "  (martwy)" : ""), c2 - 10 * S);
            TextClip(ImVec2(tx, y + 34 * S), fSmall, C_ACC, si.inst, c2 - 10 * S);
            char b[220];
            snprintf(b, sizeof b, "%s  ·  %s  ·  poziom %d", si.group.c_str(), si.guild.c_str(), si.level);
            TextClip(ImVec2(c2, y + 12 * S), fSmall, C_DIM, b, c3 - 10 * S);
            snprintf(b, sizeof b, "%s  ·  %.0f m", si.place.c_str(), si.dist);
            TextClip(ImVec2(c2, y + 34 * S), fSmall, C_FAINT, b, c3 - 10 * S);
            HpBar(c3, c3 + 110 * S, y + ch * 0.5f, si.hp, si.hpmax, C_HP, 5);
            snprintf(b, sizeof b, "%d/%d", si.hp, si.hpmax);
            Text(ImVec2(c3 + 118 * S, y + ch * 0.5f - 9 * S), fSmall, C_TEXT, b);
            if (hov) {
                char xyz[96]; snprintf(xyz, sizeof xyz, "%.0f / %.0f / %.0f", si.x, si.y, si.z);
                TipKV(si.name, { { "Instancja", si.inst }, { "Miejsce", si.place }, { "X / Y / Z", xyz } }, &C_ACC);
            }
            if (IconButton("##rm", ICN_TIMES, "Usuń z listy", ImVec2(x + w - 12 * S - bs, y + (ch - bs) * 0.5f), bs, C_DIM)) Act_RemoveSavedTarget((int)k);
            ImGui::PopID();
            ImGui::SetCursorScreenPos(ImVec2(rp.x, rp.y + ch + 6 * S)); ImGui::Dummy(ImVec2(w, 0));
        }
    }
}
// ---- Postacie / Drużyna
void PartyPage(const Snapshot& s, float w) {
    SectionLabel("W drużynie");
    if (s.party.empty()) { PF(fSmall); ImGui::TextColored(C_DIM, "Nikogo - wybierz postacie z listy poniżej albo użyj kontroli umysłu na celu."); ImGui::PopFont(); }
    static const char* stName[] = { "IDZIE ZA TOBĄ", "CZEKA", "NA POSTERUNKU" };
    static const ImVec4* stCol[] = { &C_ON, &C_ST_BUG, &C_DIM };
    struct Cmd { int id; const char* icon; const char* tip; const ImVec4* col; };
    static const Cmd cmds[] = { { 0, ICN_USERS, "Idź za mną", &C_G_OTHER }, { 1, ICN_PAUSE, "Czekaj tutaj", &C_ST_BUG }, { 2, ICN_FLAG, "Wróć na posterunek", &C_DIM },
                                { 3, ICN_MAGNET, "Przywołaj do mnie", &C_G_MOVE }, { 4, ICN_SIGNIN, "Teleportuj mnie do niego", &C_G_MOVE }, { 5, ICN_TIMES, "Wyrzuć z drużyny", &C_G_ATK },
                                { 110, ICN_CAMERA, "Zrób portret", &C_G_OTHER }, { 109, ICN_TRASH, "Usuń ze świata", &C_G_ATK } };
    const int NC = (int)(sizeof cmds / sizeof cmds[0]);
    float h = 58 * S;
    int i = 0;
    for (auto& n : s.party) {
        ImVec2 p = ImGui::GetCursorScreenPos();
        DL()->AddRectFilled(p, ImVec2(p.x + w, p.y + h), U32(C_CARD), R_ITEM());
        ImGui::PushID((int)(n.ptr & 0x7FFFFFFF));
        Avatar(ImVec2(p.x + 12 * S + 20 * S, p.y + h * 0.5f), 24 * S, n.monster, n.ptr, n.inst);
        float tx = p.x + 64 * S;
        float bs = 32 * S, bx = p.x + w - 12 * S - NC * (bs + 6 * S) + 6 * S;
        TextClip(ImVec2(tx, p.y + 10 * S), fBold, n.dead ? C_ST_CRASH : C_TEXT, n.name, bx - 140 * S);
        char b[200]; snprintf(b, sizeof b, "%s  ·  %.0f m  ·  %d/%d HP", n.guild.c_str(), n.dist, n.hp, n.hpmax);
        TextClip(ImVec2(tx, p.y + 31 * S), fSmall, C_DIM, b, bx - 140 * S);
        float pw; PF(fSmall); float sw = ImGui::CalcTextSize(stName[n.pstate % 3]).x + 14 * S; ImGui::PopFont();
        Pill(ImVec2(bx - sw - 12 * S, p.y + (h - 22 * S) * 0.5f), stName[n.pstate % 3], *stCol[n.pstate % 3], &pw);
        for (int k = 0; k < NC; k++) {
            ImGui::PushID(k);
            if (IconButton("##pc", cmds[k].icon, cmds[k].tip, ImVec2(bx + k * (bs + 6 * S), p.y + (h - bs) * 0.5f), bs, C_TEXT, false, cmds[k].col)) {
                if (cmds[k].id >= 100) Act_EnvCmd(n.ptr, cmds[k].id - 100); else Act_PartyCmd(n.ptr, cmds[k].id);
            }
            ImGui::PopID();
        }
        ImGui::PopID();
        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + 6 * S)); ImGui::Dummy(ImVec2(w, 0));
        i++;
    }
    SectionLabel("Do zwerbowania");
    BeginPanel("##allyp", nullptr, w);
    InputBox("##asearch", allySearch, sizeof allySearch, "Szukaj po imieniu, gildii albo ID...", w * 0.40f);
    ImGui::SameLine();
    const char* fl[] = { "Wszyscy", "Ludzie", "Potwory", "Wrodzy", "Nie-wrodzy" };
    for (int k = 0; k < 5; k++) if (Chip(fl[k], allyFilter == k)) allyFilter = k;
    ImGui::NewLine();
    auto pass = [&](const NearNpc& n) {
        if (n.dead) return false;
        if (allyFilter == 1 && n.monster) return false;
        if (allyFilter == 2 && !n.monster) return false;
        if (allyFilter == 3 && !n.hostile) return false;
        if (allyFilter == 4 && n.hostile) return false;
        return MatchQ(allySearch, n.name, n.guild) || MatchQ(allySearch, n.inst, n.inst);
    };
    int vis = 0; for (auto& n : s.nearby) if (pass(n)) vis++;
    ImVec2 bp = ImGui::GetCursorScreenPos();
    if (Button("##all", "Zaznacz widocznych", bp, false)) for (auto& n : s.nearby) if (pass(n)) allyPick.insert(n.ptr);
    float x2 = bp.x + ButtonW("Zaznacz widocznych") + 8 * S;
    if (Button("##none", "Odznacz", ImVec2(x2, bp.y), false)) allyPick.clear();
    x2 += ButtonW("Odznacz") + 8 * S;
    char lbl[64]; snprintf(lbl, sizeof lbl, "Dodaj do drużyny (%d)", (int)allyPick.size());
    if (Button("##add", lbl, ImVec2(x2, bp.y)) && !allyPick.empty()) { Act_MakeAllies(std::vector<uintptr_t>(allyPick.begin(), allyPick.end())); allyPick.clear(); }
    ImGui::SetCursorScreenPos(ImVec2(bp.x, bp.y + BtnH() + 8 * S));
    PF(fSmall); ImGui::TextColored(C_DIM, "%d postaci w promieniu (ustawisz w karcie Zachowanie drużyny), %d pasuje do filtra", (int)s.nearby.size(), vis); ImGui::PopFont();
    EndPanel();
    float rh = 46 * S; i = 0;
    for (auto& n : s.nearby) {
        if (!pass(n)) continue;
        ImVec2 p = ImGui::GetCursorScreenPos();
        ListRowBg(p, w, rh, i++);
        ImGui::PushID((int)(n.ptr & 0x7FFFFFFF));
        bool on = allyPick.count(n.ptr) != 0;
        ImGui::SetCursorScreenPos(p);
        if (ImGui::InvisibleButton("##row", ImVec2(w, rh))) { if (on) allyPick.erase(n.ptr); else allyPick.insert(n.ptr); }
        Check("##c", on, ImVec2(p.x + 14 * S, p.y + (rh - 18 * S) * 0.5f));
        float tx = p.x + 46 * S;
        Text(ImVec2(tx, p.y + 6 * S), fBold, n.hostile ? C_ST_CRASH : C_TEXT, n.name.c_str());
        char b[220]; snprintf(b, sizeof b, "%s  ·  %s  ·  poziom %d  ·  %.0f m  ·  %s", kNpcGroupNames[n.group], n.guild.c_str(), n.level, n.dist, n.inst.c_str());
        TextClip(ImVec2(tx, p.y + 26 * S), fSmall, C_DIM, b, p.x + w - 12 * S);
        ImGui::PopID();
        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + rh + 4 * S)); ImGui::Dummy(ImVec2(w, 0));
    }
}
// ---- Postacie / Otoczenie (build 14): everybody around you, with the Cel actions on each of them
int envFilter = 0; char envSearch[64] = "";
void EnvPage(const Snapshot& s, float w) {
    T.envWanted = true;
    BeginPanel("##envp", nullptr, w);
    InputBox("##esearch", envSearch, sizeof envSearch, "Szukaj po imieniu, gildii albo ID...", w * 0.36f);
    ImGui::SameLine();
    const char* fl[] = { "Wszyscy", "Ludzie", "Potwory", "Wrodzy", "Drużyna", "Zrespione", "Martwi" };
    for (int k = 0; k < 7; k++) if (Chip(fl[k], envFilter == k)) envFilter = k;
    ImGui::NewLine();
    auto pass = [&](const NearNpc& n) {
        if (envFilter != 6 && n.dead) return false;
        if (envFilter == 1 && n.monster) return false;
        if (envFilter == 2 && !n.monster) return false;
        if (envFilter == 3 && !n.hostile) return false;
        if (envFilter == 4 && !n.ally) return false;
        if (envFilter == 5 && n.kind != "Zrespiony" && n.kind != "Arena") return false;
        if (envFilter == 6 && !n.dead) return false;
        return MatchQ(envSearch, n.name, n.guild) || MatchQ(envSearch, n.inst, n.kind);
    };
    int vis = 0; for (auto& n : s.env) if (pass(n)) vis++;
    PF(fSmall); ImGui::TextColored(C_DIM, "%d postaci w promieniu %.0f m (zmienisz w karcie Otoczenie), %d pasuje do filtra. Najedź na ikonę, żeby zobaczyć akcję.",
                                   (int)s.env.size(), gx::Val("envradius"), vis); ImGui::PopFont();
    EndPanel();
    struct Act { int cmd; const char* icon; const char* tip; const ImVec4* col; };
    float h = 58 * S, bs = 30 * S;
    int i = 0;
    for (auto& n : s.env) {
        if (!pass(n)) continue;
        ImVec2 p = ImGui::GetCursorScreenPos();
        ListRowBg(p, w, h, i++);
        ImGui::PushID((int)(n.ptr & 0x7FFFFFFF));
        Avatar(ImVec2(p.x + 12 * S + 20 * S, p.y + h * 0.5f), 22 * S, n.monster, n.ptr, n.inst);
        std::vector<Act> acts = { { 0, ICN_BOOKMARK, "Ustaw jako cel (lista w zakładce Cel)", &C_ACC }, { 1, ICN_SIGNIN, "Teleportuj mnie do niego", &C_G_MOVE },
                                  { 2, ICN_MAGNET, "Przyciągnij do mnie", &C_G_MOVE } };
        if (!n.dead) {
            acts.push_back(n.ally ? Act{ 3, "\xef\x88\xb5", "Wyrzuć z drużyny", &C_G_ATK } : Act{ 3, "\xef\x88\xb4", "Dodaj do drużyny", &C_G_OTHER });
            acts.push_back({ 4, ICN_HEART, "Uspokój (neutralny wobec ciebie)", &C_G_OTHER });
            acts.push_back({ 5, ICN_CROSSHAIR, "Niech cię zaatakuje", &C_G_ATK });
            acts.push_back({ 6, ICN_SNOW, n.frozen ? "Odmroź" : "Zamroź", &C_G_OTHER });
            acts.push_back({ 7, ICN_KILL, "Zabij", &C_G_ATK });
        } else acts.push_back({ 8, ICN_REFRESH, "Ożyw", &C_G_OTHER });
        acts.push_back({ 10, ICN_CAMERA, "Zrób portret", &C_G_OTHER });
        acts.push_back({ 9, ICN_TRASH, "Usuń ze świata", &C_G_ATK });
        float bx = p.x + w - 12 * S - acts.size() * (bs + 5 * S) + 5 * S;
        float tx = p.x + 62 * S;
        float infoR = bx - 130 * S;
        std::string nm = n.name + (n.dead ? "  (martwy)" : "") + (n.frozen ? "  (zamrożony)" : "");
        TextClip(ImVec2(tx, p.y + 9 * S), fBold, n.dead ? C_DIM : n.ally ? C_ON : n.hostile ? C_ST_CRASH : C_TEXT, nm, infoR);
        char b[260];
        snprintf(b, sizeof b, "%s%s%s  ·  %s  ·  poziom %d  ·  %.0f m  ·  %s", n.kind.empty() ? "" : "[", n.kind.c_str(), n.kind.empty() ? "" : "]  ·  ",
                 n.guild.c_str(), n.level, n.dist, n.inst.c_str());
        TextClip(ImVec2(tx, p.y + 32 * S), fSmall, C_DIM, b, infoR);
        HpBar(infoR + 10 * S, bx - 16 * S, p.y + h * 0.5f, n.hp, n.hpmax, C_HP, 5);
        if (ImGui::IsMouseHoveringRect(ImVec2(infoR, p.y), ImVec2(bx - 10 * S, p.y + h))) Tip("%d / %d HP", n.hp, n.hpmax);
        for (size_t k = 0; k < acts.size(); k++) {
            ImGui::PushID((int)k);
            if (IconButton("##ea", acts[k].icon, acts[k].tip, ImVec2(bx + k * (bs + 5 * S), p.y + (h - bs) * 0.5f), bs, C_TEXT, false, acts[k].col)) Act_EnvCmd(n.ptr, acts[k].cmd);
            ImGui::PopID();
        }
        ImGui::PopID();
        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + 4 * S)); ImGui::Dummy(ImVec2(w, 0));
    }
    if (vis == 0) { PF(fSmall); ImGui::TextColored(C_DIM, "Nikogo w pobliżu dla tego filtra."); ImGui::PopFont(); }
}
// ---- Ekwipunek / Zbieranie (build 15): every item lying in the world - coordinates, teleport, pick-up bot
int mapCat = -1; char mapSearch[64] = ""; int mapMinVal = 0; std::set<uintptr_t> mapPick; bool mapByValue = false;
void MapItemsPage(const Snapshot& s, float w) {
    T.mapWanted = true;
    SectionLabel("Przedmioty na mapie");
    static const std::vector<MapItem> none;
    const std::vector<MapItem>& all = s.mapItems ? *s.mapItems : none;
    BeginPanel("##mapp", nullptr, w);
    InputBox("##msearch", mapSearch, sizeof mapSearch, "Szukaj po nazwie albo ID...", w * 0.34f);
    ImGui::SameLine();
    if (Chip("Wszystkie", mapCat == -1)) mapCat = -1;
    for (int c = 0; c <= IC_COUNT; c++) { ImGui::PushID(c); if (Chip(c == IC_COUNT ? "Ruda" : kItemCatNames[c], mapCat == c)) mapCat = c; ImGui::PopID(); }
    ImGui::NewLine();
    if (Chip("Najbliższe", !mapByValue)) mapByValue = false;
    if (Chip("Najcenniejsze", mapByValue)) mapByValue = true;
    ImGui::NewLine();
    std::vector<const MapItem*> v;
    for (auto& m : all) {
        if (mapCat >= 0 && m.cat != mapCat) continue;
        if (m.value < mapMinVal) continue;
        if (!MatchQ(mapSearch, m.name, m.id)) continue;
        v.push_back(&m);
    }
    if (mapByValue) std::stable_sort(v.begin(), v.end(), [](const MapItem* a, const MapItem* b) { return a->value > b->value; });
    ImVec2 bp = ImGui::GetCursorScreenPos(); float x2 = bp.x;
    auto btn = [&](const char* id, const char* l, bool prim) { bool r = Button(id, l, ImVec2(x2, bp.y), prim); x2 += ButtonW(l) + 8 * S; return r; };
    if (btn("##mall", "Zaznacz widoczne", false)) for (auto* m : v) mapPick.insert(m->ptr);
    if (btn("##mnone", "Odznacz", false)) mapPick.clear();
    char l1[64]; snprintf(l1, sizeof l1, "Bot: zbierz zaznaczone (%d)", (int)mapPick.size());
    if (!s.botOn) {
        if (btn("##mbot", l1, true) && !mapPick.empty()) { Act_BotStart(std::vector<uintptr_t>(mapPick.begin(), mapPick.end())); mapPick.clear(); }
        char l2[64]; snprintf(l2, sizeof l2, "Bot: zbierz widoczne (%d)", (int)v.size());
        if (btn("##mbotv", l2, false) && !v.empty()) { std::vector<uintptr_t> p; for (auto* m : v) p.push_back(m->ptr); Act_BotStart(p); }
    } else if (btn("##mstop", "Zatrzymaj bota", true)) Act_BotStop();
    if (btn("##mcsv", "Zapisz listę do pliku (CSV)", false)) {
        std::string path = DataPath("G1CrazyTrainer_przedmioty.csv");
        if (FILE* f = fopen(path.c_str(), "wb")) {
            fprintf(f, "\xEF\xBB\xBFnazwa;instancja;kategoria;wartosc;ilosc;x;y;z\r\n");
            for (auto* m : v) fprintf(f, "%s;%s;%s;%d;%d;%.0f;%.0f;%.0f\r\n", m->name.c_str(), m->id.c_str(), m->cat == IC_COUNT ? "Ruda" : kItemCatNames[m->cat], m->value, m->amount, m->x, m->y, m->z);
            fclose(f); T.Toast("Zapisano " + std::to_string(v.size()) + " pozycji: G1CrazyTrainer_przedmioty.csv");
        }
    }
    ImGui::SetCursorScreenPos(ImVec2(bp.x, bp.y + BtnH() + 8 * S));
    PF(fSmall);
    if (s.botOn) ImGui::TextColored(C_ON, "Bot zbiera: zostało %d, zebrano %d  ·  Esc w grze przerywa", s.botLeft, s.botGot);
    else ImGui::TextColored(C_DIM, "%d przedmiotów w świecie, %d pasuje  ·  bot teleportuje cię do każdego po kolei (najkrótsza trasa) i podnosi go", (int)all.size(), (int)v.size());
    ImGui::PopFont();
    EndPanel();
    float h = 46 * S;
    ImGuiListClipper clip; clip.Begin((int)v.size(), h + 4 * S);
    while (clip.Step())
        for (int k = clip.DisplayStart; k < clip.DisplayEnd; k++) {
            const MapItem& m = *v[k];
            ImVec2 p = ImGui::GetCursorScreenPos();
            ListRowBg(p, w, h, k);
            ImGui::PushID((int)(m.ptr & 0x7FFFFFFF));
            bool on = mapPick.count(m.ptr) != 0;
            if (Check("##c", on, ImVec2(p.x + 12 * S, p.y + (h - 18 * S) * 0.5f))) { if (on) mapPick.erase(m.ptr); else mapPick.insert(m.ptr); }
            float ic = 34 * S; ImVec2 ip(p.x + 40 * S, p.y + (h - ic) * 0.5f);
            if (void* tex = PortraitFor("I_" + m.id)) DL()->AddImage((ImTextureID)(intptr_t)tex, ip, ImVec2(ip.x + ic, ip.y + ic));
            else { DL()->AddRectFilled(ip, ImVec2(ip.x + ic, ip.y + ic), U32(C_ACCD, 0.5f), R_BTN()); IconAt(DL(), ImVec2(ip.x + ic * 0.5f, ip.y + ic * 0.5f), CatIcon(m.cat == IC_COUNT ? IC_MISC : m.cat), U32(C_ACC), 12 * S); }
            float tx = ip.x + ic + 12 * S, bs = 30 * S, bx = p.x + w - 12 * S - 2 * (bs + 6 * S) + 6 * S;
            std::string nm = m.name + (m.amount > 1 ? "  x" + std::to_string(m.amount) : "");
            TextClip(ImVec2(tx, p.y + 5 * S), fBold, C_TEXT, nm, bx - 10 * S);
            char b[200]; snprintf(b, sizeof b, "%s  ·  %d rudy  ·  %.0f m  ·  %.0f / %.0f / %.0f  ·  %s", m.cat == IC_COUNT ? "Ruda" : kItemCatNames[m.cat], m.value / 2, m.dist, m.x, m.y, m.z, m.id.c_str());
            TextClip(ImVec2(tx, p.y + 25 * S), fSmall, C_DIM, b, bx - 10 * S);
            if (IconButton("##tp", ICN_SIGNIN, "Teleportuj mnie tutaj", ImVec2(bx, p.y + (h - bs) * 0.5f), bs, C_TEXT, false, &C_G_MOVE)) Act_Teleport(m.x, m.y + 100.f, m.z, m.name);
            if (IconButton("##pk", ICN_BAG, "Podnieś teraz (bez chodzenia)", ImVec2(bx + bs + 6 * S, p.y + (h - bs) * 0.5f), bs, C_TEXT, false, &C_G_OTHER)) Act_PickItem(m.ptr);
            ImGui::PopID();
            ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + 4 * S)); ImGui::Dummy(ImVec2(w, 0));
        }
    clip.End();
}
// ---- spell checklists (part of "Deszcz zaklęć" / "Magia na celu" cards)
float SpellList(float x, float y, float w, bool draw, const char* rowId, const char* const* names, int n, const char* title) {
    float rowH = 32 * S;
    int cols = 2;
    float h = 30 * S + ((n + cols - 1) / cols) * rowH + 6 * S;
    if (!draw) return h;
    Row* m = T.Get(rowId);
    if (!m) return h;
    DL()->AddLine(ImVec2(x + 18 * S, y), ImVec2(x + w - 18 * S, y), U32(C_LINE));
    Text(ImVec2(x + 18 * S, y + 8 * S), fSmall, C_DIM, title);
    int mask = (int)m->val;
    float cw = (w - 36 * S) / cols;
    for (int i = 0; i < n; i++) {
        float cx = x + 18 * S + (i % cols) * cw, cy = y + 30 * S + (i / cols) * rowH;
        ImGui::PushID(rowId); ImGui::PushID(i);
        bool on = (mask >> i) & 1;
        ImGui::SetCursorScreenPos(ImVec2(cx - 6 * S, cy));
        if (ImGui::InvisibleButton("##spr", ImVec2(cw - 4 * S, rowH - 4 * S))) { mask ^= (1 << i); m->val = (float)mask; T.Save(); }
        bool hv = ImGui::IsItemHovered();
        if (hv) DL()->AddRectFilled(ImVec2(cx - 6 * S, cy), ImVec2(cx + cw - 10 * S, cy + rowH - 4 * S), U32(C_HOVER, 0.6f), R_BTN());
        Check("##sp", on, ImVec2(cx, cy + (rowH - 4 * S - 18 * S) * 0.5f));
        TextV(cx + 28 * S, cy + (rowH - 4 * S) * 0.5f, fReg, on ? C_TEXT : C_DIM, names[i]);
        ImGui::PopID(); ImGui::PopID();
    }
    return h;
}
float RainSpells(float x, float y, float w, bool draw) { return SpellList(x, y, w, draw, "rainmask", kRainNames, RAIN_FX_COUNT, "Zaklęcia (losowo z zaznaczonych)"); }
float TargetSpells(float x, float y, float w, bool draw) {
    float h = SpellList(x, y, w, draw, "tspells", kTSpellNames, kTSpellCount, "Zaklęcia na cel (tylko jedna osoba, losowo z zaznaczonych)");
    if (!draw) return h + BtnH() + 12 * S;
    if (Button("##cast", "Rzuć w cel", ImVec2(x + 18 * S, y + h))) Act_CastOnTarget();
    return h + BtnH() + 12 * S;
}
// ---- Świat / Teleport
// icons for waypoint place types (Font Awesome 4 glyphs inside the loaded range)
const char* TagIcon(const std::string& t) {
    static const std::map<std::string, const char*> m = {
        { "Arena", ICN_GAVEL }, { "Bagno", "\xef\x81\x83" }, { "Woda", "\xef\x81\x83" }, { "Brama", ICN_SIGNIN }, { "Chata", ICN_HOME },
        { "Droga", "\xef\x80\x98" }, { "Most", ICN_ARROWS }, { "Dzicz", ICN_MAP }, { "Wzgórza", ICN_MAP }, { "Jaskinia", ICN_GHOST },
        { "Karczma", ICN_CUTLERY }, { "Kopalnia", ICN_DIAMOND }, { "Las", "\xef\x86\xbb" }, { "Mur", ICN_SHIELD }, { "Straż", ICN_SHIELD },
        { "Obóz", ICN_USERS }, { "Orkowie", ICN_KILL }, { "Ziemie orków", ICN_KILL }, { "Potwory", ICN_PAW }, { "Ruiny", ICN_CUBE },
        { "Rzemiosło", "\xef\x82\xad" }, { "Targ", "\xef\x81\xba" }, { "Trening", ICN_CROSSHAIR }, { "Wieża", ICN_MAGIC },
        { "Zamek", "\xef\x8a\x86" }, { "Świątynia", "\xef\x86\x9c" } };
    auto it = m.find(t);
    return it == m.end() ? ICN_MAP : it->second;
}
void TeleportPage(const Snapshot& s, float w) {
    SectionLabel("Twoje pozycje");
    BeginPanel("##tpadd", nullptr, w);
    PF(fSmall); ImGui::TextColored(C_DIM, "Świat: %s   ·   teraz: X %.0f  Y %.0f  Z %.0f", s.world.c_str(), s.px, s.py, s.pz); ImGui::PopFont();
    InputBox("##posname", posName, sizeof posName, "nazwa nowej pozycji (np. Kopalnia, Xardas)...", w - 36 * S - ButtonW("Zapisz obecną") - ImGui::GetStyle().ItemSpacing.x);
    ImGui::SameLine();
    if (Button("##savepos", "Zapisz obecną", ImGui::GetCursorScreenPos())) { Act_SavePos(posName); posName[0] = 0; }
    EndPanel();
    std::vector<SavedPos> pos;
    { std::lock_guard<std::mutex> l(T.mtx); pos = T.positions; }
    if (pos.empty()) { PF(fSmall); ImGui::TextColored(C_DIM, "Brak zapisanych pozycji w tym świecie."); ImGui::PopFont(); }
    else {
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        int cols = w > 720 * S ? 3 : 2;
        float gp = 12 * S, cw = (w - gp * (cols - 1)) / cols, ch = 96 * S;
        int n = (int)pos.size();
        float y = p0.y;
        for (int i = 0; i < n; i++) {
            int col = i % cols;
            if (col == 0 && i > 0) y += ch + gp;
            float x = p0.x + col * (cw + gp);
            ImGui::PushID(i + 9000);
            DL()->AddRectFilled(ImVec2(x, y), ImVec2(x + cw, y + ch), U32(C_CARD), R_CARD());
            if (editPos == i) {
                DL()->AddRect(ImVec2(x, y), ImVec2(x + cw, y + ch), U32(C_ACC, 0.6f), R_CARD(), 1.f * S);
                ImGui::SetCursorScreenPos(ImVec2(x + 12 * S, y + 10 * S));
                InputBox("##pn", editPosName, sizeof editPosName, "nazwa", cw - 24 * S);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, C_CARD2); ImGui::PushStyleColor(ImGuiCol_Text, C_ACC);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, R_BTN());
                float fw = (cw - 24 * S - 2 * 6 * S - 2 * BtnH() - 12 * S) / 3;
                ImGui::SetCursorScreenPos(ImVec2(x + 12 * S, y + ch - BtnH() - 12 * S));
                ImGui::SetNextItemWidth(fw); ImGui::InputFloat("##ex", &editPosV.x, 0, 0, "%.0f"); ImGui::SameLine(0, 6 * S);
                ImGui::SetNextItemWidth(fw); ImGui::InputFloat("##ey", &editPosV.y, 0, 0, "%.0f"); ImGui::SameLine(0, 6 * S);
                ImGui::SetNextItemWidth(fw); ImGui::InputFloat("##ez", &editPosV.z, 0, 0, "%.0f");
                ImGui::PopStyleVar(); ImGui::PopStyleColor(2);
                float bx = x + cw - 12 * S - BtnH();
                if (IconButton("##ok", ICN_CHECK, "Zapisz zmiany", ImVec2(bx, y + ch - BtnH() - 12 * S), BtnH(), C_ON, false, &C_ON)) {
                    editPosV.name = editPosName; Act_UpdatePos(i, editPosV); editPos = -1;
                }
                if (IconButton("##cx", ICN_TIMES, "Anuluj", ImVec2(bx - BtnH() - 6 * S, y + ch - BtnH() - 12 * S), BtnH(), C_DIM)) editPos = -1;
            } else {
                IconAt(DL(), ImVec2(x + 26 * S, y + 24 * S), ICN_MAP, U32(C_ACC), 14 * S);
                TextClip(ImVec2(x + 44 * S, y + 14 * S), fBold, C_TEXT, pos[i].name, x + cw - 14 * S);
                char c[96]; snprintf(c, sizeof c, "X %.0f  Y %.0f  Z %.0f", pos[i].x, pos[i].y, pos[i].z);
                TextClip(ImVec2(x + 44 * S, y + 35 * S), fSmall, C_DIM, c, x + cw - 14 * S);
                float by = y + ch - BtnH() - 12 * S;
                if (Button("##go", "Teleport", ImVec2(x + 14 * S, by))) Act_Teleport(pos[i].x, pos[i].y, pos[i].z, pos[i].name);
                float bx = x + cw - 14 * S - BtnH();
                if (IconButton("##del", ICN_TRASH, "Usuń pozycję", ImVec2(bx, by), BtnH(), C_ST_CRASH)) Act_DeletePos(i);
                if (IconButton("##edit", ICN_PENCIL, "Edytuj nazwę i koordynaty", ImVec2(bx - BtnH() - 6 * S, by), BtnH(), C_TEXT)) {
                    editPos = i; editPosV = pos[i]; snprintf(editPosName, sizeof editPosName, "%s", pos[i].name.c_str());
                }
            }
            ImGui::PopID();
        }
        ImGui::SetCursorScreenPos(ImVec2(p0.x, y + ch + gp));
        ImGui::Dummy(ImVec2(w, 0));
    }
    SectionLabel("Ręczne koordynaty");
    BeginPanel("##tpman", nullptr, w);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, C_CARD2); ImGui::PushStyleColor(ImGuiCol_Text, C_ACC);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8 * S, 7 * S)); ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, R_BTN());
    // fill the row: 3 fields + their labels + 2 buttons (#11)
    float spx = ImGui::GetStyle().ItemSpacing.x, inner = w - 36 * S;
    float lab = TextW("X", fReg) + spx * 2, btns = ButtonW("Obecna") + ButtonW("Teleport") + 8 * S;
    float fw = std::max(60 * S, (inner - btns - lab * 3) / 3);
    ImGui::SetNextItemWidth(fw); ImGui::InputFloat("X##m", &manX, 0, 0, "%.0f"); ImGui::SameLine();
    ImGui::SetNextItemWidth(fw); ImGui::InputFloat("Y##m", &manY, 0, 0, "%.0f"); ImGui::SameLine();
    ImGui::SetNextItemWidth(fw); ImGui::InputFloat("Z##m", &manZ, 0, 0, "%.0f"); ImGui::SameLine();
    ImGui::PopStyleVar(2); ImGui::PopStyleColor(2);
    ImVec2 bp = ImGui::GetCursorScreenPos();
    if (Button("##cur", "Obecna", bp, false)) { manX = s.px; manY = s.py; manZ = s.pz; }
    if (Button("##mgo", "Teleport", ImVec2(bp.x + ButtonW("Obecna") + 8 * S, bp.y))) Act_Teleport(manX, manY, manZ, "ręczne koordynaty");
    EndPanel();
    SectionLabel("Miejsca w świecie");
    if (g_cat.wps.empty()) { ImGui::TextColored(C_DIM, "Brak waypointów (wczytaj grę)."); return; }
    // labels are computed once per world
    static void* builtFor = nullptr; static size_t builtN = 0;
    static std::vector<gx::WpLabel> labels; static std::vector<std::pair<std::string, int>> tags;
    static std::vector<std::string> regions;
    if (builtN != g_cat.wps.size() || builtFor != (void*)g_cat.wps.data()) {
        builtFor = (void*)g_cat.wps.data(); builtN = g_cat.wps.size();
        labels.clear(); std::map<std::string, int> cnt;
        for (auto& e : g_cat.wps) { labels.push_back(gx::DescribeWp(e.name)); for (auto& t : labels.back().tags) cnt[t]++; }
        tags.assign(cnt.begin(), cnt.end());
        std::sort(tags.begin(), tags.end(), [](auto& a, auto& b) { return a.second > b.second; });
        std::set<std::string> rs; for (auto& L : labels) if (!L.region.empty()) rs.insert(L.region);
        regions = { "Wszystkie regiony" };
        for (auto& t : tags) if (rs.count(t.first)) regions.push_back(t.first);
    }
    static std::string wpRegion; static std::set<std::string> wpTags;
    BeginPanel("##wpf", nullptr, w);
    // build 9 (#10): search | region | icon button with place types; chosen types as removable chips below
    float rw = 220 * S, ib = ImGui::GetFontSize() + 14 * S;   // same height as the input and the select (build 10)
    InputBox("##wsearch", search, sizeof search, "Szukaj: opis albo nazwa waypointu (OCR_, PSI_...)", w - 36 * S - rw - ib - spx * 2);
    ImGui::SameLine();
    std::string cr = wpRegion.empty() ? "Wszystkie regiony" : wpRegion;
    if (StrCombo("##wreg", regions, cr, rw)) wpRegion = cr == "Wszystkie regiony" ? "" : cr;
    ImGui::SameLine();
    ImVec2 tp = ImGui::GetCursorScreenPos();
    float iy = tp.y;
    char tt[64]; snprintf(tt, sizeof tt, wpTags.empty() ? "Rodzaj miejsca" : "Rodzaj miejsca (wybrane: %d)", (int)wpTags.size());
    if (IconButton("##wtags", ICN_FILTER, tt, ImVec2(tp.x, iy), ib, C_TEXT, !wpTags.empty())) ImGui::OpenPopup("##wtagpop");
    ImGui::SetCursorScreenPos(ImVec2(tp.x, tp.y + ib));
    PopupStyle();
    ImGui::SetNextWindowSize(ImVec2(560 * S, 0));
    if (ImGui::BeginPopup("##wtagpop")) {
        PF(fBold); ImGui::TextColored(C_TEXT, "Rodzaj miejsca"); ImGui::PopFont();
        PF(fSmall); ImGui::TextColored(C_DIM, wpRegion.empty() ? "dowolny z zaznaczonych" : "dowolny z zaznaczonych  ·  region: %s", wpRegion.c_str()); ImGui::PopFont();
        std::map<std::string, int> inReg;   // counts follow the selected region
        for (auto& L : labels) if (wpRegion.empty() || L.region == wpRegion) for (auto& t : L.tags) inReg[t]++;
        int col = 0;
        for (auto& t0 : tags) {
            if (std::find(regions.begin(), regions.end(), t0.first) != regions.end()) continue;
            auto ci = inReg.find(t0.first);
            int n = ci == inReg.end() ? 0 : ci->second;
            if (n == 0 && !wpTags.count(t0.first)) continue;
            bool on = wpTags.count(t0.first) != 0;
            ImGui::PushID(t0.first.c_str());
            if (Chip((t0.first + "##c").c_str(), on, TagIcon(t0.first))) { if (on) wpTags.erase(t0.first); else wpTags.insert(t0.first); }
            if (ImGui::IsItemHovered()) Tip("%d miejsc", n);
            ImGui::PopID();
            (void)col;
        }
        ImGui::NewLine();
        if (!wpTags.empty() && Button("##wclr", "Wyczyść", ImGui::GetCursorScreenPos(), false)) wpTags.clear();
        ImGui::Dummy(ImVec2(0, 2 * S));
        ImGui::EndPopup();
    }
    PopupStyleEnd();
    if (!wpTags.empty()) {   // chosen types as chips (click = remove)
        std::string rm;
        for (auto& t : wpTags) { ImGui::PushID(t.c_str()); if (Chip((t + "  ×").c_str(), true, TagIcon(t))) rm = t; ImGui::PopID(); }
        if (!rm.empty()) wpTags.erase(rm);
    }
    EndPanel();
    std::vector<int> v;
    for (int i = 0; i < (int)g_cat.wps.size(); i++) {
        const auto& L = labels[i];
        if (!wpRegion.empty() && L.region != wpRegion) continue;
        if (!wpTags.empty()) { bool any = false; for (auto& t : L.tags) if (wpTags.count(t)) any = true; if (!any) continue; }
        if (!(MatchQ(search, g_cat.wps[i].name, L.title) || MatchQ(search, L.region, L.region))) continue;
        v.push_back(i);
    }
    std::stable_sort(v.begin(), v.end(), [&](int a, int b) { return labels[a].region < labels[b].region; });
    PF(fSmall); ImGui::TextColored(C_DIM, "%d miejsc", (int)v.size()); ImGui::PopFont();
    float wh = 48 * S;
    ImGuiListClipper clip; clip.Begin((int)v.size(), wh + 4 * S);
    while (clip.Step())
        for (int k = clip.DisplayStart; k < clip.DisplayEnd; k++) {
            const WpEntry& e = g_cat.wps[v[k]];
            const auto& L = labels[v[k]];
            ImVec2 p = ImGui::GetCursorScreenPos();
            ListRowBg(p, w, wh, k);
            ImGui::PushID(v[k]);
            float bw = ButtonW("Teleport", fReg);
            float bx = p.x + w - bw - 10 * S;
            if (Button("##tp", "Teleport", ImVec2(bx, p.y + (wh - BtnH()) * 0.5f), true, fReg)) Act_Teleport(e.x, e.y, e.z, L.title);
            ImGui::PopID();
            std::string title = (L.region.empty() ? std::string("Inne") : L.region) + "  ›  " + L.title;
            TextClip(ImVec2(p.x + 14 * S, p.y + 7 * S), fBold, C_TEXT, title, bx - 10 * S);
            float tx = p.x + 14 * S;
            Text(ImVec2(tx, p.y + 27 * S), fSmall, C_FAINT, e.name.c_str());
            tx += TextW(e.name.c_str(), fSmall) + 10 * S;
            for (size_t t = L.region.empty() ? 0 : 1; t < L.tags.size() && tx < bx - 80 * S; t++) {
                float pw; Pill(ImVec2(tx, p.y + 25 * S), L.tags[t].c_str(), C_ACC, &pw); tx += pw + 4 * S;
            }
            ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + wh + 4 * S)); ImGui::Dummy(ImVec2(w, 0));
        }
}
// ---- ESP colours (card part)
float EspColors(float x, float y, float cw, bool draw) {
    static const char* names[7] = { "Wrogowie", "Drużyna", "Neutralni", "Potwory", "Martwi", "Przedmioty", "Skrzynie" };
    float rh = 38 * S;
    if (!draw) return 7 * rh;
    for (int i = 0; i < 7; i++) {
        float yy = y + i * rh, cy = yy + rh * 0.5f;
        TextV(x + 18 * S, cy, fReg, C_TEXT, names[i]);
        ImVec4 col = ImGui::ColorConvertU32ToFloat4(g_espColors[i]);
        char hex[12]; snprintf(hex, sizeof hex, "#%02X%02X%02X", (int)(col.x * 255), (int)(col.y * 255), (int)(col.z * 255));
        float tw = TextW(hex, fSmall);
        TextV(x + cw - 56 * S - tw, cy, fSmall, C_DIM, hex);
        ImGui::SetCursorScreenPos(ImVec2(x + cw - 18 * S - 24 * S, cy - 12 * S));
        ImGui::PushID(i + 4000);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12 * S);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5 * S, 5 * S));
        if (ImGui::ColorEdit3("##c", &col.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) g_espColors[i] = ImGui::ColorConvertFloat4ToU32(ImVec4(col.x, col.y, col.z, 1));
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            T.Save();
        }
        ImGui::PopStyleVar(2);
        ImGui::PopID();
        if (i < 6) DL()->AddLine(ImVec2(x + 18 * S, yy + rh), ImVec2(x + cw - 18 * S, yy + rh), U32(C_LINE));
    }
    return 7 * rh;
}

// ---- Narzędzia / Kompendium: every cached portrait and item icon (debugging + browsing)
int g_cmpKind = 0; char g_cmpSearch[64] = ""; std::string g_cmpSel; double g_cmpAt = -10;
std::vector<std::string> g_cmpKeys;
std::string CompName(const std::string& key) {
    if (key == "PC_HERO") return "Bezimienny";
    if (key == "LOOKPREVIEW") return "Podgląd wyglądu";
    if (key.rfind("I_", 0) == 0) { std::string id = key.substr(2); for (auto& e : g_cat.items) if (e.id == id) return e.name; return id; }
    for (auto& e : g_cat.npcs) if (e.id == key) return e.name;
    return key;
}
void CompendiumPage(float w) {
    if (NowSec() - g_cmpAt > 3.0) { g_cmpAt = NowSec(); g_cmpKeys = PortraitKeys(); }
    int nNpc = 0, nItem = 0;
    for (auto& k : g_cmpKeys) (k.rfind("I_", 0) == 0 ? nItem : nNpc)++;
    std::set<std::string> have(g_cmpKeys.begin(), g_cmpKeys.end());
    int missNpc = 0, missItem = 0;
    { std::set<std::string> seen; for (auto& e : g_cat.npcs) if (seen.insert(e.id).second && !have.count(e.id)) missNpc++; }
    for (auto& e : g_cat.items) if (!have.count("I_" + e.id)) missItem++;
    BeginPanel("##cmpf", nullptr, w);
    InputBox("##cmps", g_cmpSearch, sizeof g_cmpSearch, "Szukaj po nazwie albo ID...", w * 0.36f);
    ImGui::SameLine();
    char b0[48], b1[48], b2[48];
    snprintf(b0, sizeof b0, "Wszystko (%d)", (int)g_cmpKeys.size()); snprintf(b1, sizeof b1, "Postacie (%d)", nNpc); snprintf(b2, sizeof b2, "Przedmioty (%d)", nItem);
    if (Chip(b0, g_cmpKind == 0)) g_cmpKind = 0;
    if (Chip(b1, g_cmpKind == 1, ICN_USER)) g_cmpKind = 1;
    if (Chip(b2, g_cmpKind == 2, ICN_BAG)) g_cmpKind = 2;
    ImGui::NewLine();
    PF(fSmall); ImGui::TextColored(C_DIM, "Portrety postaci robią się same, gdy wycelujesz w postać albo ją zapiszesz.  ·  Brakuje ikon przedmiotów: %d", missItem); ImGui::PopFont();
    (void)missNpc;
    ImVec2 bp = ImGui::GetCursorScreenPos();
    if (Button("##cmpi", "Zrób brakujące ikony przedmiotów", bp, false)) Act_PhotoBatch(1);
    {   // build 15: wipe and regenerate (second click confirms)
        static double armI = 0, armP = 0;
        float x2 = bp.x + ButtonW("Zrób brakujące ikony przedmiotów") + 8 * S;
        const char* l1 = NowSec() - armI < 3 ? "Na pewno? Kliknij jeszcze raz" : "Usuń wszystkie ikony i zrób od nowa";
        if (Button("##cmpri", l1, ImVec2(x2, bp.y), false)) { if (NowSec() - armI < 3) { Act_PhotoReset(1); armI = 0; g_cmpAt = -10; } else armI = NowSec(); }
        x2 += ButtonW(l1) + 8 * S;
        const char* l2 = NowSec() - armP < 3 ? "Na pewno? Kliknij jeszcze raz" : "Usuń wszystkie portrety postaci";
        if (Button("##cmprp", l2, ImVec2(x2, bp.y), false)) { if (NowSec() - armP < 3) { Act_PhotoReset(0); armP = 0; g_cmpAt = -10; } else armP = NowSec(); }
    }
    ImGui::SetCursorScreenPos(ImVec2(bp.x, bp.y + BtnH()));
    ImGui::Dummy(ImVec2(0, 0));
    EndPanel();
    std::vector<const std::string*> v;
    for (auto& k : g_cmpKeys) {
        bool item = k.rfind("I_", 0) == 0;
        if ((g_cmpKind == 1 && item) || (g_cmpKind == 2 && !item)) continue;
        if (!MatchQ(g_cmpSearch, CompName(k), k)) continue;
        v.push_back(&k);
    }
    // selected picture: big preview + actions
    if (!g_cmpSel.empty()) {
        BeginPanel("##cmpsel", nullptr, w);
        ImVec2 p = ImGui::GetCursorScreenPos();
        float big = 200 * S;
        DL()->AddRectFilled(p, ImVec2(p.x + big, p.y + big), U32(C_CARD2), R_ITEM());
        if (void* tex = PortraitFor(g_cmpSel)) DL()->AddImage((ImTextureID)(intptr_t)tex, p, ImVec2(p.x + big, p.y + big));
        float tx = p.x + big + 18 * S;
        Text(ImVec2(tx, p.y + 4 * S), fBig, C_TEXT, CompName(g_cmpSel).c_str());
        Text(ImVec2(tx, p.y + 36 * S), fSmall, C_ACC, g_cmpSel.c_str());
        float bx = tx, by = p.y + 66 * S;
        float r1 = ButtonW("Zrób ponownie"), r2 = ButtonW("Usuń");
        if (Button("##cmpre", "Zrób ponownie", ImVec2(bx, by))) { Act_PhotoKey(g_cmpSel); }
        if (Button("##cmpdel", "Usuń", ImVec2(bx + r1 + 8 * S, by), false)) { PortraitDelete(g_cmpSel); g_cmpSel.clear(); g_cmpAt = -10; }
        if (Button("##cmpx", "Zamknij", ImVec2(bx + r1 + r2 + 16 * S, by), false)) g_cmpSel.clear();
        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + big));
        ImGui::Dummy(ImVec2(0, 0));
        EndPanel();
    }
    if (v.empty()) { ImGui::TextColored(C_DIM, g_cmpKeys.empty() ? "Brak zapisanych obrazków - zrób je przyciskami wyżej." : "Nic nie pasuje do filtra."); return; }
    float tile = 112 * S, gap = 8 * S;
    int cols = std::max(1, (int)((w + gap) / (tile + gap)));
    int rows = ((int)v.size() + cols - 1) / cols;
    float th = tile + 34 * S;
    ImGuiListClipper clip; clip.Begin(rows, th + gap);
    while (clip.Step())
        for (int r = clip.DisplayStart; r < clip.DisplayEnd; r++) {
            ImVec2 rp = ImGui::GetCursorScreenPos();
            for (int c = 0; c < cols; c++) {
                int i = r * cols + c; if (i >= (int)v.size()) break;
                const std::string& k = *v[i];
                ImVec2 p(rp.x + c * (tile + gap), rp.y);
                ImGui::SetCursorScreenPos(p);
                ImGui::PushID(k.c_str());
                bool clicked = ImGui::InvisibleButton("##t", ImVec2(tile, th));
                bool hov = ImGui::IsItemHovered();
                ImGui::PopID();
                bool sel = g_cmpSel == k;
                DL()->AddRectFilled(p, ImVec2(p.x + tile, p.y + th), U32(sel ? C_ACCD : hov ? C_HOVER : C_CARD), R_ITEM());
                float im = tile - 16 * S;
                if (void* tex = PortraitFor(k)) DL()->AddImage((ImTextureID)(intptr_t)tex, ImVec2(p.x + 8 * S, p.y + 6 * S), ImVec2(p.x + 8 * S + im, p.y + 6 * S + im));
                TextClip(ImVec2(p.x + 8 * S, p.y + tile - 4 * S), fSmall, C_TEXT, CompName(k), p.x + tile - 6 * S);
                TextClip(ImVec2(p.x + 8 * S, p.y + tile + 13 * S), fSmall, C_FAINT, k, p.x + tile - 6 * S);
                if (clicked) g_cmpSel = sel ? "" : k;
            }
            ImGui::SetCursorScreenPos(ImVec2(rp.x, rp.y + th + gap));
            ImGui::Dummy(ImVec2(0, 0));
        }
}

// ---- Narzędzia / Konsola
bool MultiBox(const char* id, char* buf, size_t n, const char* hint, float w, float h0);
void ConsolePanel(const Snapshot& s, float w) {
    BeginPanel("##con", "Konsola i makra", w);
    PF(fSmall); ImGui::TextColored(C_DIM, "Jedna komenda w linii, np.: insert ITMW_2H_SWORD_HEAVY_04 / goto waypoint OCC_BARONS_GREATHALL_CENTER / cheat full"); ImGui::PopFont();
    MultiBox("##conin", consoleBuf, sizeof consoleBuf, "komendy, jedna w linii...", w - 36 * S, 100 * S);
    ImGui::AlignTextToFramePadding(); ImGui::TextColored(C_DIM, "Powtórz x"); ImGui::SameLine(); NumField("##rep", &macroRepeat, 1, 500, 70 * S); ImGui::SameLine();
    ImVec2 bp = ImGui::GetCursorScreenPos();
    if (Button("##run", "Wykonaj", bp) && consoleBuf[0]) Act_RunMacro({ "", consoleBuf, macroRepeat });
    float nx = bp.x + ButtonW("Wykonaj") + 12 * S;
    ImGui::SetCursorScreenPos(ImVec2(nx, bp.y));
    float rowEnd = ImGui::GetWindowPos().x + w - 18 * S;
    InputBox("##mname", macroName, sizeof macroName, "nazwa makra...", std::max(120 * S, rowEnd - nx - ButtonW("Zapisz makro") - ImGui::GetStyle().ItemSpacing.x)); ImGui::SameLine();
    if (Button("##msave", "Zapisz makro", ImGui::GetCursorScreenPos(), false) && macroName[0] && consoleBuf[0]) {
        bool rep = false;
        for (auto& m : T.macros) if (m.name == macroName) { m.cmds = consoleBuf; m.repeat = macroRepeat; rep = true; }
        if (!rep) T.macros.push_back({ macroName, consoleBuf, macroRepeat });
        T.SaveMacros(); macroName[0] = 0; T.Toast("Zapisano makro");
    }
    if (!s.marvinOut.empty()) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, C_CARD2); ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, R_ITEM());
        ImGui::BeginChild("##out", ImVec2(w - 36 * S, 110 * S), false);
        PF(fSmall); ImGui::TextWrapped("%s", s.marvinOut.c_str()); ImGui::PopFont();
        ImGui::EndChild();
        ImGui::PopStyleVar(); ImGui::PopStyleColor();
    }
    EndPanel();
    if (!T.macros.empty()) SectionLabel("Zapisane makra");
    float h = 50 * S;
    for (int i = 0; i < (int)T.macros.size(); i++) {
        Macro& m = T.macros[i];
        ImVec2 p = ImGui::GetCursorScreenPos();
        ListRowBg(p, w, h, i);
        Text(ImVec2(p.x + 14 * S, p.y + 7 * S), fBold, C_TEXT, m.name.c_str());
        std::string first = m.cmds.substr(0, m.cmds.find('\n'));
        char sub[200]; snprintf(sub, sizeof sub, "x%d  ·  %s%s", m.repeat, first.c_str(), m.cmds.find('\n') != std::string::npos ? " ..." : "");
        Text(ImVec2(p.x + 14 * S, p.y + 27 * S), fSmall, C_DIM, sub);
        ImGui::PushID(i + 5000);
        float bs = BtnH(), by = p.y + (h - bs) * 0.5f;
        if (IconButton("##run", ICN_PLAY, "Uruchom", ImVec2(p.x + w - bs * 3 - 22 * S, by), bs, C_ON)) Act_RunMacro(m);
        if (IconButton("##edit", ICN_PENCIL, "Wczytaj do edytora", ImVec2(p.x + w - bs * 2 - 16 * S, by), bs, C_TEXT)) {
            snprintf(consoleBuf, sizeof consoleBuf, "%s", m.cmds.c_str()); snprintf(macroName, sizeof macroName, "%s", m.name.c_str()); macroRepeat = m.repeat;
        }
        bool del = IconButton("##del", ICN_TRASH, "Usuń", ImVec2(p.x + w - bs - 10 * S, by), bs, C_ST_CRASH);
        ImGui::PopID();
        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + 4 * S)); ImGui::Dummy(ImVec2(w, 0));
        if (del) { T.macros.erase(T.macros.begin() + i); T.SaveMacros(); break; }
    }
    ImGui::Dummy(ImVec2(0, 6 * S));
    BeginPanel("##scr", "Wywołaj funkcję skryptu (bez parametrów)", w);
    InputBox("##scrn", scriptBuf, sizeof scriptBuf, "nazwa funkcji Daedalusa...", w - 36 * S - ButtonW("Wywołaj") - ImGui::GetStyle().ItemSpacing.x);
    ImGui::SameLine();
    if (Button("##run2", "Wywołaj", ImGui::GetCursorScreenPos()) && scriptBuf[0]) Act_RunScript(scriptBuf);
    EndPanel();
}
void DiagPage(const Snapshot& s, float w) {
    BeginPanel("##diag", "Autotest rdzenia (przy każdym wczytaniu świata)", w);
    if (s.diag.empty()) ImGui::TextColored(C_DIM, "Wczytaj grę - autotest uruchomi się sam.");
    for (auto& d : s.diag) {
        ImVec2 p = ImGui::GetCursorScreenPos();
        DL()->AddCircleFilled(ImVec2(p.x + 6 * S, p.y + 10 * S), 5 * S, U32(d.ok ? C_ST_OK : C_ST_CRASH), 12);
        ImGui::SetCursorScreenPos(ImVec2(p.x + 20 * S, p.y));
        ImGui::TextColored(d.ok ? C_TEXT : C_ST_CRASH, "%s", d.text.c_str());
    }
    EndPanel();
    BeginPanel("##crash", "Wykryte crashe (ta wersja)", w);
    if (T.crashKeys.empty()) ImGui::TextColored(C_DIM, "Brak - nic nie wywaliło gry od instalacji tej wersji.");
    for (auto& k : T.crashKeys) ImGui::TextColored(C_ST_CRASH, "%s", k.c_str());
    PF(fSmall); ImGui::TextColored(C_DIM, "Log: %s", DataPath("G1CrazyTrainer.log").c_str()); ImGui::PopFont();
    EndPanel();
}
// ---- Konfiguracja
int g_cfgTab = 0;
void ConfigPage(float w) {
    // build 16: settings split into tabs
    static const char* tabs[] = { "Wygląd menu", "Zachowanie", "Okna i obrazki", "Skróty klawiszowe", "Tryb testera" };
    for (int i = 0; i < 5; i++) { ImGui::PushID(i + 7300); if (Chip(tabs[i], g_cfgTab == i)) g_cfgTab = i; ImGui::PopID(); }
    ImGui::NewLine(); ImGui::Dummy(ImVec2(0, 4 * S));
    std::vector<CardSpec> cards;
    CardSpec look{ "Wygląd menu", {}, nullptr, 0, false };
    look.customH = 124 * S;
    look.custom = [](float x, float y, float) -> float {
        Text(ImVec2(x + 18 * S, y + 6 * S), fReg, C_TEXT, "Kolor akcentu");
        static const float presets[][3] = { { 0.66f, 0.55f, 1.f }, { 0.36f, 0.66f, 1.f }, { 0.35f, 0.85f, 0.60f }, { 1.f, 0.72f, 0.30f }, { 1.f, 0.42f, 0.45f }, { 0.95f, 0.45f, 0.85f } };
        for (int i = 0; i < 6; i++) {
            ImGui::PushID(i + 4100);
            ImVec2 c(x + 32 * S + i * 34 * S, y + 46 * S);
            ImGui::SetCursorScreenPos(ImVec2(c.x - 13 * S, c.y - 13 * S));
            if (ImGui::InvisibleButton("##pre", ImVec2(26 * S, 26 * S))) { memcpy(T.accent, presets[i], sizeof(float) * 3); T.Save(); }
            DL()->AddCircleFilled(c, 11 * S, ImGui::GetColorU32(ImVec4(presets[i][0], presets[i][1], presets[i][2], 1)), 24);
            if (std::fabs(T.accent[0] - presets[i][0]) < 0.01f && std::fabs(T.accent[1] - presets[i][1]) < 0.01f && std::fabs(T.accent[2] - presets[i][2]) < 0.01f)
                DL()->AddCircle(c, 15 * S, U32(C_TEXT), 24, 1.5f * S);
            ImGui::PopID();
        }
        ImGui::SetCursorScreenPos(ImVec2(x + 18 * S, y + 76 * S));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12 * S);
        ImGui::ColorEdit3("Własny kolor", T.accent, ImGuiColorEditFlags_NoInputs);
        if (ImGui::IsItemDeactivatedAfterEdit()) T.Save();
        ImGui::PopStyleVar();
        return 124 * S;
    };
    if (g_cfgTab == 0) { cards.push_back(look); for (auto& c : CardsFor("Konfiguracja", "Wygląd menu")) cards.push_back(c); }
    CardSpec tester{ "Tryb testera", {}, nullptr, 0, false };
    tester.customH = 136 * S;
    tester.custom = [](float x, float y, float cw) -> float {
        TextV(x + 18 * S, y + 20 * S, fReg, C_TEXT, "Włącz tryb testera");
        if (Switch("##tm", T.testerMode, ImVec2(x + cw - 18 * S - 36 * S, y + 10 * S))) { T.testerMode = !T.testerMode; T.Save(); }
        float dx = x + 18 * S, dy = y + 48 * S;
        struct { const ImVec4* c; const char* t; } lg[] = { { &C_ST_OK, "OK" }, { &C_ST_BUG, "BUG" }, { &C_ST_CRASH, "CRASH" }, { &C_ST_NEW, "TEST" } };
        for (auto& l : lg) { DL()->AddCircleFilled(ImVec2(dx + 4 * S, dy + 8 * S), 4 * S, U32(*l.c), 12); Text(ImVec2(dx + 14 * S, dy), fSmall, C_DIM, l.t); dx += 66 * S; }
        Text(ImVec2(x + 18 * S, y + 70 * S), fSmall, C_FAINT, "kropka = status opcji, flaga obok = zgłoszenie do tej opcji");
        if (T.testerMode && Button("##otc", ICN_CHECK "  Otwórz Centrum testera", ImVec2(x + 18 * S, y + 94 * S), false)) testsWin = true;
        return 136 * S;
    };
    if (g_cfgTab == 4) cards.push_back(tester);
    if (g_cfgTab == 1) for (auto& c : CardsFor("Konfiguracja", "Zachowanie")) cards.push_back(c);
    if (g_cfgTab == 2) { for (auto& c : CardsFor("Konfiguracja", "Okna")) cards.push_back(c); for (auto& c : CardsFor("Konfiguracja", "Obrazki")) cards.push_back(c); }
    CardSpec tog{ "Przełączniki", {}, nullptr, 0, false };
    tog.customH = 48 * S;
    tog.custom = [](float x, float y, float) -> float {
        if (Button("##restore", "Przywróć ostatnio włączone", ImVec2(x + 18 * S, y + 8 * S), false)) T.RestoreToggles();
        return 48 * S;
    };
    if (g_cfgTab == 1) cards.push_back(tog);
    Masonry(cards, w);
    if (g_cfgTab != 3) return;
    SectionLabel("Skróty klawiszowe");
    BeginPanel("##hk", nullptr, w);
    PF(fSmall); ImGui::TextColored(C_DIM, "Skrót przypiszesz przy każdej opcji: najedź na wiersz i kliknij \"+ klawisz\". Działa z Ctrl / Alt / Shift."); ImGui::PopFont();
    int n = 0;
    for (auto& r : T.rows) {
        if (r.kind == K_VALUE || !r.hotkey) continue;
        n++;
        ImGui::PushID(r.id.c_str());
        ImVec2 p = ImGui::GetCursorScreenPos();
        float rh = 34 * S;
        TextV(p.x, p.y + rh * 0.5f, fReg, C_TEXT, r.name.c_str());
        TextV(p.x + w * 0.42f, p.y + rh * 0.5f, fSmall, C_DIM, (r.cat + " › " + r.sub).c_str());
        float rx = p.x + w - 36 * S;
        if (r.hotkey && IconButton("##rmk", ICN_TIMES, "Usuń skrót", ImVec2(rx - 26 * S, p.y + (rh - 26 * S) * 0.5f), 26 * S, C_DIM)) { r.hotkey = 0; T.Save(); }
        HotkeyChip(r, rx - 34 * S, p.y + rh * 0.5f, true);
        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + rh)); ImGui::Dummy(ImVec2(10, 0));
        ImGui::PopID();
    }
    if (!n) ImGui::TextColored(C_DIM, "Brak przypisanych skrótów.");
    EndPanel();
}

// ------------------------------------------------------------------ tester centre (build 8)
// Data lives in tester.cpp (append-only ticket history, events, export); this is only the UI.
int testFilter = 0, testDot = -1; char testSearch[64] = "";
int g_tTab = 0;                                    // 0 checklist, 1 tickets, 2 new ticket, 3 send, 4 log
std::set<std::string> g_tOpen;                     // expanded history in the checklist / ticket list
struct KnownIssue { const char* id; const char* text; Status st = ST_NEW; };
const KnownIssue kIssues[] = {
    { "UI-01", "Scroll kółkiem myszy w menu", ST_OK },
    { "UI-02", "Zrzut ekranu: F11 cały, Shift+F11 obszar - z menu, jak Win+Shift+S", ST_NEW },
    { "UI-03", "Esc w menu trainera tylko zamyka menu", ST_OK },
    { "UI-04", "ESP i napisy chowają się w menu pauzy gry (Esc)", ST_OK },
    { "UI-05", "Suwaki: wartość nad suwakiem, równe wyrównanie", ST_OK },
    { "UI-06", "Przełącznik Obecna postać / Bezimienny / Cel", ST_OK },
    { "UI-07", "Portrety postaci (na bieżąco) i ikony przedmiotów", ST_OK },
    { "UI-08", "Spawner przedmiotów: wyszukiwanie, sortowanie, filtry", ST_OK },
    { "UI-09", "Okno aktywnych modów: wyłączone / tylko w menu / w grze", ST_OK },
    { "UI-10", "Lista skrótów klawiszowych", ST_OK },
    { "UI-11", "Teleport: filtr regionu i rodzaju miejsca", ST_OK },
    { "UI-12", "Wygląd: podgląd na klonie + przycisk Zastosuj", ST_OK },
    { "UI-13", "Pisanie w polach tekstowych nie steruje grą", ST_OK },
    { "UI-14", "Wydajność: przycięcia / spadki FPS przy włączonym trainerze", ST_OK },
    { "UI-15", "Okno testera: checklista, zgłoszenia, wysyłanie", ST_OK },
};
// form draft
struct Draft {
    int type = tst::TT_BUG, severity = 1, repro = 0; std::string target, targetName;
    char title[160] = "", steps[2000] = "", expected[600] = "", actual[1500] = "", notes[1500] = "";
    bool withContext = true;
    std::vector<std::string> files;
    std::string editUid;   // build 10: editing an existing ticket of mine from this build
} g_draft;
void EditTicket(const tst::Ticket& t) {
    Draft d; d.editUid = t.uid; d.type = t.type; d.severity = t.severity; d.repro = t.repro; d.target = t.target; d.targetName = t.targetName;
    snprintf(d.title, sizeof d.title, "%s", t.title.c_str()); snprintf(d.steps, sizeof d.steps, "%s", t.steps.c_str());
    snprintf(d.expected, sizeof d.expected, "%s", t.expected.c_str()); snprintf(d.actual, sizeof d.actual, "%s", t.actual.c_str());
    snprintf(d.notes, sizeof d.notes, "%s", t.notes.c_str()); d.files = t.files; d.withContext = !t.context.empty();
    g_draft = d; g_tTab = 2;
}
char g_nickBuf[32] = ""; bool g_nickInit = false;
char g_targetFilter[64] = "";
std::string g_lastExport;

std::string TargetName(const std::string& id) {
    if (id.empty()) return "Ogólne";
    auto pre = [&](const char* p, const char* label) -> std::string { size_t n = strlen(p); return id.compare(0, n, p) == 0 ? std::string(label) + id.substr(n) : ""; };
    for (auto pr : { std::make_pair("kat:", "Kategoria: "), std::make_pair("pod:", ""), std::make_pair("karta:", "Karta: "),
                     std::make_pair("sekcja:", "Sekcja: "), std::make_pair("aspekt:", "Aspekt: ") }) { std::string r = pre(pr.first, pr.second); if (!r.empty()) return r; }
    for (auto& k : kIssues) if (id == k.id) return k.text;
    if (Row* r = T.Get(id)) return r->name;
    return id;
}
void OpenTicketFor(const std::string& id, int type) {
    tst::Load();
    g_draft = Draft{}; g_draft.type = type; g_draft.target = id; g_draft.targetName = TargetName(id);
    g_tTab = 2; testsWin = true;
}
const ImVec4& TypeCol(int t) {
    switch (t) { case tst::TT_OK: return C_ST_OK; case tst::TT_PARTIAL: return C_ST_BUG; case tst::TT_BUG: case tst::TT_CRASH: return C_ST_CRASH; default: return C_ACC; }
}
// multi-line text box (build 10): wraps to the box width, the clear "x" has its own column on the right,
// and the bottom edge can be dragged to change the height (remembered per box)
bool MultiBox(const char* id, char* buf, size_t n, const char* hint, float w, float h0) {
    static std::unordered_map<ImGuiID, float> heights;
    ImGuiID key = ImGui::GetID(id);
    auto it = heights.find(key);
    float h = it == heights.end() ? h0 : it->second;
    ImVec2 p = ImGui::GetCursorScreenPos();
    float gutter = 30 * S;
    DL()->AddRectFilled(p, ImVec2(p.x + w, p.y + h), U32(C_CARD2), R_BTN());
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10 * S, 7 * S));
    bool ch = ImGui::InputTextMultiline(id, buf, n, ImVec2(w - gutter, h), ImGuiInputTextFlags_WordWrap);
    bool active = ImGui::IsItemActive();
    ImGui::PopStyleVar(); ImGui::PopStyleColor();
    if (active) DL()->AddRect(p, ImVec2(p.x + w, p.y + h), U32(C_ACC, 0.45f), R_BTN(), 1.f * S);
    if (!buf[0] && !active) DL()->AddText(ImVec2(p.x + 10 * S, p.y + 7 * S), U32(C_FAINT), hint);
    if (buf[0]) {   // clear "x" in the right column (text never runs under it)
        float r = 9 * S; ImVec2 c(p.x + w - gutter * 0.5f, p.y + r + 7 * S);
        bool hov = ImGui::IsMouseHoveringRect(ImVec2(c.x - r, c.y - r), ImVec2(c.x + r, c.y + r));
        DL()->AddCircleFilled(c, r, U32(hov ? C_ACC : C_CARD), 16);
        IconAt(DL(), c, ICN_TIMES, U32(hov ? ImVec4(1, 1, 1, 1) : C_DIM), 10 * S);
        if (hov && ImGui::IsMouseClicked(0)) { buf[0] = 0; ImGui::ClearActiveID(); ch = true; }
    }
    // resize grip in the bottom-right corner (build 13 - like the menu's own window grip)
    float gs = 16 * S;
    ImVec2 g0(p.x + w - gs - 2 * S, p.y + h - gs - 2 * S);
    ImGui::SetCursorScreenPos(g0);
    ImGui::PushID(id);
    ImGui::InvisibleButton("##rsz", ImVec2(gs, gs));
    bool gh = ImGui::IsItemHovered() || ImGui::IsItemActive();
    if (ImGui::IsItemActive()) heights[key] = h = std::max(36 * S, std::min(900 * S, h + ImGui::GetIO().MouseDelta.y));
    ImGui::PopID();
    ImVec2 br(p.x + w - 5 * S, p.y + h - 5 * S);
    for (int k = 1; k <= 3; k++) DL()->AddLine(ImVec2(br.x - k * 4 * S, br.y), ImVec2(br.x, br.y - k * 4 * S), U32(gh ? C_ACC : C_FAINT), 1.3f * S);
    if (gh) Tip("Przeciągnij, żeby zmienić wysokość");
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + ImGui::GetStyle().ItemSpacing.y));
    ImGui::Dummy(ImVec2(0, 0));
    return ch;
}
void FieldLabel(const char* t) { PF(fSmall); ImGui::TextColored(C_DIM, "%s", t); ImGui::PopFont(); }

// history of one option: known issues (GitHub), developer notes, every ticket ever written for it
// build 17: long histories are folded - the 5 newest entries, the rest behind "show all"
std::set<std::string> g_histOpen;
void HistoryBlock(const std::string& id, float w) {
    auto known = tst::KnownFor(id);
    auto mine = tst::For(id);
    Row* r = T.Get(id);
    ImGui::Indent(14 * S);
    if (r && !r->hist.empty()) { PF(fSmall); ImGui::TextColored(C_DIM, "Od autora:"); ImGui::SameLine(); ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + w - 120 * S); ImGui::TextColored(C_TEXT, "%s", r->hist.c_str()); ImGui::PopTextWrapPos(); ImGui::PopFont(); }
    int total = (int)known.size() + (int)mine.size();
    bool open = g_histOpen.count(id) != 0;
    int shown = 0, limit = open ? 1 << 30 : 5;
    for (int i = (int)mine.size() - 1; i >= 0 && shown < limit; i--, shown++) {   // newest first
        auto* t = mine[i];
        PF(fSmall);
        ImGui::TextColored(TypeCol(t->type), "%s", tst::kTypeNames[t->type]); ImGui::SameLine();
        ImGui::TextColored(C_FAINT, "b%d · %s · %s%s", t->build, t->time.c_str(), t->nick.c_str(), t->sent ? " · wysłane" : "");
        std::string body = t->title;
        if (!t->actual.empty() && t->actual != t->title) body += (body.empty() ? "" : " - ") + t->actual;
        if (!body.empty()) { ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + w - 60 * S); ImGui::TextColored(C_TEXT, "%s", body.c_str()); ImGui::PopTextWrapPos(); }
        ImGui::PopFont();
    }
    for (size_t i = 0; i < known.size() && shown < limit; i++, shown++) {
        auto* k = known[i];
        PF(fSmall);
        ImGui::TextColored(k->state == "open" ? C_ST_BUG : C_ST_OK, "#%d %s", k->issue, k->state == "open" ? "otwarte" : "rozwiązane");
        ImGui::SameLine(); ImGui::TextColored(C_TEXT, "%s", k->title.c_str());
        if (k->build) { ImGui::SameLine(); ImGui::TextColored(C_FAINT, "(build %d)", k->build); }
        ImGui::PopFont();
    }
    if (total > 5) {
        ImGui::PushID(id.c_str());
        char lb[64]; snprintf(lb, sizeof lb, open ? "\xef\x84\x86  Zwiń historię" : "\xef\x84\x87  Pokaż całą historię (%d)", total);
        PF(fSmall); if (ImGui::Selectable(lb, false, 0, ImVec2(w * 0.5f, 0))) { if (open) g_histOpen.erase(id); else g_histOpen.insert(id); } ImGui::PopFont();
        ImGui::PopID();
    }
    if (known.empty() && mine.empty() && !(r && !r->hist.empty())) { PF(fSmall); ImGui::TextColored(C_FAINT, "Brak historii."); ImGui::PopFont(); }
    ImGui::Unindent(14 * S);
    ImGui::Dummy(ImVec2(0, 4 * S));
}

void TestRow(const std::string& id, const std::string& name, const std::string& where, float w, int idx) {
    int v = tst::LastVerdict(id);
    int st = v < 0 ? 0 : v == tst::TT_OK ? 1 : v == tst::TT_PARTIAL ? 2 : 3;
    if (testFilter > 0 && st != testFilter - 1) return;
    int nT = (int)tst::For(id).size(), nK = (int)tst::KnownFor(id).size();
    if (testFilter == 5 && !nK) return;
    if (testDot >= 0) {
        int dot = ST_NEW;
        if (Row* rr = T.Get(id)) dot = rr->status; else for (auto& k : kIssues) if (id == k.id) dot = k.st;
        if (dot != testDot) return;
    }
    if (!MatchQ(testSearch, name, id) && !MatchQ(testSearch, where, where)) return;
    static const char* stN[] = { "Nie sprawdzone", "Działa", "Częściowo", "Nie działa" };
    static const ImVec4* stC[] = { &C_DIM, &C_ST_OK, &C_ST_BUG, &C_ST_CRASH };
    ImVec2 p = ImGui::GetCursorScreenPos();
    float h = 62 * S;
    DL()->AddRectFilled(p, ImVec2(p.x + w, p.y + h), U32(idx & 1 ? C_CARD : C_CARD2), R_ITEM());
    DL()->AddRectFilled(ImVec2(p.x, p.y + 10 * S), ImVec2(p.x + 3 * S, p.y + h - 10 * S), U32(*stC[st]), 1.5f * S);
    ImGui::PushID(id.c_str());
    float pw; Pill(ImVec2(p.x + 14 * S, p.y + 10 * S), ("#" + id).c_str(), C_ACC, &pw);
    float bx = p.x + w - 14 * S;
    // ticket + history buttons
    bx -= 32 * S;
    if (IconButton("##tk", ICN_FLAG, "Napisz zgłoszenie do tej opcji", ImVec2(bx, p.y + 14 * S), 32 * S, C_TEXT)) OpenTicketFor(id, tst::TT_BUG);
    bx -= 8 * S;
    for (int k = 3; k >= 1; k--) {
        float bw = ButtonW(stN[k], fReg);
        bx -= bw;
        ImGui::PushID(k);
        bool act = st == k;
        if (Button("##st", stN[k], ImVec2(bx, p.y + 14 * S), act, fReg, 0, stC[k]) && !act) {
            tst::Ticket t; t.type = k == 1 ? tst::TT_OK : k == 2 ? tst::TT_PARTIAL : tst::TT_BUG; t.verdict = true;
            t.target = id; t.targetName = name; t.title = stN[k];
            tst::Add(t);
            if (k > 1) { OpenTicketFor(id, t.type); T.Toast("Zapisano ocenę - opisz problem w formularzu (opcjonalnie)"); }
        }
        ImGui::PopID();
        bx -= 6 * S;
    }
    TextClip(ImVec2(p.x + 22 * S + pw, p.y + 9 * S), fBold, C_TEXT, name, bx - 10 * S);
    // second line: where + history counter (click = expand)
    std::string hl = nT || nK ? (std::to_string(nT) + " wpisów" + (nK ? " · " + std::to_string(nK) + " znanych problemów" : "")) : "";
    TextClip(ImVec2(p.x + 22 * S + pw, p.y + 33 * S), fSmall, C_DIM, where, bx - 10 * S - (hl.empty() ? 0 : TextW(hl.c_str(), fSmall) + 30 * S));
    bool open = g_tOpen.count(id) != 0;
    if (!hl.empty() || (T.Get(id) && !T.Get(id)->hist.empty())) {
        if (hl.empty()) hl = "historia";
        float hw = TextW(hl.c_str(), fSmall);
        ImVec2 hp(bx - 10 * S - hw - 16 * S, p.y + 33 * S);
        ImGui::SetCursorScreenPos(hp);
        if (ImGui::InvisibleButton("##hist", ImVec2(hw + 16 * S, 18 * S))) { if (open) g_tOpen.erase(id); else g_tOpen.insert(id); }
        bool hv = ImGui::IsItemHovered();
        IconAt(DL(), ImVec2(hp.x + 5 * S, hp.y + 8 * S), open ? ICN_ARROW : ICO_CHEVR, U32(hv ? C_ACC : C_FAINT), 9 * S);
        Text(ImVec2(hp.x + 14 * S, hp.y), fSmall, hv || open ? C_ACC : (nK ? C_ST_BUG : C_DIM), hl.c_str());
    }
    ImGui::PopID();
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + 4 * S));
    if (open) { ImGui::Dummy(ImVec2(0, 2 * S)); HistoryBlock(id, w); }
    ImGui::Dummy(ImVec2(w, 0));
}

void TabChecklist(float w) {
    BeginPanel("##tst", nullptr, w);
    PF(fSmall); ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + w - 40 * S);
    ImGui::TextColored(C_DIM, "Oceń każdą opcję: Działa / Częściowo / Nie działa. Nic nie jest nadpisywane - każda ocena trafia do historii. "
                              "Flaga obok otwiera formularz zgłoszenia. Kliknij licznik wpisów, żeby zobaczyć wszystko, co już o tej opcji wiadomo.");
    ImGui::PopTextWrapPos(); ImGui::PopFont();
    InputBox("##tsq", testSearch, sizeof testSearch, "Szukaj po nazwie albo #ID...", w * 0.38f);
    ImGui::SameLine();
    const char* fl[] = { "Wszystko", "Nie sprawdzone", "Działa", "Częściowo", "Nie działa", "Ze znanymi problemami" };
    for (int i = 0; i < 6; i++) if (Chip(fl[i], testFilter == i)) testFilter = i;
    ImGui::NewLine();
    // build 9: filter by the author's status dot (alpha builds: test only what is new / broken, #6)
    ImGui::AlignTextToFramePadding(); ImGui::TextColored(C_DIM, "Kropka autora:"); ImGui::SameLine();
    struct { int st; const char* t; const ImVec4* c; } dots[] = { { -1, "dowolna", &C_DIM }, { ST_NEW, "TEST (nowe)", &C_ST_NEW }, { ST_BUG, "BUG", &C_ST_BUG }, { ST_CRASH, "CRASH", &C_ST_CRASH }, { ST_OK, "OK", &C_ST_OK } };
    for (auto& dt : dots) {
        ImGui::PushID(dt.st + 4400);
        ImVec2 cp = ImGui::GetCursorScreenPos();
        if (Chip((std::string("     ") + dt.t).c_str(), testDot == dt.st)) testDot = dt.st;
        if (dt.st >= 0) DL()->AddCircleFilled(ImVec2(cp.x + 16 * S, cp.y + 15 * S), 4.5f * S, U32(*dt.c), 12);
        ImGui::PopID();
    }
    ImGui::NewLine();
    int cnt[4] = {};
    for (auto& r : T.rows) if (r.kind != K_VALUE && r.cat != "Konfiguracja") { int v = tst::LastVerdict(r.id); cnt[v < 0 ? 0 : v == tst::TT_OK ? 1 : v == tst::TT_PARTIAL ? 2 : 3]++; }
    int all = cnt[0] + cnt[1] + cnt[2] + cnt[3];
    ImGui::TextColored(C_DIM, "Build %d:  sprawdzone %d z %d  ·  działa %d  ·  częściowo %d  ·  nie działa %d", TRAINER_BUILD, all - cnt[0], all, cnt[1], cnt[2], cnt[3]);
    EndPanel();
    SectionLabel("Menu i interfejs");
    int i = 0;
    for (auto& k : kIssues) TestRow(k.id, k.text, "interfejs", w, i++);
    std::string lastCat;
    for (auto& r : T.rows) {
        if (r.kind == K_VALUE || r.cat == "Konfiguracja") continue;
        if (r.cat != lastCat) { SectionLabel(r.cat.c_str()); lastCat = r.cat; }
        TestRow(r.id, r.name, r.sub + " › " + r.group, w, i++);
    }
}

void TicketCard(const tst::Ticket& t, float w, int idx, std::string& del) {
    bool open = g_tOpen.count(t.uid) != 0;
    ImGui::PushID(t.uid.c_str());
    ImVec2 p = ImGui::GetCursorScreenPos();
    float h = 52 * S;
    DL()->AddRectFilled(p, ImVec2(p.x + w, p.y + h), U32(idx & 1 ? C_CARD : C_CARD2), R_ITEM());
    float pw; Pill(ImVec2(p.x + 12 * S, p.y + 8 * S), tst::kTypeNames[t.type], TypeCol(t.type), &pw);
    float bx = p.x + w - 12 * S;
    bx -= 30 * S; if ((!t.sent || tst::Editable(t)) && IconButton("##del", ICN_TRASH, "Usuń", ImVec2(bx, p.y + 11 * S), 30 * S, C_DIM)) del = t.uid;
    bx -= 6 * S + 30 * S; if (IconButton("##cp", ICN_FILE, "Kopiuj tekst (np. na Discorda)", ImVec2(bx, p.y + 11 * S), 30 * S, C_TEXT)) { tst::CopyToClipboard(tst::ShortText(t)); T.Toast("Skopiowano do schowka"); }
    if (t.type != tst::TT_OK) { bx -= 6 * S + 30 * S; if (IconButton("##gh", ICN_SIGNIN, "Otwórz gotowe zgłoszenie na GitHubie (w przeglądarce)", ImVec2(bx, p.y + 11 * S), 30 * S, C_TEXT)) tst::OpenUrl(tst::GithubUrl(t)); }
    std::string title = t.title.empty() ? (t.targetName.empty() ? "(bez tytułu)" : t.targetName) : t.title;
    TextClip(ImVec2(p.x + 20 * S + pw, p.y + 7 * S), fBold, C_TEXT, title, bx - 10 * S);
    char meta[256]; snprintf(meta, sizeof meta, "%s  ·  build %d  ·  %s  ·  %s%s", t.targetName.empty() ? "ogólne" : t.targetName.c_str(), t.build, t.time.c_str(), t.nick.c_str(), t.sent ? "  ·  wysłane" : "  ·  NIEWYSŁANE");
    TextClip(ImVec2(p.x + 12 * S, p.y + 30 * S), fSmall, t.sent ? C_DIM : C_ACC, meta, bx - 10 * S);
    ImGui::SetCursorScreenPos(p);
    if (ImGui::InvisibleButton("##exp", ImVec2(bx - p.x - 6 * S, h))) { if (open) g_tOpen.erase(t.uid); else g_tOpen.insert(t.uid); }
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + 4 * S));
    if (open) {
        ImGui::Indent(14 * S); PF(fSmall);
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + w - 40 * S);
        ImGui::TextColored(C_FAINT, "%s", t.uid.c_str());
        if (t.type == tst::TT_BUG || t.type == tst::TT_CRASH || t.type == tst::TT_PARTIAL) ImGui::TextColored(C_DIM, "Waga: %s  ·  Powtarzalność: %s", tst::kSeverityNames[t.severity], tst::kReproNames[t.repro]);
        auto sec = [](const char* l, const std::string& v) { if (v.empty()) return; ImGui::TextColored(C_DIM, "%s", l); ImGui::TextColored(C_TEXT, "%s", v.c_str()); };
        sec(t.type == tst::TT_IDEA ? "Opis" : "Kroki", t.steps); sec(t.type == tst::TT_IDEA ? "Po co" : "Oczekiwane", t.expected); sec("Faktycznie", t.actual); sec("Uwagi", t.notes); sec("Kontekst (automatycznie)", t.context);
        ImGui::PopTextWrapPos(); ImGui::PopFont(); ImGui::Unindent(14 * S);
        ImGui::Dummy(ImVec2(0, 6 * S));
    }
    ImGui::PopID();
}
int g_listType = -1; bool g_listUnsent = false; char g_listSearch[64] = "";
void TabTickets(float w) {
    BeginPanel("##tl", nullptr, w);
    InputBox("##tls", g_listSearch, sizeof g_listSearch, "Szukaj w zgłoszeniach...", w * 0.34f);
    ImGui::SameLine();
    if (Chip("Wszystkie", g_listType < 0)) g_listType = -1;
    for (int i = 0; i < tst::TT_COUNT; i++) if (Chip(tst::kTypeNames[i], g_listType == i)) g_listType = i;
    if (Chip("Tylko niewysłane", g_listUnsent)) g_listUnsent = !g_listUnsent;
    EndPanel();
    auto& all = tst::Tickets();
    std::string del; int i = 0;
    for (auto it = all.rbegin(); it != all.rend(); ++it) {
        const auto& t = *it;
        if (g_listType >= 0 && t.type != g_listType) continue;
        if (g_listUnsent && t.sent) continue;
        if (!MatchQ(g_listSearch, t.title, t.targetName) && !MatchQ(g_listSearch, t.actual, t.target) && !MatchQ(g_listSearch, t.steps, t.uid)) continue;
        TicketCard(t, w, i++, del);
    }
    if (!i) ImGui::TextColored(C_DIM, "Brak zgłoszeń.");
    if (!del.empty()) tst::Remove(del);
    if (!tst::KnownIssues().empty()) {
        SectionLabel("Znane problemy (z GitHuba, w chwili wydania)");
        for (auto& k : tst::KnownIssues()) {
            PF(fSmall);
            ImGui::TextColored(k.state == "open" ? C_ST_BUG : C_ST_OK, "#%d", k.issue); ImGui::SameLine();
            ImGui::TextColored(C_DIM, "[%s]", TargetName(k.target).c_str()); ImGui::SameLine();
            ImGui::TextColored(C_TEXT, "%s", k.title.c_str());
            ImGui::PopFont();
        }
    }
}

void TabNew(float w) {
    Draft& d = g_draft;
    BeginPanel("##tn", nullptr, w);
    if (!d.editUid.empty()) { PF(fBold); ImGui::TextColored(C_ACC, "Edytujesz zgłoszenie %s", d.editUid.c_str()); ImGui::PopFont(); }
    FieldLabel("Rodzaj");
    const int types[] = { tst::TT_BUG, tst::TT_CRASH, tst::TT_PARTIAL, tst::TT_IDEA, tst::TT_QUESTION };
    for (int t : types) if (Chip(tst::kTypeNames[t], d.type == t)) d.type = t;
    ImGui::NewLine();
    FieldLabel("Czego dotyczy");
    {   // target picker with its own search box
        std::string cur = d.target.empty() ? "Ogólne (cała gra / trainer)" : d.targetName + "   #" + d.target;
        ImGui::SetNextItemWidth(w - 40 * S);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, C_CARD2); ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, R_BTN());
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10 * S, 7 * S));
        if (ImGui::BeginCombo("##tgt", cur.c_str(), ImGuiComboFlags_HeightLarge)) {
            InputBox("##tgf", g_targetFilter, sizeof g_targetFilter, "Szukaj opcji...", w - 70 * S);
            if (ImGui::Selectable("Ogólne (cała gra / trainer)", d.target.empty())) { d.target.clear(); d.targetName.clear(); }
            static const char* kAspects[] = { "UI (wygląd menu)", "UX (wygoda, czytelność)", "Wydajność (FPS, przycięcia)", "Stabilność (crashe)", "Teksty i tłumaczenie", "Sterowanie i skróty" };
            ImGui::TextColored(C_FAINT, "Aspekty");
            for (auto a : kAspects) { std::string id = std::string("aspekt:") + a; if (MatchQ(g_targetFilter, a, id) && ImGui::Selectable(a, d.target == id)) { d.target = id; d.targetName = TargetName(id); } }
            ImGui::TextColored(C_FAINT, "Kategorie i podkategorie");
            for (auto& sec : kSecs) {
                std::string kid = std::string("kat:") + sec.name;
                if (MatchQ(g_targetFilter, sec.name, kid) && ImGui::Selectable((std::string(sec.name) + " (cała kategoria)").c_str(), d.target == kid)) { d.target = kid; d.targetName = TargetName(kid); }
                for (auto sb : sec.subs) {
                    std::string pid = std::string("pod:") + sec.name + " › " + sb;
                    if (MatchQ(g_targetFilter, sb, pid) && ImGui::Selectable(("     " + std::string(sec.name) + " › " + sb).c_str(), d.target == pid)) { d.target = pid; d.targetName = TargetName(pid); }
                }
            }
            ImGui::TextColored(C_FAINT, "Opcje");
            for (auto& k : kIssues) if (MatchQ(g_targetFilter, k.text, k.id)) {
                std::string l = std::string(k.text) + "   #" + k.id;
                if (ImGui::Selectable(l.c_str(), d.target == k.id)) { d.target = k.id; d.targetName = k.text; }
            }
            for (auto& r : T.rows) {
                if (r.kind == K_VALUE || r.cat == "Konfiguracja" || !MatchQ(g_targetFilter, r.name, r.id)) continue;
                std::string l = r.cat + " › " + r.name + "   #" + r.id;
                if (ImGui::Selectable(l.c_str(), d.target == r.id)) { d.target = r.id; d.targetName = r.name; }
            }
            ImGui::EndCombo();
        }
        ImGui::PopStyleVar(2); ImGui::PopStyleColor();
    }
    if (!d.target.empty()) {   // what is already known - avoids duplicates
        auto known = tst::KnownFor(d.target); auto mine = tst::For(d.target);
        Row* r = T.Get(d.target);
        if (!known.empty() || !mine.empty() || (r && !r->hist.empty())) {
            PF(fSmall); ImGui::TextColored(C_ST_BUG, "Sprawdź, czy to już nie jest znane:"); ImGui::PopFont();
            HistoryBlock(d.target, w - 40 * S);
        }
    }
    bool bug = d.type != tst::TT_IDEA && d.type != tst::TT_QUESTION;
    float fw = w - 40 * S;
    FieldLabel("Tytuł (jedno zdanie)");
    InputBox("##tt", d.title, sizeof d.title, d.type == tst::TT_IDEA ? "np. Arena: tryb drużynowy z towarzyszami" : "np. Deszcz zaklęć wyłącza grę przy 10 celach", fw);
    if (d.type == tst::TT_IDEA) {
        FieldLabel("Opis pomysłu"); MultiBox("##ts", d.steps, sizeof d.steps, "Co dokładnie ma robić? Jak ma wyglądać w menu?", fw, 110 * S);
        FieldLabel("Po co / dla kogo"); MultiBox("##te", d.expected, sizeof d.expected, "Dlaczego to fajne / jaki problem rozwiązuje", fw, 60 * S);
    } else if (bug) {
        FieldLabel("Kroki - co robiłeś po kolei");
        MultiBox("##ts", d.steps, sizeof d.steps, "1. Wczytałem zapis w Starym Obozie\n2. Włączyłem ...\n3. ...", fw, 100 * S);
        FieldLabel("Co powinno się stać"); MultiBox("##te", d.expected, sizeof d.expected, "", fw, 50 * S);
        FieldLabel("Co się stało"); MultiBox("##ta", d.actual, sizeof d.actual, "", fw, 70 * S);
        FieldLabel("Waga"); for (int i = 0; i < 3; i++) { ImGui::PushID(i + 900); if (Chip(tst::kSeverityNames[i], d.severity == i)) d.severity = i; ImGui::PopID(); }
        ImGui::NewLine();
        FieldLabel("Jak często"); for (int i = 0; i < 4; i++) { ImGui::PushID(i + 950); if (Chip(tst::kReproNames[i], d.repro == i)) d.repro = i; ImGui::PopID(); }
        ImGui::NewLine();
    } else {
        FieldLabel("Treść"); MultiBox("##ts", d.steps, sizeof d.steps, "", fw, 110 * S);
    }
    FieldLabel("Dodatkowe uwagi (opcjonalnie)"); MultiBox("##tno", d.notes, sizeof d.notes, "", fw, 50 * S);
    FieldLabel("Załączniki - zrzuty ekranu, filmy, pliki (bardzo pomagają)");
    {
        ImVec2 ap = ImGui::GetCursorScreenPos();
        float a1 = ButtonW(ICN_FILE "  Wklej zrzut (Ctrl+V)");
        if (Button("##attp", ICN_FILE "  Wklej zrzut (Ctrl+V)", ap, false)) tst::PasteImageAsync();
        if (Button("##attf", ICN_PLUS "  Dodaj plik z dysku", ImVec2(ap.x + a1 + 8 * S, ap.y), false)) tst::PickFilesAsync();
        ImGui::SetCursorScreenPos(ImVec2(ap.x, ap.y + BtnH() + 6 * S));
        if (tst::AttachBusy()) { PF(fSmall); ImGui::TextColored(C_ACC, "Dodaję..."); ImGui::PopFont(); }
        int del = -1;
        for (int i = 0; i < (int)d.files.size(); i++) {
            ImGui::PushID(i + 8100);
            if (Chip((d.files[i] + "  ×").c_str(), true, ICN_FILE)) del = i;
            ImGui::PopID();
        }
        if (!d.files.empty()) ImGui::NewLine();
        if (del >= 0) { DeleteFileA((tst::AttachDir() + "\\" + d.files[del]).c_str()); d.files.erase(d.files.begin() + del); }
        if (d.files.empty()) { PF(fSmall); ImGui::TextColored(C_FAINT, "Zrób zrzut (Win+Shift+S albo PrtScr) i naciśnij Ctrl+V w tym oknie."); ImGui::PopFont(); }
    }
    ImVec2 cp = ImGui::GetCursorScreenPos();
    if (Check("##ctx", d.withContext, cp)) d.withContext = !d.withContext;
    ImGui::SameLine(0, 8 * S); ImGui::AlignTextToFramePadding();
    ImGui::TextColored(C_DIM, "Dołącz stan gry, włączone opcje i ostatnie zdarzenia (zalecane)");
    if (ImGui::IsItemHovered()) { PushTipStyle(); if (ImGui::BeginTooltip()) { PF(fSmall); ImGui::TextUnformatted(tst::BuildContext(d.target).c_str()); ImGui::PopFont(); ImGui::EndTooltip(); } PopTipStyle(); }
    ImGui::NewLine();
    bool ok = d.title[0] || d.steps[0] || d.actual[0];
    auto make = [&]() -> tst::Ticket& {
        tst::Ticket t; t.type = d.type; t.severity = d.severity; t.repro = d.repro; t.target = d.target; t.targetName = d.targetName;
        t.title = d.title; t.steps = d.steps; t.expected = d.expected; t.actual = d.actual; t.notes = d.notes;
        if (d.withContext) t.context = tst::BuildContext(d.target);
        t.files = d.files;
        return tst::Add(t);
    };
    ImVec2 bp = ImGui::GetCursorScreenPos();
    const char* saveLbl = d.editUid.empty() ? "Zapisz zgłoszenie" : "Zapisz zmiany";
    float b1 = ButtonW(saveLbl);
    if (Button("##tsave", saveLbl, bp, ok) && ok) {
        if (!d.editUid.empty()) {   // edit in place: same id, back to "not sent" so the change goes out with the next report
            if (tst::Ticket* t = tst::Find(d.editUid)) {
                t->type = d.type; t->severity = d.severity; t->repro = d.repro; t->target = d.target; t->targetName = d.targetName;
                t->title = d.title; t->steps = d.steps; t->expected = d.expected; t->actual = d.actual; t->notes = d.notes; t->files = d.files;
                if (d.withContext && t->context.empty()) t->context = tst::BuildContext(d.target);
                t->sent = false; tst::Save();
            }
            T.Toast("Zapisano zmiany w zgłoszeniu");
        } else { make(); T.Toast("Zapisano - wyślesz je w zakładce Wyślij"); }
        d = Draft{}; g_tTab = 1;
    }
    if (Button("##tclr", d.editUid.empty() ? "Wyczyść" : "Anuluj edycję", ImVec2(bp.x + b1 + 8 * S, bp.y), false)) d = Draft{};
    ImGui::SetCursorScreenPos(ImVec2(bp.x, bp.y + BtnH() + 4 * S));
    if (!ok) { PF(fSmall); ImGui::TextColored(C_FAINT, "Wpisz przynajmniej tytuł albo opis."); ImGui::PopFont(); }
    EndPanel();
}

void TabSend(float w) {
    int unsent = 0, total = (int)tst::Tickets().size();
    for (auto& t : tst::Tickets()) if (!t.sent) unsent++;
    BeginPanel("##sd", "Paczka dla autora", w);
    PF(fSmall); ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + w - 40 * S);
    ImGui::TextColored(C_DIM, "Jeden plik ze wszystkim: twoje niewysłane zgłoszenia, oceny, dziennik zdarzeń, błędy z logu i informacje o komputerze. "
                              "Wyślij go autorowi (Discord / mail) - z raportów powstają zgłoszenia na GitHubie. Po eksporcie zgłoszenia oznaczają się jako wysłane.");
    ImGui::PopTextWrapPos(); ImGui::PopFont();
    ImGui::TextColored(C_TEXT, "Niewysłane: %d  ·  wszystkie: %d", unsent, total);
    ImVec2 bp = ImGui::GetCursorScreenPos();
    float b1 = ButtonW("Eksportuj niewysłane"), b2 = ButtonW("Eksportuj wszystko");
    std::string path;
    if (Button("##ex1", "Eksportuj niewysłane", bp)) { tst::ExportReport(true, &path); g_lastExport = path; tst::OpenFolder(path); }
    if (Button("##ex2", "Eksportuj wszystko", ImVec2(bp.x + b1 + 8 * S, bp.y), false)) { tst::ExportReport(false, &path); g_lastExport = path; tst::OpenFolder(path); }
    if (!g_lastExport.empty() && Button("##exo", "Pokaż plik", ImVec2(bp.x + b1 + b2 + 16 * S, bp.y), false)) tst::OpenFolder(g_lastExport);
    ImGui::SetCursorScreenPos(ImVec2(bp.x, bp.y + BtnH() + 6 * S));
    if (!g_lastExport.empty()) { PF(fSmall); ImGui::TextColored(C_ST_OK, "Zapisano: %s", g_lastExport.c_str()); ImGui::PopFont(); }
    EndPanel();
    BeginPanel("##sd2", "Szybko: tekst na Discorda", w);
    PF(fSmall); ImGui::TextColored(C_DIM, "Kopiuje krótki tekst niewysłanych zgłoszeń. Plik raportu jest lepszy - ma dziennik, logi i załączniki."); ImGui::PopFont();
    bp = ImGui::GetCursorScreenPos();
    if (Button("##dc", "Kopiuj na Discorda", bp, false)) {
        std::string o = "**Raport testera " + tst::Nick() + " - build " + std::to_string(TRAINER_BUILD) + "**\n";
        int n = 0; for (auto& t : tst::Tickets()) if (!t.sent && t.type != tst::TT_OK) { o += "\n" + tst::ShortText(t); n++; }
        if (n) { tst::CopyToClipboard(o); T.Toast("Skopiowano " + std::to_string(n) + " zgłoszeń"); } else T.Toast("Brak niewysłanych zgłoszeń");
    }
    ImGui::SetCursorScreenPos(ImVec2(bp.x, bp.y + BtnH()));
    EndPanel();
}

char g_logFilter[64] = "";
void TabLog(float w) {
    BeginPanel("##lg", nullptr, w);
    PF(fSmall); ImGui::TextColored(C_DIM, "Automatyczny dziennik tej sesji: co włączałeś, co wykonałeś i jakie błędy złapał trainer. Dołącza się do zgłoszeń i raportu."); ImGui::PopFont();
    InputBox("##lgf", g_logFilter, sizeof g_logFilter, "Filtruj (np. rain, WYJĄTEK)...", w * 0.4f);
    EndPanel();
    auto ev = tst::EventsFor("", 400);
    PF(fSmall);
    for (auto it = ev.rbegin(); it != ev.rend(); ++it) {
        if (!MatchQ(g_logFilter, it->text, it->target)) continue;
        bool err = it->text.find("WYJĄTEK") != std::string::npos;
        ImGui::TextColored(C_FAINT, "%s", it->stamp.c_str() + 11); ImGui::SameLine();
        ImGui::TextColored(C_ACC, "%s", it->target.empty() ? "-" : it->target.c_str()); ImGui::SameLine();
        ImGui::TextColored(err ? C_ST_CRASH : C_TEXT, "%s", it->text.c_str());
    }
    if (ev.empty()) ImGui::TextColored(C_DIM, "Jeszcze nic się nie wydarzyło.");
    ImGui::PopFont();
}

void TesterPage(float w) {
    tst::Load();
    if (!g_nickInit) { g_nickInit = true; snprintf(g_nickBuf, sizeof g_nickBuf, "%s", tst::Nick().c_str()); }
    switch (g_tTab) {
    case 0: TabChecklist(w); break;
    case 1: TabTickets(w); break;
    case 2: TabNew(w); break;
    case 3: TabSend(w); break;
    default: TabLog(w); break;
    }
}

// tester centre in its own movable window next to the menu (build 7, tabs since build 8)
void TestsWindow(float dispW) {
    float dispH = ImGui::GetIO().DisplaySize.y;
    ImGui::SetNextWindowSize(ImVec2(760 * S, std::min(dispH * 0.85f, 800 * S)), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(dispW - 780 * S, dispH * 0.06f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(560 * S, 360 * S), ImVec2(dispW, dispH));
    ImGui::Begin("##g1tests", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollWithMouse);
    tst::Load();
    if (!g_nickInit) { g_nickInit = true; snprintf(g_nickBuf, sizeof g_nickBuf, "%s", tst::Nick().c_str()); }
    {   // attachments finished on the worker thread -> into the draft; Ctrl+V with an image in the clipboard = screenshot
        std::string msg;
        for (auto& f : tst::TakeAttachments(&msg)) g_draft.files.push_back(f);
        if (!msg.empty()) T.Toast(msg);
        if (g_tTab == 2 && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V, false) && IsClipboardFormatAvailable(CF_DIB)) tst::PasteImageAsync();
    }
    ImVec2 wp = ImGui::GetWindowPos(), ws = ImGui::GetWindowSize();
    float hcy = wp.y + 30 * S;
    PF(fTitle); float th = ImGui::CalcTextSize("Ag").y; ImGui::PopFont();
    IconAt(DL(), ImVec2(wp.x + 30 * S, hcy), ICN_CHECK, U32(C_ACC), 15 * S);
    TextClip(ImVec2(wp.x + 48 * S, std::floor(hcy - th * 0.5f)), fTitle, C_TEXT, "Centrum testera", wp.x + ws.x * 0.5f);
    // nick (required - every ticket is signed with it)
    float nw = 190 * S, nx = wp.x + ws.x - 60 * S - nw;
    ImGui::SetCursorScreenPos(ImVec2(nx, hcy - 16 * S));
    bool noNick = !g_nickBuf[0];
    if (noNick) DL()->AddRect(ImVec2(nx - 2 * S, hcy - 18 * S), ImVec2(nx + nw + 2 * S, hcy + 18 * S), U32(C_ST_BUG), R_BTN(), 1.5f * S);
    InputBox("##nick", g_nickBuf, sizeof g_nickBuf, "twój nick (wymagany)", nw);
    if (ImGui::IsItemDeactivatedAfterEdit() || (std::string(g_nickBuf) != tst::Nick() && !ImGui::IsItemActive())) tst::SetNick(g_nickBuf);
    ImVec2 xc(wp.x + ws.x - 30 * S, hcy);
    float r = 13 * S;
    bool hov = ImGui::IsMouseHoveringRect(ImVec2(xc.x - r, xc.y - r), ImVec2(xc.x + r, xc.y + r));
    if (hov) DL()->AddCircleFilled(xc, r, U32(C_HOVER), 20);
    IconAt(DL(), xc, ICN_TIMES, U32(hov ? C_TEXT : C_DIM), 13 * S);
    if (hov && ImGui::IsMouseClicked(0)) testsWin = false;
    // tabs
    int unsent = 0; for (auto& t : tst::Tickets()) if (!t.sent && t.type != tst::TT_OK) unsent++;
    std::string tabs[5] = { "Checklista", "Zgłoszenia (" + std::to_string(tst::Tickets().size()) + ")", "+ Nowe zgłoszenie",
                            unsent ? "Wyślij (" + std::to_string(unsent) + ")" : "Wyślij", "Dziennik" };
    const char* icons[5] = { ICN_LIST, ICN_FLAG, ICN_PENCIL, ICN_SIGNIN, ICN_CLOCK };
    ImGui::SetCursorScreenPos(ImVec2(wp.x + 22 * S, wp.y + 58 * S));
    for (int i = 0; i < 5; i++) { ImGui::PushID(i + 7700); if (Chip(tabs[i].c_str(), g_tTab == i, icons[i])) g_tTab = i; ImGui::PopID(); }
    ImGui::NewLine();
    float top = 100 * S;
    ImGui::SetCursorScreenPos(ImVec2(wp.x + 22 * S, wp.y + top));
    ImGui::BeginChild("##testsc", ImVec2(ws.x - 32 * S, ws.y - top - 14 * S), ImGuiChildFlags_None);
    if (noNick) { PF(fBold); ImGui::TextColored(C_ST_BUG, "Najpierw wpisz swój nick w prawym górnym rogu - każde zgłoszenie jest nim podpisane."); ImGui::PopFont(); }
    testsDrawing = true;
    TesterPage(ws.x - 50 * S);
    testsDrawing = false;
    {   // build 17: the mouse wheel scrolls the form even over text boxes and labels
        ImGuiIO& io = ImGui::GetIO(); ImGuiWindow* hw = GImGui->HoveredWindow; ImGuiWindow* me = ImGui::GetCurrentWindow();
        if (io.MouseWheel != 0 && hw && hw != me && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && hw->ScrollMax.y <= 0.5f)
            ImGui::SetScrollY(ImGui::GetScrollY() - io.MouseWheel * 90 * S);
    }
    ImGui::Dummy(ImVec2(0, 8 * S));
    ImGui::EndChild();
    ImGui::End();
}

// ------------------------------------------------------------------ overlays
// build 14: one label (shared by the in-game overlay and the previews in Wizualia)
void DrawLabel(ImDrawList* bg, const Label& L, bool lines, float ox, float oy, float W, float H) {
    auto P = [&](float x, float y) { return ImVec2(ox + x * W, oy + y * H); };
    ImVec2 a = P(L.x, L.y);
    ImU32 col = L.color;
    for (size_t i = 0; i + 3 < L.bones.size(); i += 4) bg->AddLine(P(L.bones[i], L.bones[i + 1]), P(L.bones[i + 2], L.bones[i + 3]), col, 1.4f * S);
    if (L.box == 1) bg->AddRect(P(L.bx0, L.by0), P(L.bx1, L.by1), col, 3 * S, 1.5f * S);
    if (L.box == 2) {
        static const int E[12][2] = { {0,1},{0,2},{0,4},{1,3},{1,5},{2,3},{2,6},{3,7},{4,5},{4,6},{5,7},{6,7} };
        for (auto& e : E) bg->AddLine(P(L.corners[e[0] * 2], L.corners[e[0] * 2 + 1]), P(L.corners[e[1] * 2], L.corners[e[1] * 2 + 1]), col, 1.3f * S);
    }
    if (lines && L.type == 0) {   // to the box, else to the skeleton's feet, else to the label
        ImVec2 e = L.box ? P((L.bx0 + L.bx1) * 0.5f, L.by1) : (L.anchor && !L.bones.empty() ? P(L.ax, L.ay) : a);
        bg->AddLine(ImVec2(ox + W * 0.5f, oy + H), e, (col & 0x00FFFFFF) | 0x90000000, 1.2f * S);
    }
    if (L.hasT) {   // build 16: arrow to whom this NPC attacks
        ImVec2 b = P(L.tx, L.ty);
        bg->AddLine(a, b, IM_COL32(255, 80, 80, 220), 2.f * S);
        float dx = b.x - a.x, dy = b.y - a.y, l = std::sqrt(dx * dx + dy * dy);
        if (l > 1) { dx /= l; dy /= l; bg->AddTriangleFilled(b, ImVec2(b.x - dx * 10 * S - dy * 5 * S, b.y - dy * 10 * S + dx * 5 * S), ImVec2(b.x - dx * 10 * S + dy * 5 * S, b.y - dy * 10 * S - dx * 5 * S), IM_COL32(255, 80, 80, 230)); }
    }
    if (L.text.empty() && L.sub.empty() && L.hp < 0 && L.lines.empty()) return;
    ImVec2 ts = L.text.empty() ? ImVec2(0, 0) : ImGui::CalcTextSize(L.text.c_str());
    ImVec2 ss = L.sub.empty() ? ImVec2(0, 0) : ImGui::CalcTextSize(L.sub.c_str());
    float bw = std::max(ts.x, ss.x);
    for (auto& l : L.lines) bw = std::max(bw, ImGui::CalcTextSize(l.c_str()).x);
    bw += 16 * S;
    float lh = ImGui::GetTextLineHeight();
    float bh = ts.y + ss.y + (L.hp >= 0 ? 7 * S : 0) + L.lines.size() * lh + 10 * S;
    float topY = (L.box ? oy + L.by0 * H : a.y) - bh - 4 * S;
    ImVec2 p0(a.x - bw * 0.5f, topY);
    bg->AddRectFilled(p0, ImVec2(p0.x + bw, p0.y + bh), IM_COL32(14, 13, 22, 190), R_BTN());
    float y = p0.y + 5 * S;
    if (!L.text.empty()) { bg->AddText(ImVec2(a.x - ts.x * 0.5f, y), col, L.text.c_str()); y += ts.y; }
    if (!L.sub.empty()) { bg->AddText(ImVec2(a.x - ss.x * 0.5f, y), IM_COL32(170, 170, 190, 255), L.sub.c_str()); y += ss.y; }
    if (L.hp >= 0) {
        float x0 = p0.x + 8 * S, x1 = p0.x + bw - 8 * S;
        bg->AddRectFilled(ImVec2(x0, y + 2 * S), ImVec2(x1, y + 5 * S), IM_COL32(60, 60, 80, 255), 2 * S);
        bg->AddRectFilled(ImVec2(x0, y + 2 * S), ImVec2(x0 + (x1 - x0) * L.hp, y + 5 * S), U32(C_ON), 2 * S);
        y += 7 * S;
    }
    for (auto& l : L.lines) { bg->AddText(ImVec2(p0.x + 8 * S, y), IM_COL32(210, 210, 225, 255), l.c_str()); y += lh; }
}
void DrawEsp(const Snapshot& s, float W, float H) {
    ImDrawList* bg = ImGui::GetBackgroundDrawList();
    PF(fSmall);
    for (auto& L : s.labels) DrawLabel(bg, L, s.espLines, 0, 0, W, H);
    ImGui::PopFont();
}
// build 14: radar. Dots are relative to the hero in world units; with "rotate" the hero's forward is up.
void DrawRadar(ImDrawList* dl, ImVec2 c, float R, const std::vector<RadarDot>& dots, float fx, float fz, float rx, float rz, bool tips) {
    float range = std::max(1.f, gx::Val("radarrange") * 100.f);
    bool rot = gx::On("radarrot");
    float alpha = std::max(0.f, std::min(1.f, gx::Val("radaralpha") / 100.f));
    dl->AddCircleFilled(c, R, IM_COL32(14, 13, 22, (int)(alpha * 230)), 64);
    dl->AddCircle(c, R, U32(C_ACC, 0.55f), 64, 1.6f * S);
    for (float k : { 0.333f, 0.666f }) dl->AddCircle(c, R * k, IM_COL32(255, 255, 255, 28), 48, 1.f);
    dl->AddLine(ImVec2(c.x - R, c.y), ImVec2(c.x + R, c.y), IM_COL32(255, 255, 255, 22));
    dl->AddLine(ImVec2(c.x, c.y - R), ImVec2(c.x, c.y + R), IM_COL32(255, 255, 255, 22));
    auto map = [&](float dx, float dz) {
        float sx, sy;
        if (rot) { sx = dx * rx + dz * rz; sy = -(dx * fx + dz * fz); } else { sx = dx; sy = -dz; }
        return ImVec2(c.x + sx / range * R, c.y + sy / range * R);
    };
    if (!rot) {   // north marker
        dl->AddText(fSmall, fSmall->LegacySize, ImVec2(c.x - 4 * S, c.y - R + 3 * S), IM_COL32(200, 200, 220, 160), "N");
    }
    // nearest npcs get names
    std::vector<std::pair<float, size_t>> nearDots;
    for (size_t i = 0; i < dots.size(); i++) if (dots[i].type == 0 || dots[i].type == 3) nearDots.push_back({ dots[i].x * dots[i].x + dots[i].z * dots[i].z, i });
    std::sort(nearDots.begin(), nearDots.end());
    std::set<size_t> named; if (gx::On("radarnames")) for (size_t k = 0; k < nearDots.size() && k < 6; k++) named.insert(nearDots[k].second);
    ImVec2 mouse = ImGui::GetIO().MousePos; int best = -1; float bestD = 9 * S * 9 * S;
    for (size_t i = 0; i < dots.size(); i++) {
        const RadarDot& d = dots[i];
        if (d.cat >= 0 && !gx::On(RadarCatId(d.cat, false))) continue;   // build 17: radar's own categories
        if (d.x * d.x + d.z * d.z > range * range) continue;
        ImVec2 p = map(d.x, d.z);
        ImU32 col = (d.color & 0x00FFFFFF) | 0xFF000000;
        if (d.type == 1) dl->AddRectFilled(ImVec2(p.x - 2.2f * S, p.y - 2.2f * S), ImVec2(p.x + 2.2f * S, p.y + 2.2f * S), col);
        else if (d.type == 2) { dl->AddQuadFilled(ImVec2(p.x, p.y - 4 * S), ImVec2(p.x + 4 * S, p.y), ImVec2(p.x, p.y + 4 * S), ImVec2(p.x - 4 * S, p.y), col); }
        else { dl->AddCircleFilled(p, (d.type == 3 ? 4.2f : 3.6f) * S, col, 12); if (d.type == 3) dl->AddCircle(p, 6.5f * S, col, 16, 1.2f * S); }
        if (named.count(i)) dl->AddText(fSmall, fSmall->LegacySize * 0.85f, ImVec2(p.x + 6 * S, p.y - 7 * S), IM_COL32(220, 220, 235, 220), d.name.c_str());
        float mx = mouse.x - p.x, my = mouse.y - p.y;
        if (tips && mx * mx + my * my < bestD) { bestD = mx * mx + my * my; best = (int)i; }
    }
    // hero arrow
    float ax = 0, ay = -1;
    if (!rot) { ax = fx; ay = -fz; }
    float px = -ay, py = ax;
    dl->AddTriangleFilled(ImVec2(c.x + ax * 8 * S, c.y + ay * 8 * S), ImVec2(c.x - ax * 5 * S + px * 5 * S, c.y - ay * 5 * S + py * 5 * S),
                          ImVec2(c.x - ax * 5 * S - px * 5 * S, c.y - ay * 5 * S - py * 5 * S), U32(C_TEXT));
    char rl[24]; snprintf(rl, sizeof rl, "%.0f m", range / 100.f);
    dl->AddText(fSmall, fSmall->LegacySize * 0.85f, ImVec2(c.x - TextW(rl, fSmall) * 0.42f, c.y + R - 16 * S), IM_COL32(200, 200, 220, 150), rl);
    if (best >= 0) { ImGui::SetTooltip("%s\n%s", dots[best].name.c_str(), dots[best].info.c_str()); }
}
void PreviewScene(float x, float y, float w, float h);
// ---- build 16: crosshair ---------------------------------------------------------------------------------
struct Xhair { std::string name; int style; float gap, len, thick, rot, spin; bool dot, outline; int r, g, b; bool builtin; float dotSize = 2; int dotShape = 0; };
Xhair XhCurrent() {
    using gx::Val; using gx::On;
    Xhair x{ "", (int)Val("xhstyle"), Val("xhgap"), Val("xhlen"), Val("xhthick"), Val("xhrot"), Val("xhspin"), On("xhdot"), On("xhoutline"), (int)Val("xhr"), (int)Val("xhg"), (int)Val("xhb"), false };
    x.dotSize = Val("xhdotsize"); x.dotShape = (int)Val("xhdotshape");
    return x;
}
void XhApply(const Xhair& x) {
    auto set = [](const char* id, float v) { if (Row* r = T.Get(id)) r->val = v; };
    auto tog = [](const char* id, bool v) { if (Row* r = T.Get(id)) r->on = v; };
    set("xhstyle", (float)x.style); set("xhgap", x.gap); set("xhlen", x.len); set("xhthick", x.thick); set("xhrot", x.rot); set("xhspin", x.spin);
    tog("xhdot", x.dot); tog("xhoutline", x.outline); set("xhr", (float)x.r); set("xhg", (float)x.g); set("xhb", (float)x.b);
    set("xhdotsize", x.dotSize); set("xhdotshape", (float)x.dotShape);
    T.Save();
}
std::vector<Xhair> g_xhPresets; bool g_xhLoaded = false;
std::string XhFile() { return DataPath("G1CrazyTrainer_celowniki.txt"); }
void XhLoad() {
    g_xhLoaded = true; g_xhPresets = {
        { "Klasyczny", 0, 4, 12, 2, 0, 0, false, true, 255, 250, 206, true },
        { "Kropka", 1, 0, 0, 2, 0, 0, true, true, 255, 255, 255, true, 4, 0 },
        { "Okrąg", 2, 10, 0, 2, 0, 0, true, true, 120, 255, 140, true, 2, 0 },
        { "Ukośny", 0, 5, 10, 2, 45, 0, true, true, 255, 90, 90, true, 2, 2 } };
    if (FILE* f = fopen(XhFile().c_str(), "rb")) {
        char line[512]; bool v2 = false;
        while (fgets(line, sizeof line, f)) {
            if (!strncmp(line, "#v2", 3)) { v2 = true; continue; }
            Xhair x{}; char nm[128] = ""; int dot = 0, ol = 0, dsh = 0; float dsz = 2;
            int n = sscanf(line, "%127[^|]|%d|%f|%f|%f|%f|%f|%d|%d|%d|%d|%d|%f|%d", nm, &x.style, &x.gap, &x.len, &x.thick, &x.rot, &x.spin, &dot, &ol, &x.r, &x.g, &x.b, &dsz, &dsh);
            if (n < 12) continue;
            if (!v2) { if (x.style == 2) { x.style = 0; x.rot += 45; } else if (x.style == 3) x.style = 2; else if (x.style == 4) x.style = 3; }
            x.name = nm; x.dot = dot; x.outline = ol; x.builtin = false; x.dotSize = n >= 13 ? dsz : 2; x.dotShape = n >= 14 ? dsh : 0;
            g_xhPresets.push_back(x);
        }
        fclose(f);
    }
}
void XhSave() {
    if (FILE* f = fopen(XhFile().c_str(), "wb")) {
        fprintf(f, "#v2\n");
        for (auto& x : g_xhPresets) if (!x.builtin)
            fprintf(f, "%s|%d|%.2f|%.2f|%.2f|%.2f|%.2f|%d|%d|%d|%d|%d|%.2f|%d\n", x.name.c_str(), x.style, x.gap, x.len, x.thick, x.rot, x.spin, (int)x.dot, (int)x.outline, x.r, x.g, x.b, x.dotSize, x.dotShape);
        fclose(f);
    }
}
void XhDot(ImDrawList* dl, ImVec2 c, float r, int shape, ImU32 col, ImU32 ol, bool outline, float sc) {
    float o = 1.3f * sc;
    if (shape == 1) { if (outline) dl->AddRectFilled(ImVec2(c.x - r - o, c.y - r - o), ImVec2(c.x + r + o, c.y + r + o), ol); dl->AddRectFilled(ImVec2(c.x - r, c.y - r), ImVec2(c.x + r, c.y + r), col); }
    else if (shape == 2) {
        if (outline) dl->AddQuadFilled(ImVec2(c.x, c.y - r - o * 1.4f), ImVec2(c.x + r + o * 1.4f, c.y), ImVec2(c.x, c.y + r + o * 1.4f), ImVec2(c.x - r - o * 1.4f, c.y), ol);
        dl->AddQuadFilled(ImVec2(c.x, c.y - r), ImVec2(c.x + r, c.y), ImVec2(c.x, c.y + r), ImVec2(c.x - r, c.y), col);
    } else { if (outline) dl->AddCircleFilled(c, r + o, ol, 20); dl->AddCircleFilled(c, r, col, 20); }
}
void DrawXhair(ImDrawList* dl, ImVec2 c, const Xhair& x, float sc, float alpha) {
    float ang = (x.rot + (float)std::fmod(NowSec() * x.spin, 360.0)) * 0.0174533f;
    ImU32 col = IM_COL32(x.r, x.g, x.b, (int)(alpha * 255)), ol = IM_COL32(0, 0, 0, (int)(alpha * 200));
    float g = x.gap * sc, l = x.len * sc, t = std::max(1.f, x.thick * sc);
    switch (x.style) {
    case 0:
        for (int i = 0; i < 4; i++) {
            float a = i * 1.5708f + ang, ca = std::cos(a), sa = std::sin(a);
            ImVec2 p0(c.x + ca * g, c.y + sa * g), p1(c.x + ca * (g + l), c.y + sa * (g + l));
            if (x.outline) dl->AddLine(ImVec2(p0.x - ca * sc, p0.y - sa * sc), ImVec2(p1.x + ca * sc, p1.y + sa * sc), ol, t + 2 * sc);
            dl->AddLine(p0, p1, col, t);
        }
        break;
    case 1: XhDot(dl, c, std::max(1.f, x.dotSize * sc), x.dotShape, col, ol, x.outline, sc); return;   // the dot is the whole crosshair
    case 2: if (x.outline) dl->AddCircle(c, std::max(2.f, g), ol, 40, t + 2 * sc); dl->AddCircle(c, std::max(2.f, g), col, 40, t); break;
    case 3:
        for (int i = 0; i < 4; i++) {   // corner brackets as one polyline each (no overlap at any thickness)
            float a = 0.7854f + i * 1.5708f + ang, r = g + t * 0.5f;
            ImVec2 k(c.x + std::cos(a) * r * 1.4142f, c.y + std::sin(a) * r * 1.4142f);
            float a1 = a + 3.1416f - 0.7854f, a2 = a + 3.1416f + 0.7854f;
            ImVec2 pts[3] = { ImVec2(k.x + std::cos(a1) * l, k.y + std::sin(a1) * l), k, ImVec2(k.x + std::cos(a2) * l, k.y + std::sin(a2) * l) };
            if (x.outline) dl->AddPolyline(pts, 3, ol, t + 2 * sc);
            dl->AddPolyline(pts, 3, col, t);
        }
        break;
    default: break;
    }
    if (x.dot) XhDot(dl, c, std::max(1.f, x.dotSize * sc), x.dotShape, col, ol, x.outline, sc);
}
// build 17: the Celownik card with only the settings of the chosen shape
std::vector<Row*> XhRows() {
    std::vector<const char*> ids = { "xhair", "xhstyle" };
    int st = (int)gx::Val("xhstyle");
    if (st == 0) { for (auto id : { "xhgap", "xhlen", "xhthick", "xhrot", "xhspin", "xhdot" }) ids.push_back(id); if (gx::On("xhdot")) { ids.push_back("xhdotsize"); ids.push_back("xhdotshape"); } }
    if (st == 1) { ids.push_back("xhdotsize"); ids.push_back("xhdotshape"); }
    if (st == 2) { for (auto id : { "xhgap", "xhthick", "xhdot" }) ids.push_back(id); if (gx::On("xhdot")) { ids.push_back("xhdotsize"); ids.push_back("xhdotshape"); } }
    if (st == 3) { for (auto id : { "xhgap", "xhlen", "xhthick", "xhrot", "xhspin", "xhdot" }) ids.push_back(id); if (gx::On("xhdot")) { ids.push_back("xhdotsize"); ids.push_back("xhdotshape"); } }
    ids.push_back("xhalpha"); ids.push_back("xhoutline");
    std::vector<Row*> v; for (auto id : ids) if (Row* r = T.Get(id)) v.push_back(r);
    return v;
}
float XhColor(float x, float y, float cw, bool draw) {
    float h = 44 * S; if (!draw) return h;
    Text(ImVec2(x + 18 * S, y + 12 * S), fReg, C_TEXT, "Kolor");
    float col[3] = { gx::Val("xhr") / 255.f, gx::Val("xhg") / 255.f, gx::Val("xhb") / 255.f };
    char hex[12]; snprintf(hex, sizeof hex, "#%02X%02X%02X", (int)gx::Val("xhr"), (int)gx::Val("xhg"), (int)gx::Val("xhb"));
    TextV(x + cw - 56 * S - TextW(hex, fSmall), y + 22 * S, fSmall, C_DIM, hex);
    ImGui::SetCursorScreenPos(ImVec2(x + cw - 18 * S - 24 * S, y + 10 * S));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12 * S);
    if (ImGui::ColorEdit3("##xhc", col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
        if (Row* r = T.Get("xhr")) r->val = std::round(col[0] * 255); if (Row* r = T.Get("xhg")) r->val = std::round(col[1] * 255); if (Row* r = T.Get("xhb")) r->val = std::round(col[2] * 255);
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) T.Save();
    ImGui::PopStyleVar();
    return h;
}
float XhPreview(float x, float y, float cw, bool draw) {
    float h = 170 * S;
    if (!draw) return h;
    float px = x + 18 * S, pw = cw - 36 * S, py = y + 8 * S, ph = h - 16 * S;
    PreviewScene(px, py, pw, ph);
    Text(ImVec2(px + 10 * S, py + 8 * S), fSmall, C_FAINT, "podgląd na żywo");
    DrawXhair(DL(), ImVec2(px + pw * 0.5f, py + ph * 0.5f), XhCurrent(), S * 1.4f, gx::Val("xhalpha") / 100.f);
    return h;
}
char g_xhName[48] = "";
float XhPresets(float x, float y, float cw, bool draw) {
    if (!g_xhLoaded) XhLoad();
    int cols = 4; float tile = (cw - 36 * S - (cols - 1) * 8 * S) / cols, th = 92 * S;
    int n = (int)g_xhPresets.size();
    float h = 10 * S + ((n + cols - 1) / cols) * (th + 8 * S) + 46 * S;
    if (!draw) return h;
    int del = -1;
    Xhair cur = XhCurrent();
    for (int i = 0; i < n; i++) {
        auto& p = g_xhPresets[i];
        ImVec2 a(x + 18 * S + (i % cols) * (tile + 8 * S), y + 10 * S + (i / cols) * (th + 8 * S)), b(a.x + tile, a.y + th);
        bool same = p.style == cur.style && std::fabs(p.gap - cur.gap) < 0.01f && std::fabs(p.len - cur.len) < 0.01f && std::fabs(p.rot - cur.rot) < 0.01f && p.r == cur.r && p.g == cur.g && p.b == cur.b;
        ImGui::SetCursorScreenPos(a); ImGui::PushID(i + 9100);
        bool click = ImGui::InvisibleButton("##xp", ImVec2(tile, th)); bool hv = ImGui::IsItemHovered();
        DL()->AddRectFilled(a, b, U32(same ? Mix(C_CARD2, C_ACC, 0.25f) : hv ? C_HOVER : C_CARD2), R_ITEM());
        if (same) DL()->AddRect(a, b, U32(C_ACC), R_ITEM(), 1.5f * S);
        DrawXhair(DL(), ImVec2((a.x + b.x) * 0.5f, a.y + 36 * S), p, S * 0.9f, 1.f);
        float tw = TextW(p.name.c_str(), fSmall);
        Text(ImVec2((a.x + b.x) * 0.5f - tw * 0.5f, b.y - 24 * S), fSmall, same ? C_TEXT : C_DIM, p.name.c_str());
        if (click) { XhApply(p); if (Row* r = T.Get("xhair")) if (!r->on) ApplyRow(*r); }
        if (!p.builtin && IconButton("##xd", ICN_TIMES, "Usuń preset", ImVec2(b.x - 24 * S, a.y + 4 * S), 20 * S, C_DIM)) del = i;
        ImGui::PopID();
    }
    if (del >= 0) { g_xhPresets.erase(g_xhPresets.begin() + del); XhSave(); }
    float by = y + 10 * S + ((n + cols - 1) / cols) * (th + 8 * S) + 6 * S;
    ImGui::SetCursorScreenPos(ImVec2(x + 18 * S, by));
    float bw = ButtonW("Zapisz jako preset");
    InputBox("##xhn", g_xhName, sizeof g_xhName, "nazwa presetu", cw - 36 * S - bw - 8 * S);
    if (Button("##xhs", "Zapisz jako preset", ImVec2(x + cw - 18 * S - bw, by), g_xhName[0] != 0) && g_xhName[0]) {
        Xhair c2 = XhCurrent(); c2.name = g_xhName; for (auto& ch : c2.name) if (ch == '|') ch = '/';
        g_xhPresets.push_back(c2); XhSave(); g_xhName[0] = 0; T.Toast("Zapisano preset celownika");
    }
    return h;
}
// ---- build 17: generic presets for a set of rows (Grafika) ------------------------------------------------
struct RowPreset { std::string name; std::vector<std::pair<std::string, float>> v; bool builtin; };
struct PresetSet { std::string file; std::vector<std::string> ids; std::vector<RowPreset> list; bool loaded = false; char name[48] = ""; };
void PresetLoad(PresetSet& ps, const std::vector<RowPreset>& builtins) {
    ps.loaded = true; ps.list = builtins;
    if (FILE* f = fopen(DataPath(ps.file.c_str()).c_str(), "rb")) {
        char line[2048];
        while (fgets(line, sizeof line, f)) {
            std::string l = line; while (!l.empty() && (l.back() == '\n' || l.back() == '\r')) l.pop_back();
            size_t bar = l.find('|'); if (bar == std::string::npos) continue;
            RowPreset p{ l.substr(0, bar), {}, false };
            std::string rest = l.substr(bar + 1); size_t pos = 0;
            while (pos < rest.size()) { size_t e = rest.find(';', pos); std::string kv = rest.substr(pos, e == std::string::npos ? std::string::npos : e - pos);
                size_t eq = kv.find('='); if (eq != std::string::npos) p.v.push_back({ kv.substr(0, eq), (float)atof(kv.substr(eq + 1).c_str()) });
                if (e == std::string::npos) break; pos = e + 1; }
            ps.list.push_back(p);
        }
        fclose(f);
    }
}
void PresetSave(PresetSet& ps) {
    if (FILE* f = fopen(DataPath(ps.file.c_str()).c_str(), "wb")) {
        for (auto& p : ps.list) if (!p.builtin) { fprintf(f, "%s|", p.name.c_str()); for (auto& kv : p.v) fprintf(f, "%s=%.3f;", kv.first.c_str(), kv.second); fprintf(f, "\n"); }
        fclose(f);
    }
}
void PresetApply(const RowPreset& p) {
    for (auto& kv : p.v) if (Row* r = T.Get(kv.first)) { if (r->kind == K_TOGGLE) r->on = kv.second > 0.5f; else r->val = kv.second; }
    T.Save(); T.Toast("Preset: " + p.name);
}
float PresetsCard(PresetSet& ps, float x, float y, float cw, bool draw) {
    int cols = 3; float tile = (cw - 36 * S - (cols - 1) * 8 * S) / cols, th = 40 * S;
    int n = (int)ps.list.size();
    float h = 10 * S + ((n + cols - 1) / cols) * (th + 8 * S) + 46 * S;
    if (!draw) return h;
    int del = -1;
    for (int i = 0; i < n; i++) {
        auto& p = ps.list[i];
        ImVec2 a(x + 18 * S + (i % cols) * (tile + 8 * S), y + 10 * S + (i / cols) * (th + 8 * S)), b(a.x + tile, a.y + th);
        ImGui::SetCursorScreenPos(a); ImGui::PushID(i + 9500 + (int)ps.file.size());
        bool click = ImGui::InvisibleButton("##pp", ImVec2(tile, th)); bool hv = ImGui::IsItemHovered();
        DL()->AddRectFilled(a, b, U32(hv ? C_HOVER : C_CARD2), R_ITEM());
        TextV(a.x + 12 * S, (a.y + b.y) * 0.5f, fReg, C_TEXT, p.name.c_str());
        if (click) PresetApply(p);
        if (!p.builtin && IconButton("##pd", ICN_TIMES, "Usuń preset", ImVec2(b.x - 26 * S, a.y + (th - 20 * S) * 0.5f), 20 * S, C_DIM)) del = i;
        ImGui::PopID();
    }
    if (del >= 0) { ps.list.erase(ps.list.begin() + del); PresetSave(ps); }
    float by = y + 10 * S + ((n + cols - 1) / cols) * (th + 8 * S) + 6 * S;
    ImGui::SetCursorScreenPos(ImVec2(x + 18 * S, by));
    float bw = ButtonW("Zapisz obecne jako preset");
    ImGui::PushID(ps.file.c_str());
    InputBox("##psn", ps.name, sizeof ps.name, "nazwa presetu", cw - 36 * S - bw - 8 * S);
    if (Button("##pss", "Zapisz obecne jako preset", ImVec2(x + cw - 18 * S - bw, by), ps.name[0] != 0) && ps.name[0]) {
        RowPreset p{ ps.name, {}, false }; for (auto& c : p.name) if (c == '|' || c == ';') c = '/';
        for (auto& id : ps.ids) if (Row* r = T.Get(id)) p.v.push_back({ id, r->kind == K_TOGGLE ? (r->on ? 1.f : 0.f) : r->val });
        ps.list.push_back(p); PresetSave(ps); ps.name[0] = 0; T.Toast("Zapisano preset");
    }
    ImGui::PopID();
    return h;
}
PresetSet g_gfxPresets{ "G1CrazyTrainer_grafika.txt", { "gfx", "gfxmode", "gfxbright", "gfxgamma", "gfxcontrast", "gfxsat", "gfxtemp", "gfxsharp", "gfxvign",
    "gdx", "gdxdraw", "gdxsmall", "gdxnpc", "gdxfx", "gdxsect", "gdxfog", "gdxsun", "gdxshadow", "gdxwind", "gdxscatter" } };
const std::vector<RowPreset>& GfxBuiltins() {
    static const std::vector<RowPreset> b = {
        { "Domyślne", { { "gfx", 0 }, { "gfxmode", 0 }, { "gfxbright", 0 }, { "gfxgamma", 100 }, { "gfxcontrast", 100 }, { "gfxsat", 100 }, { "gfxtemp", 0 }, { "gfxsharp", 0 }, { "gfxvign", 0 }, { "gdx", 0 } }, true },
        { "Jasne noce", { { "gfx", 1 }, { "gfxmode", 0 }, { "gfxbright", 4 }, { "gfxgamma", 150 }, { "gfxcontrast", 100 }, { "gfxsat", 105 }, { "gfxtemp", 0 }, { "gfxsharp", 10 }, { "gfxvign", 0 } }, true },
        { "Kinowy", { { "gfx", 1 }, { "gfxmode", 0 }, { "gfxbright", -2 }, { "gfxgamma", 95 }, { "gfxcontrast", 118 }, { "gfxsat", 85 }, { "gfxtemp", 15 }, { "gfxsharp", 15 }, { "gfxvign", 40 } }, true },
        { "Żywe kolory", { { "gfx", 1 }, { "gfxmode", 0 }, { "gfxbright", 0 }, { "gfxgamma", 105 }, { "gfxcontrast", 110 }, { "gfxsat", 145 }, { "gfxtemp", 5 }, { "gfxsharp", 30 }, { "gfxvign", 0 } }, true },
        { "Daleki widok", { { "gdx", 1 }, { "gdxdraw", 1200 }, { "gdxsmall", 300 }, { "gdxnpc", 300 }, { "gdxfx", 300 }, { "gdxsect", 12 }, { "gdxfog", 50 } }, true } };
    return b;
}
// build 16: Shift+F11 - drag a rectangle over the frozen frame; Esc cancels
void ShotSelectOverlay(float W, float H) {
    ImDrawList* fg = ImGui::GetForegroundDrawList();
    if (void* tex = ShotHeldTex()) fg->AddImage((ImTextureID)(intptr_t)tex, ImVec2(0, 0), ImVec2(W, H));   // the frozen frame (with the menu)
    ImGuiIO& io = ImGui::GetIO();
    static bool drag = false; static ImVec2 a;
    ImVec2 m = io.MousePos;
    if (ImGui::IsMouseClicked(0)) { drag = true; a = m; }
    ImU32 dim = IM_COL32(0, 0, 0, 120);
    if (drag) {
        ImVec2 p0(std::min(a.x, m.x), std::min(a.y, m.y)), p1(std::max(a.x, m.x), std::max(a.y, m.y));
        fg->AddRectFilled(ImVec2(0, 0), ImVec2(W, p0.y), dim); fg->AddRectFilled(ImVec2(0, p1.y), ImVec2(W, H), dim);
        fg->AddRectFilled(ImVec2(0, p0.y), ImVec2(p0.x, p1.y), dim); fg->AddRectFilled(ImVec2(p1.x, p0.y), ImVec2(W, p1.y), dim);
        fg->AddRect(p0, p1, U32(C_ACC), 0.f, 2 * S);
        char b[48]; snprintf(b, sizeof b, "%.0f x %.0f", p1.x - p0.x, p1.y - p0.y);
        fg->AddText(fSmall, fSmall->LegacySize, ImVec2(p0.x + 4 * S, p1.y + 4 * S), U32(C_TEXT), b);
        if (ImGui::IsMouseReleased(0)) {
            drag = false;
            int hw, hh; ShotSize(hw, hh);
            float sx = W > 0 ? hw / W : 1, sy = H > 0 ? hh / H : 1;
            ShotCommit((int)(p0.x * sx), (int)(p0.y * sy), (int)(p1.x * sx), (int)(p1.y * sy));
            T.Toast("Zrzut obszaru: w schowku i w folderze G1CrazyTrainer_zrzuty");
        }
    } else fg->AddRectFilled(ImVec2(0, 0), ImVec2(W, H), dim);
    const char* hint = "Zaznacz obszar myszą  ·  Esc = anuluj";
    ImVec2 ts = fBold->CalcTextSizeA(fBold->LegacySize, FLT_MAX, 0, hint);
    fg->AddRectFilled(ImVec2(W * 0.5f - ts.x * 0.5f - 14 * S, 24 * S), ImVec2(W * 0.5f + ts.x * 0.5f + 14 * S, 24 * S + ts.y + 16 * S), U32(C_WIN), R_ITEM());
    fg->AddText(fBold, fBold->LegacySize, ImVec2(W * 0.5f - ts.x * 0.5f, 32 * S), U32(C_TEXT), hint);
}
// ---- build 16: widget dock, compass, direction arrows ------------------------------------------------------
float g_fps = 0;
std::vector<float> g_ft(120, 16.f); size_t g_ftPos = 0;   // build 17: frame times for the info graph
float DrawDock(ImDrawList* dl, ImVec2 origin, float wPx, const Snapshot& s, bool tips, float sc, bool dryRun = false) {
    float a = std::max(0.2f, std::min(1.f, gx::Val("walpha") / 100.f));
    float pad = 10 * sc, gap = 8 * sc, y = origin.y, x = origin.x;
    ImU32 bg = IM_COL32(20, 18, 28, (int)(a * 225)), brd = IM_COL32(255, 255, 255, (int)(a * 22));
    ImFont* fs = fSmall; float fsz = fSmall->LegacySize * sc / S, fbz = fBold->LegacySize * sc / S;
    auto box = [&](float bx, float bw, float bh) { if (!dryRun) { dl->AddRectFilled(ImVec2(bx, y), ImVec2(bx + bw, y + bh), bg, 8 * sc); dl->AddRect(ImVec2(bx, y), ImVec2(bx + bw, y + bh), brd, 8 * sc, 1.f); } };
    auto title = [&](float bx, float bw, const char* t, const char* v, ImU32 vc) {
        if (dryRun) return;
        dl->AddText(fs, fsz, ImVec2(bx + pad, y + pad * 0.7f), IM_COL32(170, 166, 190, 255), t);
        if (v) { ImVec2 ts = fs->CalcTextSizeA(fsz, FLT_MAX, 0, v); dl->AddText(fs, fsz, ImVec2(bx + bw - pad - ts.x, y + pad * 0.7f), vc, v); }
    };
    char b[160];
    if (gx::On("whp")) {
        float bh = (gx::On("wmana") ? 58 : 40) * sc; box(x, wPx, bh);
        float f = s.hpmax > 0 ? (float)s.hp / s.hpmax : 0; snprintf(b, sizeof b, "%d / %d", s.hp, s.hpmax);
        title(x, wPx, "Zdrowie", b, IM_COL32(235, 235, 245, 255));
        if (!dryRun) {
            float by = y + 26 * sc;
            dl->AddRectFilled(ImVec2(x + pad, by), ImVec2(x + wPx - pad, by + 5 * sc), IM_COL32(60, 40, 48, 255), 3 * sc);
            dl->AddRectFilled(ImVec2(x + pad, by), ImVec2(x + pad + (wPx - 2 * pad) * f, by + 5 * sc), IM_COL32(220, 50, 60, 255), 3 * sc);
            if (gx::On("wmana")) {
                float fm = s.manamax > 0 ? (float)s.mana / s.manamax : 0; by += 16 * sc;
                dl->AddRectFilled(ImVec2(x + pad, by), ImVec2(x + wPx - pad, by + 5 * sc), IM_COL32(40, 44, 70, 255), 3 * sc);
                dl->AddRectFilled(ImVec2(x + pad, by), ImVec2(x + pad + (wPx - 2 * pad) * fm, by + 5 * sc), IM_COL32(70, 120, 240, 255), 3 * sc);
            }
        }
        y += bh + gap;
    }
    if (gx::On("radar")) {
        float bh = wPx; box(x, wPx, bh);
        snprintf(b, sizeof b, "%d", (int)s.radar.size()); title(x, wPx, "Radar", b, IM_COL32(235, 235, 245, 255));
        if (!dryRun) DrawRadar(dl, ImVec2(x + wPx * 0.5f, y + bh * 0.5f + 8 * sc), wPx * 0.5f - 22 * sc, s.radar, s.rfx, s.rfz, s.rrx, s.rrz, tips);
        y += bh + gap;
    }
    if (gx::On("wthreat")) {
        float bh = 50 * sc; box(x, wPx, bh);
        snprintf(b, sizeof b, "%d", s.thrCount); title(x, wPx, "Zagrożenia", s.thrCount ? b : "czysto", s.thrCount ? IM_COL32(255, 110, 110, 255) : IM_COL32(120, 220, 140, 255));
        if (!dryRun && s.thrCount) {
            snprintf(b, sizeof b, "%s", s.thrName.c_str()); dl->AddText(fs, fsz, ImVec2(x + pad, y + 28 * sc), IM_COL32(235, 235, 245, 255), b);
            snprintf(b, sizeof b, "%.0f m", s.thrDist); ImVec2 ts = fs->CalcTextSizeA(fsz, FLT_MAX, 0, b);
            dl->AddText(fs, fsz, ImVec2(x + wPx - pad - ts.x, y + 28 * sc), IM_COL32(170, 166, 190, 255), b);
        }
        y += bh + gap;
    }
    if (gx::On("wxp")) {   // build 17
        float bh = 40 * sc; box(x, wPx, bh);
        float f = s.expNext > 0 ? std::max(0.f, std::min(1.f, (float)s.exp / s.expNext)) : 0;
        snprintf(b, sizeof b, "poziom %d  ·  %d / %d", s.level, s.exp, s.expNext); title(x, wPx, "Doświadczenie", b, IM_COL32(235, 235, 245, 255));
        if (!dryRun) { float by = y + 26 * sc; dl->AddRectFilled(ImVec2(x + pad, by), ImVec2(x + wPx - pad, by + 5 * sc), IM_COL32(50, 46, 30, 255), 3 * sc);
            dl->AddRectFilled(ImVec2(x + pad, by), ImVec2(x + pad + (wPx - 2 * pad) * f, by + 5 * sc), IM_COL32(230, 190, 70, 255), 3 * sc); }
        y += bh + gap;
    }
    bool wp = gx::On("wweapon"), fp = gx::On("wfps");
    if (wp || fp) {
        float bh = 50 * sc, bw = (wp && fp) ? (wPx - gap) * 0.5f : wPx; float bx = x;
        auto small = [&](const char* t, const char* v) {
            box(bx, bw, bh); title(bx, bw, t, nullptr, 0);
            if (!dryRun) { ImVec2 ts = fBold->CalcTextSizeA(fbz, FLT_MAX, 0, v); dl->AddText(fBold, fbz, ImVec2(bx + bw * 0.5f - std::min(ts.x, bw - 2 * pad) * 0.5f, y + 26 * sc), IM_COL32(235, 235, 245, 255), v); }
            bx += bw + gap;
        };
        if (wp) small("Broń", s.weapon.empty() ? "-" : s.weapon.c_str());
        if (fp) { snprintf(b, sizeof b, "%.0f", g_fps); small("FPS", b); }
        y += bh + gap;
    }
    if (gx::On("wammo")) {   // build 17
        float bh = 50 * sc, bw = (wPx - gap) * 0.5f, bx = x;
        for (int k = 0; k < 2; k++) {
            box(bx, bw, bh); title(bx, bw, k ? "Bełty" : "Strzały", nullptr, 0);
            if (!dryRun) { snprintf(b, sizeof b, "%d", k ? s.bolts : s.arrows); ImVec2 ts = fBold->CalcTextSizeA(fbz, FLT_MAX, 0, b);
                dl->AddText(fBold, fbz, ImVec2(bx + bw * 0.5f - ts.x * 0.5f, y + 26 * sc), (k ? s.bolts : s.arrows) ? IM_COL32(235, 235, 245, 255) : IM_COL32(255, 110, 110, 255), b); }
            bx += bw + gap;
        }
        y += bh + gap;
    }
    if (gx::On("hud")) {
        std::vector<std::pair<std::string, std::string>> L;
        if (gx::On("hudpos")) { snprintf(b, sizeof b, "%.0f  %.0f  %.0f", s.px, s.py, s.pz); L.push_back({ "Pozycja", b }); }
        if (gx::On("hudspeed")) { static double lt = 0; static float lx = 0, lz = 0, sp = 0; double now = NowSec();
            if (lt > 0 && now - lt > 0.05) { float dx = s.px - lx, dz = s.pz - lz; float v = std::sqrt(dx * dx + dz * dz) / 100.f / (float)(now - lt); if (v < 200) sp += (v - sp) * 0.35f; lt = now; lx = s.px; lz = s.pz; } else if (lt == 0) { lt = now; lx = s.px; lz = s.pz; }
            snprintf(b, sizeof b, "%.1f m/s", sp); L.push_back({ "Prędkość", b }); }
        if (gx::On("hudtime")) { snprintf(b, sizeof b, "dzień %d, %02d:%02d", s.day, s.hour, s.minute); L.push_back({ "Czas", b }); }
        if (gx::On("hudworld")) L.push_back({ "Świat", s.world.empty() ? "-" : s.world });
        if (gx::On("hudstats")) { snprintf(b, sizeof b, "%d/%d  ·  %d/%d", s.hp, s.hpmax, s.mana, s.manamax); L.push_back({ "HP · mana", b }); }
        if (gx::On("hudtarget")) { if (s.tValid) { snprintf(b, sizeof b, "%s %d/%d", s.tName.c_str(), s.tHp, s.tHpMax); L.push_back({ "Cel", b }); } else L.push_back({ "Cel", "-" }); }
        if (gx::On("hudfps")) { snprintf(b, sizeof b, "%.0f (%.1f ms)", g_fps, g_fps > 0 ? 1000.f / g_fps : 0.f); L.push_back({ "FPS", b }); }
        // build 17: more lines
        float avg = 0, worst = 0; for (float v : g_ft) { avg += v; worst = std::max(worst, v); } avg /= (float)g_ft.size();
        if (gx::On("hudframe")) { snprintf(b, sizeof b, "%.1f ms  (max %.1f)", avg, worst); L.push_back({ "Czas klatki", b }); }
        if (gx::On("hudtick")) { snprintf(b, sizeof b, "%.2f ms", s.tickMs); L.push_back({ "Trener", b }); }
        if (gx::On("hudmem")) { snprintf(b, sizeof b, "%.0f MB", s.memMB); L.push_back({ "Pamięć", b }); }
        if (gx::On("hudcount")) { snprintf(b, sizeof b, "%d postaci  ·  %d przedm.", s.npcCount, s.itemCount); L.push_back({ "Świat", b }); }
        if (gx::On("hudheading")) { float hd = std::atan2(s.rfx, s.rfz) * 57.29578f; if (hd < 0) hd += 360; snprintf(b, sizeof b, "%.0f°", hd); L.push_back({ "Kierunek", b }); }
        if (gx::On("hudclock")) { SYSTEMTIME st; GetLocalTime(&st); snprintf(b, sizeof b, "%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond); L.push_back({ "Godzina", b }); }
        bool graph = gx::On("hudgraph");
        if (!L.empty() || graph) {
            float lh = fsz + 5 * sc, gh = graph ? 40 * sc : 0, bh = 26 * sc + L.size() * lh + 4 * sc + gh; box(x, wPx, bh); title(x, wPx, "Informacje", nullptr, 0);
            if (!dryRun && graph) {   // frame time graph (ms, 0..50)
                float gx0 = x + pad, gx1 = x + wPx - pad, gy0 = y + bh - gh - 2 * sc, gy1 = y + bh - 6 * sc;
                dl->AddRectFilled(ImVec2(gx0, gy0), ImVec2(gx1, gy1), IM_COL32(255, 255, 255, 12), 3 * sc);
                float lim = std::max(20.f, worst * 1.2f);
                for (size_t i = 1; i < g_ft.size(); i++) {
                    size_t a = (g_ftPos + i - 1) % g_ft.size(), c2 = (g_ftPos + i) % g_ft.size();
                    float xa = gx0 + (gx1 - gx0) * (i - 1) / (g_ft.size() - 1), xb = gx0 + (gx1 - gx0) * i / (g_ft.size() - 1);
                    float ya = gy1 - (gy1 - gy0) * std::min(1.f, g_ft[a] / lim), yb = gy1 - (gy1 - gy0) * std::min(1.f, g_ft[c2] / lim);
                    dl->AddLine(ImVec2(xa, ya), ImVec2(xb, yb), g_ft[c2] > 33 ? IM_COL32(255, 110, 110, 255) : IM_COL32(120, 220, 140, 255), 1.2f * sc);
                }
            }
            if (!dryRun) for (size_t i = 0; i < L.size(); i++) {
                float ly = y + 26 * sc + i * lh;
                dl->AddText(fs, fsz, ImVec2(x + pad, ly), IM_COL32(150, 146, 170, 255), L[i].first.c_str());
                ImVec2 ts = fs->CalcTextSizeA(fsz, FLT_MAX, 0, L[i].second.c_str());
                dl->AddText(fs, fsz, ImVec2(x + wPx - pad - ts.x, ly), IM_COL32(235, 235, 245, 255), L[i].second.c_str());
            }
            y += bh + gap;
        }
    }
    return y - origin.y;
}
void DockOverlay(const Snapshot& s, float W, float H, bool menuOpen) {
    float wPx = gx::Val("wwidth") * S, m = 22 * S;
    float h = DrawDock(nullptr, ImVec2(0, 0), wPx, s, false, S, true);
    if (h <= 0) return;
    int pos = (int)gx::Val("wdock");
    ImVec2 o((pos == 1 || pos == 3) ? W - m - wPx : m, pos >= 2 ? H - m - h : m);
    PF(fSmall);
    DrawDock(ImGui::GetBackgroundDrawList(), o, wPx, s, menuOpen, S);
    ImGui::PopFont();
}
void DrawCompass(ImDrawList* dl, ImVec2 c, float wPx, float fx, float fz, const std::vector<RadarDot>* dots, float sc, float alpha) {
    float range = std::max(30.f, gx::Val("comprange")), hh = 40 * sc;
    float head = std::atan2(fx, fz) * 57.29578f; if (head < 0) head += 360.f;
    ImVec2 a(c.x - wPx * 0.5f, c.y), b(c.x + wPx * 0.5f, c.y + hh);
    dl->AddRectFilled(a, b, IM_COL32(20, 18, 28, (int)(alpha * 200)), 8 * sc);
    dl->AddRect(a, b, IM_COL32(255, 255, 255, (int)(alpha * 30)), 8 * sc, 1.f);
    static const char* card[8] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
    float pxPerDeg = (wPx - 24 * sc) / range;
    dl->PushClipRect(ImVec2(a.x + 6 * sc, a.y), ImVec2(b.x - 6 * sc, b.y), true);
    for (int d = 0; d < 360; d += 15) {
        float diff = std::fmod(d - head + 540.f, 360.f) - 180.f;
        if (std::fabs(diff) > range * 0.5f + 10) continue;
        float x = c.x + diff * pxPerDeg;
        bool major = d % 45 == 0;
        dl->AddLine(ImVec2(x, b.y - (major ? 14 : 8) * sc), ImVec2(x, b.y - 4 * sc), IM_COL32(220, 220, 235, major ? 200 : 110), 1.2f * sc);
        if (major) { const char* t = card[d / 45]; ImVec2 ts = fSmall->CalcTextSizeA(fSmall->LegacySize * sc / S * 0.85f, FLT_MAX, 0, t);
            dl->AddText(fSmall, fSmall->LegacySize * sc / S * 0.85f, ImVec2(x - ts.x * 0.5f, a.y + 5 * sc), IM_COL32(170, 166, 190, 220), t); }
    }
    if (dots) for (auto& d : *dots) {
        if (d.cat >= 0 && !gx::On(RadarCatId(d.cat, true))) continue;   // build 17: compass's own categories
        float bear = std::atan2(d.x, d.z) * 57.29578f;
        float diff = std::fmod(bear - head + 540.f, 360.f) - 180.f;
        if (std::fabs(diff) > range * 0.5f) continue;
        dl->AddCircleFilled(ImVec2(c.x + diff * pxPerDeg, b.y - 3 * sc), 2.6f * sc, (d.color & 0x00FFFFFF) | 0xFF000000, 8);
    }
    dl->PopClipRect();
    dl->AddRectFilled(ImVec2(c.x - 2 * sc, b.y - 16 * sc), ImVec2(c.x + 2 * sc, b.y - 3 * sc), IM_COL32(230, 60, 70, 255), 1.f);
    char t[24]; snprintf(t, sizeof t, "%s %.0f°", card[((int)((head + 22.5f) / 45.f)) % 8], head);
    ImVec2 ts = fBold->CalcTextSizeA(fBold->LegacySize * sc / S * 0.9f, FLT_MAX, 0, t);
    dl->AddText(fBold, fBold->LegacySize * sc / S * 0.9f, ImVec2(c.x - ts.x * 0.5f, a.y + 4 * sc), IM_COL32(240, 240, 250, 255), t);
}
void DrawArrows(ImDrawList* dl, float W, float H, const std::vector<RadarDot>& ind, float fx, float fz, float rx, float rz, float sc, bool arrows, bool edges) {
    ImVec2 c(W * 0.5f, H * 0.5f); float R = gx::Val("esparrowr") * sc;
    if (arrows && !ind.empty()) dl->AddCircle(c, R, IM_COL32(230, 60, 70, 70), 64, 1.2f * sc);
    for (auto& d : ind) {
        float ang = std::atan2(d.x * rx + d.z * rz, d.x * fx + d.z * fz);   // 0 = ahead, + = right
        float sx = std::sin(ang), sy = -std::cos(ang);
        ImU32 col = (d.color & 0x00FFFFFF) | 0xE6000000;
        if (arrows) {
            ImVec2 p(c.x + sx * R, c.y + sy * R), px(-sy, sx);
            dl->AddTriangleFilled(ImVec2(p.x + sx * 12 * sc, p.y + sy * 12 * sc), ImVec2(p.x + px.x * 7 * sc, p.y + px.y * 7 * sc), ImVec2(p.x - px.x * 7 * sc, p.y - px.y * 7 * sc), col);
        }
        if (edges && std::fabs(ang) > 0.75f) {   // outside the view: marker on the screen edge in that direction
            float m = 34 * sc, tx = std::fabs(sx) > 1e-3f ? (W * 0.5f - m) / std::fabs(sx) : 1e9f, ty = std::fabs(sy) > 1e-3f ? (H * 0.5f - m) / std::fabs(sy) : 1e9f;
            float t = std::min(tx, ty);
            ImVec2 p(c.x + sx * t, c.y + sy * t);
            dl->AddRectFilled(ImVec2(p.x - 9 * sc, p.y - 9 * sc), ImVec2(p.x + 9 * sc, p.y + 9 * sc), IM_COL32(20, 18, 28, 200), 4 * sc);
            dl->AddRect(ImVec2(p.x - 9 * sc, p.y - 9 * sc), ImVec2(p.x + 9 * sc, p.y + 9 * sc), col, 4 * sc, 1.6f * sc);
            dl->AddCircleFilled(p, 3 * sc, col, 10);
        }
    }
}
// build 17: the preview is the whole screen in miniature - same corner, same proportions, nothing cut off
float WidgetsPreview(float x, float y, float cw, bool draw) {
    ImVec2 ds = ImGui::GetIO().DisplaySize;
    float pw = cw - 36 * S, k = pw / std::max(1.f, ds.x), ph = ds.y * k;
    float h = ph + 16 * S;
    if (!draw) return h;
    float px = x + 18 * S, py = y + 8 * S;
    PreviewScene(px, py, pw, ph);
    static Snapshot fake; static bool init = false;
    if (!init) { init = true; fake.hp = 68; fake.hpmax = 100; fake.mana = 30; fake.manamax = 50; fake.thrCount = 2; fake.thrName = "Wilk"; fake.thrDist = 14; fake.weapon = "Miecz";
        fake.px = 1200; fake.py = -450; fake.pz = 8800; fake.day = 2; fake.hour = 14; fake.minute = 5; fake.world = "WORLD.ZEN"; fake.arrows = 42; fake.bolts = 0;
        fake.level = 5; fake.exp = 1300; fake.expNext = 2000; fake.tickMs = 0.4f; fake.memMB = 900; fake.npcCount = 210; fake.itemCount = 1800; fake.tValid = true; fake.tName = "Ścierwojad"; fake.tHp = 40; fake.tHpMax = 80; }
    float yaw = (float)std::fmod(NowSec() * 0.4, 6.2831853);
    fake.rfx = std::sin(yaw); fake.rfz = std::cos(yaw); fake.rrx = fake.rfz; fake.rrz = -fake.rfx;
    float rg = gx::Val("radarrange") * 100.f;
    fake.radar.clear();
    auto add = [&](float a, float b, int col, int type, int cat, const char* n) { RadarDot d{ a * rg, b * rg, g_espColors[col], type, n, "" }; d.cat = cat; fake.radar.push_back(d); };
    add(0.3f, 0.5f, 0, 0, 0, "Wilk"); add(-0.4f, 0.2f, 2, 0, 1, "Kopacz"); add(0.5f, -0.3f, 3, 0, 2, "Kret"); add(-0.1f, -0.3f, 1, 3, 3, "Diego");
    add(0.2f, -0.6f, 4, 0, 4, "Martwy"); add(-0.6f, -0.5f, 5, 1, 5, "Miecz"); add(-0.7f, 0.1f, 6, 2, 6, "Skrzynia");
    float sc = S * k;
    DL()->PushClipRect(ImVec2(px, py), ImVec2(px + pw, py + ph), true);
    PF(fSmall);
    float wPx = gx::Val("wwidth") * sc, m = 22 * sc;
    float dh = DrawDock(nullptr, ImVec2(0, 0), wPx, fake, false, sc, true);
    int pos = (int)gx::Val("wdock");
    ImVec2 o((pos == 1 || pos == 3) ? px + pw - m - wPx : px + m, pos >= 2 ? py + ph - m - dh : py + m);
    DrawDock(DL(), o, wPx, fake, false, sc);
    if (gx::On("compass")) DrawCompass(DL(), ImVec2(px + pw * 0.5f, py + 14 * sc), gx::Val("compw") * sc, fake.rfx, fake.rfz, gx::On("compmark") ? &fake.radar : nullptr, sc, 1.f);
    if (gx::On("wtarget")) {
        float bw = 360 * sc, bx = px + pw * 0.5f - bw * 0.5f, by = py + (gx::On("compass") ? 64 : 20) * sc;
        DL()->AddRectFilled(ImVec2(bx, by), ImVec2(bx + bw, by + 46 * sc), IM_COL32(20, 18, 28, 210), 8 * sc);
        DL()->AddRectFilled(ImVec2(bx + 12 * sc, by + 32 * sc), ImVec2(bx + 12 * sc + (bw - 24 * sc) * 0.5f, by + 38 * sc), IM_COL32(220, 50, 60, 255), 3 * sc);
    }
    ImGui::PopFont();
    DL()->PopClipRect();
    Text(ImVec2(px + 6 * S, py + ph + 0 * S), fSmall, C_FAINT, "");
    return h;
}
void RadarOverlay(const Snapshot& s, float W, float H, bool menuOpen) {
    float R = std::max(60.f, gx::Val("radarsize") * 0.5f) * S;
    int pos = (int)gx::Val("radarpos");
    float m = 24 * S;
    ImVec2 c((pos == 1 || pos == 3) ? m + R : W - m - R, (pos >= 2) ? H - m - R : m + R + (pos == 0 ? 0 : 0));
    PF(fSmall);
    DrawRadar(ImGui::GetBackgroundDrawList(), c, R, s.radar, s.rfx, s.rfz, s.rrx, s.rrz, menuOpen);
    ImGui::PopFont();
}
// ---- build 14: previews in every Wizualia tab (fake characters drawn with the current settings)
void PreviewScene(float x, float y, float w, float h) {
    DL()->AddRectFilledMultiColor(ImVec2(x, y), ImVec2(x + w, y + h), IM_COL32(40, 46, 62, 255), IM_COL32(40, 46, 62, 255), IM_COL32(26, 30, 26, 255), IM_COL32(26, 30, 26, 255));
    DL()->AddRectFilled(ImVec2(x, y + h * 0.78f), ImVec2(x + w, y + h), IM_COL32(34, 40, 30, 255));
}
Label FakeNpc(float cx, float footY, float hgt, float aspect, unsigned col, const char* name, const char* guild, const char* kind, float dist, int hp, int hpmax, bool eq) {
    Label L{}; L.type = 0; L.color = col; L.hp = -1;
    float hw = hgt * 0.22f / aspect, top = footY - hgt;
    L.x = cx; L.y = top - hgt * 0.04f;
    bool skel = gx::On("espskel"), box = gx::On("espbox"); int mode = (int)gx::Val("espboxmode");
    // stick figure (normalized to the preview rect)
    float hx = cx, hy = top + hgt * 0.08f, nk = top + hgt * 0.17f, hip = top + hgt * 0.52f;
    float pts[][4] = { { hx, hy, hx, nk }, { hx, nk, hx, hip }, { hx, nk, hx - hw * 0.8f, top + hgt * 0.36f }, { hx - hw * 0.8f, top + hgt * 0.36f, hx - hw * 0.9f, top + hgt * 0.52f },
                       { hx, nk, hx + hw * 0.8f, top + hgt * 0.36f }, { hx + hw * 0.8f, top + hgt * 0.36f, hx + hw * 0.95f, top + hgt * 0.5f },
                       { hx, hip, hx - hw * 0.45f, top + hgt * 0.76f }, { hx - hw * 0.45f, top + hgt * 0.76f, hx - hw * 0.5f, footY },
                       { hx, hip, hx + hw * 0.45f, top + hgt * 0.76f }, { hx + hw * 0.45f, top + hgt * 0.76f, hx + hw * 0.55f, footY } };
    if (skel) for (auto& b : pts) for (float v : b) L.bones.push_back(v);
    L.anchor = true; L.ax = cx; L.ay = footY;
    if (box) {
        if (mode == 2 || mode == 4) {
            float d = hw * 0.55f, dy = hgt * 0.05f;
            float x0 = cx - hw, x1 = cx + hw, y0 = top - hgt * 0.03f, y1 = footY + hgt * 0.01f;
            for (int i = 0; i < 8; i++) {
                float px = (i & 1) ? x1 : x0, py = (i & 2) ? y0 : y1;
                if (i & 4) { px += d; py -= dy; }
                L.corners[i * 2] = px; L.corners[i * 2 + 1] = py;
            }
            L.box = 2; L.bx0 = x0; L.by0 = y0 - dy; L.bx1 = x1 + d; L.by1 = y1;
        } else {
            float pad = mode == 1 ? 0.f : hw * 0.15f;
            L.box = 1; L.bx0 = cx - hw - pad; L.bx1 = cx + hw + pad; L.by0 = top - hgt * 0.03f; L.by1 = footY + hgt * 0.01f;
        }
    }
    if (gx::On("espname")) L.text = name;
    std::string sub; auto add = [&](const std::string& t) { if (!sub.empty()) sub += "  ·  "; sub += t; };
    if (gx::On("espguild")) add(guild);
    if (gx::On("esptype") && kind && *kind) add(std::string("[") + kind + "]");
    if (gx::On("espdist")) { char b[16]; snprintf(b, sizeof b, "%.0fm", dist); add(b); }
    if (gx::On("esphpnum")) { char b[32]; snprintf(b, sizeof b, "%d/%d HP", hp, hpmax); add(b); }
    L.sub = sub;
    if (gx::On("esphp")) L.hp = (float)hp / hpmax;
    if (eq && gx::On("espeq")) L.lines = { "[założone] Ciężka zbroja strażnika", "Ciężki miecz", "Mikstura leczenia x3" };
    return L;
}
float EspNpcPreview(float x, float y, float cw, bool draw) {
    float h = 250 * S;
    if (!draw) return h;
    float px = x + 18 * S, pw = cw - 36 * S, py = y + 8 * S, ph = h - 16 * S;
    PreviewScene(px, py, pw, ph);
    DL()->PushClipRect(ImVec2(px, py), ImVec2(px + pw, py + ph), true);
    if (gx::On("espnpc")) {
        float aspect = pw / ph;
        PF(fSmall);
        Label a = FakeNpc(0.3f, 0.9f, 0.5f, aspect, g_espColors[0], "Strażnik", "Strażnicy", "", 12, 140, 200, true);
        Label b = FakeNpc(0.74f, 0.86f, 0.36f, aspect, g_espColors[1], "Wilk", "Wilki", "Drużyna", 6, 60, 60, false);
        DrawLabel(DL(), a, gx::On("espline"), px, py, pw, ph);
        DrawLabel(DL(), b, gx::On("espline"), px, py, pw, ph);
        ImGui::PopFont();
    } else Text(ImVec2(px + 14 * S, py + 12 * S), fSmall, C_DIM, "ESP postaci jest wyłączone");
    DL()->PopClipRect();
    return h;
}
float EspItemPreview(float x, float y, float cw, bool draw) {
    float h = 210 * S;
    if (!draw) return h;
    float px = x + 18 * S, pw = cw - 36 * S, py = y + 8 * S, ph = h - 16 * S;
    PreviewScene(px, py, pw, ph);
    DL()->PushClipRect(ImVec2(px, py), ImVec2(px + pw, py + ph), true);
    if (gx::On("espitm")) {
        PF(fSmall);
        if (gx::On("espitems")) {
            Label L{}; L.type = 1; L.hp = -1; L.x = 0.3f; L.y = 0.85f; L.color = g_espColors[5]; L.text = "Miecz bojowy"; L.sub = "60 rudy  ·  7m";
            DL()->AddRectFilled(ImVec2(px + pw * 0.3f - 10 * S, py + ph * 0.85f), ImVec2(px + pw * 0.3f + 10 * S, py + ph * 0.85f + 3 * S), IM_COL32(150, 150, 160, 255));
            DrawLabel(DL(), L, false, px, py, pw, ph);
        }
        if (gx::On("espchest")) {
            float cxp = px + pw * 0.72f, cyp = py + ph * 0.86f;
            DL()->AddRectFilled(ImVec2(cxp - 22 * S, cyp - 22 * S), ImVec2(cxp + 22 * S, cyp + 4 * S), IM_COL32(110, 78, 46, 255), 3 * S);
            Label L{}; L.type = 2; L.hp = -1; L.x = 0.72f; L.y = 0.86f - 30 * S / ph; L.color = g_espColors[6]; L.text = "Skrzynia"; L.sub = "9m";
            if (gx::On("espchestinv")) L.lines = { "Bryłka rudy x25", "Pochodnia x2", "Mikstura many" };
            DrawLabel(DL(), L, false, px, py, pw, ph);
        }
        if (!gx::On("espitems") && !gx::On("espchest")) Text(ImVec2(px + 14 * S, py + 12 * S), fSmall, C_DIM, "Włącz przedmioty albo skrzynie");
        ImGui::PopFont();
    } else Text(ImVec2(px + 14 * S, py + 12 * S), fSmall, C_DIM, "ESP przedmiotów jest wyłączone");
    DL()->PopClipRect();
    return h;
}
float ColorsPreview(float x, float y, float cw, bool draw) {
    float h = 120 * S;
    if (!draw) return h;
    static const char* names[7] = { "Wróg", "Drużyna", "Neutralny", "Potwór", "Martwy", "Przedmiot", "Skrzynia" };
    float px = x + 18 * S, pw = cw - 36 * S, py = y + 8 * S, ph = h - 16 * S;
    PreviewScene(px, py, pw, ph);
    PF(fSmall);
    for (int i = 0; i < 7; i++) {
        Label L{}; L.type = 1; L.hp = -1; L.color = g_espColors[i]; L.text = names[i];
        L.x = (i + 0.5f) / 7.f; L.y = 0.62f;
        DrawLabel(DL(), L, false, px, py, pw, ph);
        DL()->AddCircleFilled(ImVec2(px + L.x * pw, py + ph * 0.75f), 4 * S, g_espColors[i]);
    }
    ImGui::PopFont();
    return h;
}
float RadarPreview(float x, float y, float cw, bool draw) {
    float R = std::min(120.f * S, (cw - 36 * S) * 0.5f);
    float h = R * 2 + 24 * S;
    if (!draw) return h;
    static std::vector<RadarDot> fake;
    float rg = gx::Val("radarrange") * 100.f;
    fake.clear();
    auto add = [&](float fx2, float fz2, int col, int type, const char* n, const char* inf) { fake.push_back({ fx2 * rg, fz2 * rg, g_espColors[col], type, n, inf }); };
    if (gx::On("raden")) add(0.2f, 0.5f, 0, 0, "Wilk", "wróg");
    if (gx::On("radne")) add(-0.4f, 0.3f, 2, 0, "Kopacz", "neutralny");
    if (gx::On("radmo")) add(0.6f, -0.2f, 3, 0, "Ścierwojad", "potwór");
    if (gx::On("radpa")) add(-0.1f, -0.15f, 1, 3, "Diego", "drużyna");
    if (gx::On("radardead")) add(0.45f, 0.62f, 4, 0, "Martwy kret", "");
    if (gx::On("radaritems")) { add(-0.55f, -0.5f, 5, 1, "Miecz", ""); add(0.1f, 0.8f, 5, 1, "Mikstura", ""); }
    if (gx::On("radarchests")) add(-0.7f, 0.1f, 6, 2, "Skrzynia", "");
    PF(fSmall);
    // build 15: the preview hero turns slowly - with "rotate" the dots turn around a fixed arrow, without it the arrow turns
    float yaw = (float)std::fmod(NowSec() * 0.5, 6.2831853);
    float fx = std::sin(yaw), fz = std::cos(yaw);
    DrawRadar(DL(), ImVec2(x + cw * 0.5f, y + 12 * S + R), R, fake, fx, fz, fz, -fx, true);
    ImGui::PopFont();
    return h;
}
// build 12: on-screen info window - coordinates, FPS, speed... each line switchable in Narzędzia › Nakładka
void HudWindow(const Snapshot& s, bool menuOpen, float dispW) {
    static double lastT = 0; static float lastX = 0, lastZ = 0, lastY = 0, spd = 0;
    double now = NowSec();
    if (lastT > 0 && now - lastT > 0.05) {   // horizontal speed, smoothed
        float dx = s.px - lastX, dz = s.pz - lastZ;
        float v = std::sqrt(dx * dx + dz * dz) / 100.f / (float)(now - lastT);
        if (v < 200.f) spd += (v - spd) * 0.35f;
        lastT = now; lastX = s.px; lastZ = s.pz; lastY = s.py;
    } else if (lastT == 0) { lastT = now; lastX = s.px; lastZ = s.pz; }
    ImGui::SetNextWindowPos(ImVec2(dispW - 300 * S, 120 * S), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(std::max(0.f, std::min(1.f, gx::Val("hudalpha") / 100.f)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12 * S, 10 * S));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, R_CARD());
    ImGuiWindowFlags fl = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing |
                          ImGuiWindowFlags_NoSavedSettings | (menuOpen ? 0 : ImGuiWindowFlags_NoInputs);
    ImGui::Begin("##hud", nullptr, fl);
    float hs = std::max(0.7f, std::min(2.f, gx::Val("hudsize") / 100.f));
    ImGui::PushFont(fSmall, fSmall->LegacySize * hs);
    auto kv = [hs](const char* k, const char* v, const ImVec4& c) { ImGui::TextColored(C_DIM, "%s", k); ImGui::SameLine(84 * S * hs); ImGui::TextColored(c, "%s", v); };
    char b[160];
    if (gx::On("hudfps")) { float f = ImGui::GetIO().Framerate; snprintf(b, sizeof b, "%.0f  (%.1f ms)", f, f > 0 ? 1000.f / f : 0.f); kv("FPS", b, f >= 50 ? C_ST_OK : f >= 30 ? C_ST_BUG : C_ST_CRASH); }
    if (gx::On("hudpos")) { snprintf(b, sizeof b, "%.0f  %.0f  %.0f", s.px, s.py, s.pz); kv("Pozycja", b, C_TEXT); }
    if (gx::On("hudspeed")) { snprintf(b, sizeof b, "%.1f m/s", spd); kv("Prędkość", b, C_TEXT); }
    if (gx::On("hudtime")) { snprintf(b, sizeof b, "dzień %d, %02d:%02d", s.day, s.hour, s.minute); kv("Czas", b, C_TEXT); }
    if (gx::On("hudworld")) { kv("Świat", s.world.empty() ? "-" : s.world.c_str(), C_TEXT); }
    if (gx::On("hudstats")) { snprintf(b, sizeof b, "%d / %d", s.hp, s.hpmax); kv("HP", b, C_HP); snprintf(b, sizeof b, "%d / %d", s.mana, s.manamax); kv("Mana", b, C_MANA); }
    if (gx::On("hudtarget")) { if (s.tValid) { snprintf(b, sizeof b, "%s  %d/%d", s.tName.c_str(), s.tHp, s.tHpMax); kv("Cel", b, C_ACC); } else kv("Cel", "-", C_DIM); }
    if (gx::On("hudmods")) { int n = 0; for (auto& r : T.rows) if (r.on && (r.kind == K_TOGGLE || r.kind == K_SLIDER) && r.cat != "Konfiguracja") n++; snprintf(b, sizeof b, "%d", n); kv("Włączone", b, C_TEXT); }
    ImGui::PopFont();
    ImGui::End();
    ImGui::PopStyleVar(2);
}
// window with everything that is switched on (and its settings)
void StatusWindow(bool menuOpen, float dispW) {
    std::vector<std::pair<Row*, std::vector<Row*>>> act;
    for (auto& r : T.rows) {
        if (!r.on || r.cat == "Konfiguracja" || r.id == "nccam" || r.id == "allynotalk" || r.id == "allykeep" || r.id == "qsallies") continue;
        if (r.kind != K_TOGGLE && r.kind != K_SLIDER && r.kind != K_SET) continue;
        if (r.cat == "Wizualia" && r.id != "espnpc" && r.id != "espitm") continue;
        std::vector<Row*> set;
        for (auto& o : T.rows) if (&o != &r && o.kind == K_VALUE && !o.hidden && o.cat == r.cat && o.sub == r.sub && o.group == r.group) set.push_back(&o);
        act.push_back({ &r, set });
    }
    ImGui::SetNextWindowPos(ImVec2(24 * S, 120 * S), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(std::max(0.f, std::min(1.f, gx::Val("statusalpha") / 100.f)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14 * S, 12 * S));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, R_CARD());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(C_ACC.x, C_ACC.y, C_ACC.z, 0.15f));
    ImGuiWindowFlags fl = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing |
                          ImGuiWindowFlags_NoNav;
    if (!menuOpen) fl |= ImGuiWindowFlags_NoInputs;
    ImGui::Begin("##status", nullptr, fl);
    PF(fBold); ImGui::TextColored(C_ACC, "Aktywne mody  (%d)", (int)act.size()); ImGui::PopFont();
    if (act.empty()) ImGui::TextColored(C_DIM, "nic nie jest włączone");
    for (auto& a : act) {
        Row& r = *a.first;
        std::string line = r.name;
        if (r.kind == K_SLIDER || r.kind == K_SET) line += "  " + ValStr(r);
        ImGui::TextColored(C_TEXT, "%s", line.c_str());
        if (!r.hotkey && a.second.empty()) continue;
        std::string sub;
        if (r.hotkey) sub = "[" + KeyName(r.hotkey) + "]";
        for (auto* o : a.second) {
            std::string v = ValStr(*o);
            for (auto& op : o->options) if (op.first == (int)o->val) v = op.second;
            if (!sub.empty()) sub += "  ·  ";
            sub += o->name + ": " + v;
        }
        PF(fSmall); ImGui::TextColored(C_DIM, "   %s", sub.c_str()); ImGui::PopFont();
    }
    ImGui::End();
    ImGui::PopStyleColor(); ImGui::PopStyleVar(3);
    (void)dispW;
}

// ------------------------------------------------------------------ sidebar
void Sidebar(const Snapshot& s, ImVec2 p, float w, float h) {
    DL()->AddRectFilled(p, ImVec2(p.x + w, p.y + h), U32(C_SIDE), R_WIN(), ImDrawFlags_RoundTopLeft | ImDrawFlags_RoundBottomLeft);
    Text(ImVec2(p.x + 24 * S, p.y + 20 * S), fTitle, C_TEXT, "GOTHIC");
    char b[64]; snprintf(b, sizeof b, "Crazy Trainer  ·  build %d", TRAINER_BUILD);
    Text(ImVec2(p.x + 24 * S, p.y + 52 * S), fSmall, C_ACC, b);
    float y = p.y + 88 * S;
    float itemH = 38 * S, subH = 30 * S, ix0 = p.x + 12 * S, ix1 = p.x + w - 12 * S;
    for (int i = 0; i < (int)kSecs.size(); i++) {
        const Sec& sec = kSecs[i];
        bool act = !cfgPage && !testsPage && curSec == i;
        bool single = sec.subs.size() == 1;
        ImGui::PushID(i + 100);
        ImGui::SetCursorScreenPos(ImVec2(ix0, y));
        if (ImGui::InvisibleButton("##sec", ImVec2(ix1 - ix0, itemH))) {
            if (cfgPage || testsPage || curSec != i) { SwapSearch(i, 0); curSec = i; curSub = 0; }
            cfgPage = false; testsPage = false; gsearch[0] = 0;
        }
        bool hov = ImGui::IsItemHovered();
        if (act || hov) DL()->AddRectFilled(ImVec2(ix0, y), ImVec2(ix1, y + itemH), U32(act ? C_ACCD : C_HOVER, act ? 0.6f : 0.6f), R_ITEM());
        IconAt(DL(), ImVec2(ix0 + 20 * S, y + itemH * 0.5f), sec.icon, U32(act ? C_ACC : C_DIM), 14 * S);
        TextV(ix0 + 40 * S, y + itemH * 0.5f, act ? fBold : fReg, act || hov ? C_TEXT : C_DIM, sec.name);
        if (!single) IconAt(DL(), ImVec2(ix1 - 16 * S, y + itemH * 0.5f), act ? ICO_CHEVD : ICO_CHEVR, U32(C_FAINT), 10 * S);
        ImGui::PopID();
        y += itemH + 2 * S;
        if (act && !single) {
            for (int j = 0; j < (int)sec.subs.size(); j++) {
                bool sa = curSub == j && !gsearch[0];
                ImGui::PushID(i * 100 + j + 1000);
                ImGui::SetCursorScreenPos(ImVec2(ix0, y));
                if (ImGui::InvisibleButton("##sub", ImVec2(ix1 - ix0, subH))) { if (curSub != j) SwapSearch(curSec, j); curSub = j; gsearch[0] = 0; }
                bool sh = ImGui::IsItemHovered();
                if (sh && !sa) DL()->AddRectFilled(ImVec2(ix0 + 32 * S, y), ImVec2(ix1, y + subH), U32(C_HOVER, 0.45f), R_BTN());
                if (sa) DL()->AddRectFilled(ImVec2(ix0 + 38 * S, y + 8 * S), ImVec2(ix0 + 40.5f * S, y + subH - 8 * S), U32(C_ACC), 1.5f * S);
                TextV(ix0 + 50 * S, y + subH * 0.5f, sa ? fBold : fReg, sa ? C_ACC : (sh ? C_TEXT : C_DIM), sec.subs[j]);
                ImGui::PopID();
                y += subH;
            }
            y += 6 * S;
        }
    }
    float by = p.y + h - 14 * S - itemH;
    if (T.testerMode) {
        float ty = by - itemH - 4 * S;
        ImGui::SetCursorScreenPos(ImVec2(ix0, ty));
        if (ImGui::InvisibleButton("##tests", ImVec2(ix1 - ix0, itemH))) testsWin = !testsWin;   // separate window, no tab switching
        bool th = ImGui::IsItemHovered(); bool testsPage = testsWin;
        if (testsPage || th) DL()->AddRectFilled(ImVec2(ix0, ty), ImVec2(ix1, ty + itemH), U32(testsPage ? C_ACCD : C_HOVER, 0.6f), R_ITEM());
        IconAt(DL(), ImVec2(ix0 + 20 * S, ty + itemH * 0.5f), ICN_CHECK, U32(testsPage ? C_ACC : C_DIM), 14 * S);
        TextV(ix0 + 40 * S, ty + itemH * 0.5f, testsPage ? fBold : fReg, testsPage || th ? C_TEXT : C_DIM, "Testy");
        by = ty + itemH + 4 * S;
        by = p.y + h - 14 * S - itemH;
        y = std::max(y, 0.f);
    }
    ImGui::SetCursorScreenPos(ImVec2(ix0, by));
    if (ImGui::InvisibleButton("##cfg", ImVec2(ix1 - ix0, itemH))) { cfgPage = true; testsPage = false; gsearch[0] = 0; }
    bool hov = ImGui::IsItemHovered();
    if (cfgPage || hov) DL()->AddRectFilled(ImVec2(ix0, by), ImVec2(ix1, by + itemH), U32(cfgPage ? C_ACCD : C_HOVER, 0.6f), R_ITEM());
    IconAt(DL(), ImVec2(ix0 + 20 * S, by + itemH * 0.5f), ICN_COG, U32(cfgPage ? C_ACC : C_DIM), 14 * S);
    TextV(ix0 + 40 * S, by + itemH * 0.5f, cfgPage ? fBold : fReg, cfgPage || hov ? C_TEXT : C_DIM, "Konfiguracja");
    float pcH = 84 * S, py = by - pcH - 12 * S - (T.testerMode ? itemH + 4 * S : 0);
    if (py < y + 10 * S) return;
    DL()->AddRectFilled(ImVec2(ix0, py), ImVec2(ix1, py + pcH), U32(C_CARD), R_ITEM());
    if (!s.inGame) {
        Text(ImVec2(ix0 + 12 * S, py + 14 * S), fBold, C_ACC, "Gothic połączony");
        Text(ImVec2(ix0 + 12 * S, py + 36 * S), fSmall, C_DIM, "wczytaj albo zacznij grę");
        return;
    }
    std::string nm = s.bodyName.empty() ? "Bezimienny" : s.bodyName;
    TextClip(ImVec2(ix0 + 12 * S, py + 10 * S), fBold, s.swapped ? C_ST_BUG : C_TEXT, nm, ix1 - 8 * S);
    char inf[96]; snprintf(inf, sizeof inf, "poz. %d  ·  %d rudy  ·  %02d:%02d", s.level, s.ore, s.hour, s.minute);
    Text(ImVec2(ix0 + 12 * S, py + 30 * S), fSmall, C_DIM, inf);
    HpBar(ix0 + 12 * S, ix1 - 12 * S, py + 54 * S, s.hp, s.hpmax, C_HP, 5);
    HpBar(ix0 + 12 * S, ix1 - 12 * S, py + 66 * S, s.mana, s.manamax, C_MANA, 5);
}
} // namespace

// ------------------------------------------------------------------------------
void UiInitFonts(float screenH) {
    S = std::max(0.75f, screenH / 1080.f);
    g_baseS = S;
    ImGuiIO& io = ImGui::GetIO();
    static const ImWchar ranges[] = { 0x0020, 0x00FF, 0x0100, 0x017F, 0x2010, 0x203A, 0 };
    static const ImWchar iconRanges[] = { 0xF000, 0xF2E0, 0 };
    char win[MAX_PATH]; GetWindowsDirectoryA(win, MAX_PATH);
    std::string reg = std::string(win) + "\\Fonts\\segoeui.ttf", bold = std::string(win) + "\\Fonts\\segoeuib.ttf";
    std::string semi = std::string(win) + "\\Fonts\\seguisb.ttf";
    if (GetFileAttributesA(semi.c_str()) == INVALID_FILE_ATTRIBUTES) semi = bold;
    std::string dir = IniPath(); dir = dir.substr(0, dir.find_last_of("\\/"));
    std::string icons = dir + "\\..\\..\\launcher\\Fonts\\forkawesome-webfont.ttf";
    if (GetFileAttributesA(icons.c_str()) == INVALID_FILE_ATTRIBUTES) icons = dir + "\\forkawesome-webfont.ttf";
    iconsOk = GetFileAttributesA(icons.c_str()) != INVALID_FILE_ATTRIBUTES;
    auto add = [&](const std::string& f, float px) -> ImFont* {
        if (GetFileAttributesA(f.c_str()) == INVALID_FILE_ATTRIBUTES) return io.Fonts->AddFontDefault();
        ImFont* font = io.Fonts->AddFontFromFileTTF(f.c_str(), px * S, nullptr, ranges);
        if (iconsOk) { ImFontConfig cfg; cfg.MergeMode = true; cfg.GlyphOffset.y = 1 * S; io.Fonts->AddFontFromFileTTF(icons.c_str(), px * S * 0.92f, &cfg, iconRanges); }
        return font;
    };
    fReg = add(reg, 15); fBold = add(semi, 15); fBig = add(bold, 19); fTitle = add(bold, 23); fSmall = add(reg, 12.5f);
    fIcon = iconsOk ? io.Fonts->AddFontFromFileTTF(icons.c_str(), 15 * S, nullptr, iconRanges) : fReg;
    io.FontDefault = fReg;
    for (int i = 0; i < 7; i++) {
        char k[16]; snprintf(k, sizeof k, "c%d", i);
        unsigned v = (unsigned)GetPrivateProfileIntA("espcolors", k, 0, IniPath().c_str());
        if (v) g_espColors[i] = v;
    }
    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowRounding = R_WIN(); st.ChildRounding = R_CARD(); st.FrameRounding = R_BTN(); st.PopupRounding = R_ITEM();
    st.ScrollbarSize = 6 * S; st.ScrollbarRounding = 3 * S; st.GrabRounding = R_BTN();
    st.WindowBorderSize = 1.f; st.FrameBorderSize = 0; st.PopupBorderSize = 1.f; st.ChildBorderSize = 0; st.WindowPadding = ImVec2(0, 0);
    st.FramePadding = ImVec2(8 * S, 5 * S); st.ItemSpacing = ImVec2(6 * S, 6 * S);
    ImVec4* c = st.Colors;
    c[ImGuiCol_WindowBg] = C_WIN; c[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0); c[ImGuiCol_Text] = C_TEXT;
    c[ImGuiCol_TextDisabled] = C_DIM; c[ImGuiCol_FrameBg] = C_CARD2; c[ImGuiCol_FrameBgHovered] = C_HOVER;
    c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_PopupBg] = ImVec4(0.09f, 0.09f, 0.12f, 0.98f);
    c[ImGuiCol_ResizeGrip] = ImVec4(0, 0, 0, 0); c[ImGuiCol_ResizeGripHovered] = ImVec4(0, 0, 0, 0); c[ImGuiCol_ResizeGripActive] = ImVec4(0, 0, 0, 0);
}

bool UiCapturingKey() { return bindRow != nullptr; }
bool UiTyping() { return editRow != nullptr || ImGui::GetIO().WantTextInput; }
void UiBindKey(int) {}   // shortcuts are recorded by polling (PollBinding)

void UiDraw(bool menuOpen, float dispW, float dispH) {
    {   // build 16: menu scale, transparency, rounding, game dimming
        float us = std::max(0.6f, std::min(1.6f, gx::Val("uiscale") / 100.f));
        S = g_baseS * us; ImGui::GetStyle().FontScaleMain = us;
        g_round = std::max(0.f, std::min(2.f, gx::Val("uiround") / 100.f));
        ImGui::GetStyle().Alpha = menuOpen ? std::max(0.5f, std::min(1.f, gx::Val("uialpha") / 100.f)) : 1.f;
        if (menuOpen && gx::Val("uidim") > 0) ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0, 0), ImVec2(dispW, dispH), IM_COL32(0, 0, 0, (int)(gx::Val("uidim") * 2.55f)));
    }
    C_ACC = ImVec4(T.accent[0], T.accent[1], T.accent[2], 1);
    C_ACCD = Mix(ImVec4(0.10f, 0.10f, 0.14f, 1), C_ACC, 0.30f);
    ImVec4* sc = ImGui::GetStyle().Colors;
    sc[ImGuiCol_FrameBgActive] = C_ACCD; sc[ImGuiCol_ScrollbarGrab] = Mix(C_TRACK, C_ACC, 0.5f); sc[ImGuiCol_ScrollbarGrabHovered] = C_ACC;
    sc[ImGuiCol_ScrollbarGrabActive] = C_ACC; sc[ImGuiCol_TextSelectedBg] = ImVec4(C_ACC.x, C_ACC.y, C_ACC.z, 0.35f);
    sc[ImGuiCol_NavCursor] = C_ACC; sc[ImGuiCol_CheckMark] = C_ACC;
    sc[ImGuiCol_Border] = ImVec4(C_ACC.x, C_ACC.y, C_ACC.z, 0.15f);
    // build 9: combo arrows, selectables and headers in the menu palette (the default ImGui blue stood out, #10)
    sc[ImGuiCol_Button] = C_CARD2; sc[ImGuiCol_ButtonHovered] = C_HOVER; sc[ImGuiCol_ButtonActive] = C_ACCD;
    sc[ImGuiCol_Header] = C_ACCD; sc[ImGuiCol_HeaderHovered] = C_HOVER; sc[ImGuiCol_HeaderActive] = C_ACCD;
    sc[ImGuiCol_Separator] = C_LINE; sc[ImGuiCol_SliderGrab] = C_ACC; sc[ImGuiCol_SliderGrabActive] = C_ACC;
    if (T.dirty && NowSec() - T.lastSave > 0.6) T.SaveNow();   // settings are written in batches (no lag while dragging)
    PollBinding();

    Snapshot s;
    std::string toast; double toastUntil;
    { std::lock_guard<std::mutex> l(T.mtx); s = T.snap; toast = T.toast; toastUntil = T.toastUntil; }
    // game paused in its own menu (Esc) / loading: the game frame stopped -> hide our overlays
    bool live = NowSec() - T.lastTick < 0.3 && !T.inGameMenu;
    ImDrawList* bg = ImGui::GetBackgroundDrawList();
    if (live) {
        DrawEsp(s, dispW, dispH);
        g_fps = ImGui::GetIO().Framerate;
        g_ft[g_ftPos] = ImGui::GetIO().DeltaTime * 1000.f; g_ftPos = (g_ftPos + 1) % g_ft.size();
        if (s.inGame && gx::On("wtarget") && s.tValid) {   // build 17: target health bar, top centre
            ImDrawList* bgl = ImGui::GetBackgroundDrawList(); float bw = 360 * S, bx = dispW * 0.5f - bw * 0.5f, by = (gx::On("compass") ? 64 : 20) * S;
            bgl->AddRectFilled(ImVec2(bx, by), ImVec2(bx + bw, by + 46 * S), IM_COL32(20, 18, 28, 210), 8 * S);
            PF(fBold); bgl->AddText(ImVec2(bx + 12 * S, by + 6 * S), IM_COL32(240, 240, 250, 255), s.tName.c_str()); ImGui::PopFont();
            char tb[48]; snprintf(tb, sizeof tb, "%d / %d", s.tHp, s.tHpMax); PF(fSmall); ImVec2 ts = ImGui::CalcTextSize(tb);
            bgl->AddText(ImVec2(bx + bw - 12 * S - ts.x, by + 9 * S), IM_COL32(200, 196, 215, 255), tb); ImGui::PopFont();
            float f = s.tHpMax > 0 ? std::max(0.f, std::min(1.f, (float)s.tHp / s.tHpMax)) : 0;
            bgl->AddRectFilled(ImVec2(bx + 12 * S, by + 32 * S), ImVec2(bx + bw - 12 * S, by + 38 * S), IM_COL32(60, 40, 48, 255), 3 * S);
            bgl->AddRectFilled(ImVec2(bx + 12 * S, by + 32 * S), ImVec2(bx + 12 * S + (bw - 24 * S) * f, by + 38 * S), IM_COL32(220, 50, 60, 255), 3 * S);
        }
        if (s.inGame && (!menuOpen || gx::On("wmenu"))) DockOverlay(s, dispW, dispH, menuOpen);
        if (s.inGame && gx::On("compass")) { PF(fSmall); DrawCompass(ImGui::GetBackgroundDrawList(), ImVec2(dispW * 0.5f, 14 * S), gx::Val("compw") * S, s.rfx, s.rfz, gx::On("compmark") ? &s.radar : nullptr, S, 1.f); ImGui::PopFont(); }
        if (s.inGame && gx::On("esparrows") && gx::On("espnpc"))
            DrawArrows(ImGui::GetBackgroundDrawList(), dispW, dispH, s.ind, s.rfx, s.rfz, s.rrx, s.rrz, S, true, false);
        if (s.inGame && gx::On("espedge") && gx::On("espnpc")) {   // build 17: boxes outside the screen
            ImDrawList* bgl = ImGui::GetBackgroundDrawList(); float m = 30 * S, cx = dispW * 0.5f, cy = dispH * 0.5f;
            for (auto& e : s.edges) {
                float x = e.x * dispW - cx, y = e.z * dispH - cy;
                float tx = std::fabs(x) > 1e-3f ? (cx - m) / std::fabs(x) : 1e9f, ty = std::fabs(y) > 1e-3f ? (cy - m) / std::fabs(y) : 1e9f, t = std::min(1.f, std::min(tx, ty));
                ImVec2 p(cx + x * t, cy + y * t); ImU32 col = (e.color & 0x00FFFFFF) | 0xE6000000;
                bgl->AddRectFilled(ImVec2(p.x - 9 * S, p.y - 9 * S), ImVec2(p.x + 9 * S, p.y + 9 * S), IM_COL32(20, 18, 28, 200), 4 * S);
                bgl->AddRect(ImVec2(p.x - 9 * S, p.y - 9 * S), ImVec2(p.x + 9 * S, p.y + 9 * S), col, 4 * S, 1.6f * S);
                bgl->AddCircleFilled(p, 3 * S, col, 10);
            }
        }
        if (s.inGame && s.faHas && !menuOpen) {   // build 17: free aim - the character under the crosshair
            ImVec2 q(s.faX * dispW, s.faY * dispH);
            ImGui::GetBackgroundDrawList()->AddTriangleFilled(ImVec2(q.x, q.y + 4 * S), ImVec2(q.x - 7 * S, q.y - 8 * S), ImVec2(q.x + 7 * S, q.y - 8 * S), IM_COL32(255, 80, 80, 230));
        }
        if (s.inGame && gx::On("xhair") && !menuOpen) DrawXhair(ImGui::GetBackgroundDrawList(), ImVec2(dispW * 0.5f, dispH * 0.5f), XhCurrent(), S, gx::Val("xhalpha") / 100.f);
        std::vector<std::string> hud;
        if (s.paused) hud.push_back("PAUZA  ·  menu trainera");
        if (s.swapped) hud.push_back("OBCE CIAŁO  ·  powrót: menu (Insert) -> przycisk u góry");
        if (s.arena) hud.push_back("ARENA  ·  fala " + std::to_string(s.arenaWave) + "  ·  wrogów: " + std::to_string(s.arenaAlive));
        if (s.speed) { char b[48]; snprintf(b, sizeof b, "TEMPO ŚWIATA  x%.2f", s.speedVal); hud.push_back(b); }
        if (s.quicksilver) hud.push_back("QUICKSILVER");
        if (s.ghost) hud.push_back("TRYB DUCHA");
        if (s.noclip) hud.push_back(gx::On("glide") ? "ŚLIZG  ·  WASD  Q/E  ·  Esc = wyłącz" : "NO-CLIP  ·  WASD  Q/E  Spacja/Ctrl  ·  Esc = wyłącz");
        float hy = 20 * S;
        PF(fBold);
        for (auto& t : hud) {
            ImVec2 ts = ImGui::CalcTextSize(t.c_str());
            ImVec2 p(dispW - ts.x - 34 * S, hy);
            bg->AddRectFilled(ImVec2(p.x - 14 * S, p.y - 8 * S), ImVec2(p.x + ts.x + 14 * S, p.y + ts.y + 8 * S), U32(C_WIN), R_ITEM());
            bg->AddRect(ImVec2(p.x - 14 * S, p.y - 8 * S), ImVec2(p.x + ts.x + 14 * S, p.y + ts.y + 8 * S), U32(C_ACC, 0.15f), R_ITEM(), 1.f);
            bg->AddText(p, U32(C_TEXT), t.c_str());
            hy += ts.y + 22 * S;
        }
        ImGui::PopFont();
    }
    T.shotSelect = ShotHeld();
    if (T.shotSelect) ShotSelectOverlay(dispW, dispH);
    int swm = (int)gx::Val("statuswin");
    if (s.inGame && ((swm == 2 && (live || menuOpen)) || (swm == 1 && menuOpen))) StatusWindow(menuOpen, dispW);
    // build 16: the info window is a widget in the dock now (Wizualia › Widżety)
    // photo studio running: cover the screen so the parked camera is never seen
    if (s.photoBusy) {
        bool full = !menuOpen || s.photoBatch;
        ImDrawList* dl = full ? ImGui::GetForegroundDrawList() : bg;
        dl->AddRectFilled(ImVec2(0, 0), ImVec2(dispW, dispH), IM_COL32(12, 11, 18, 255));
        if (full) {
            char b[160];
            if (s.photoTotal > 1) snprintf(b, sizeof b, "Generuję portrety i ikony...  %d / %d", s.photoDone, s.photoTotal);
            else snprintf(b, sizeof b, "Generuję portret...");
            PF(fBig); ImVec2 ts = ImGui::CalcTextSize(b); dl->AddText(ImVec2(dispW * 0.5f - ts.x * 0.5f, dispH * 0.46f), U32(C_TEXT), b); ImGui::PopFont();
            float bw = 360 * S, f = s.photoTotal > 0 ? (float)s.photoDone / s.photoTotal : 0.f;
            ImVec2 b0(dispW * 0.5f - bw * 0.5f, dispH * 0.46f + 36 * S);
            dl->AddRectFilled(b0, ImVec2(b0.x + bw, b0.y + 6 * S), U32(C_TRACK), 3 * S);
            dl->AddRectFilled(b0, ImVec2(b0.x + bw * f, b0.y + 6 * S), U32(C_ACC), 3 * S);
            const char* h = "raz na postać - potem portret wczytuje się z pliku  ·  Esc przerywa";
            PF(fSmall); ImVec2 hs = ImGui::CalcTextSize(h); dl->AddText(ImVec2(dispW * 0.5f - hs.x * 0.5f, b0.y + 18 * S), U32(C_DIM), h); ImGui::PopFont();
        }
    }
    if ((live || menuOpen) && NowSec() < toastUntil && !toast.empty() && !ShotPending() && !T.shotSelect) {
        PF(fBold);
        ImVec2 ts = ImGui::CalcTextSize(toast.c_str());
        ImVec2 p(dispW * 0.5f - ts.x * 0.5f, dispH * (menuOpen ? 0.94f : 0.80f));
        ImDrawList* fg = menuOpen ? ImGui::GetForegroundDrawList() : bg;
        fg->AddRectFilled(ImVec2(p.x - 16 * S, p.y - 9 * S), ImVec2(p.x + ts.x + 16 * S, p.y + ts.y + 9 * S), U32(C_WIN), R_ITEM());
        fg->AddRect(ImVec2(p.x - 16 * S, p.y - 9 * S), ImVec2(p.x + ts.x + 16 * S, p.y + ts.y + 9 * S), U32(C_ACC, 0.35f), R_ITEM(), 1.f);
        fg->AddText(p, U32(C_ACC), toast.c_str());
        ImGui::PopFont();
    }
    if (!menuOpen || T.shotSelect) { editRow = nullptr; bindRow = nullptr; return; }   // build 17: no live menu while selecting

    ImGui::SetNextWindowSize(ImVec2(std::min(dispW * 0.92f, 1080 * S), std::min(dispH * 0.88f, 780 * S)), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(dispW * 0.5f, dispH * 0.5f), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(800 * S, 540 * S), ImVec2(dispW, dispH));
    ImGui::Begin("##g1trainer", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollWithMouse);
    ImVec2 wp = ImGui::GetWindowPos(), ws = ImGui::GetWindowSize();
    float sideW = 228 * S;
    Sidebar(s, wp, sideW, ws.y);

    // header: breadcrumb left, search right (same vertical centre)
    float cx = wp.x + sideW + 26 * S, cw = ws.x - sideW - 50 * S;
    float hcy = wp.y + 36 * S;
    const Sec& sec = kSecs[curSec];
    std::string crumbA = cfgPage ? "Konfiguracja" : testsPage ? "Testy" : sec.name, crumbB = (cfgPage || testsPage || sec.subs.size() == 1) ? "" : sec.subs[curSub];
    if (gsearch[0]) { crumbA = "Wyniki wyszukiwania"; crumbB = ""; }
    float sw = 240 * S, sh = 34 * S;
    float hx = wp.x + ws.x - 24 * S - sw;
    ImGui::SetCursorScreenPos(ImVec2(hx, hcy - sh * 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12 * S, (sh - fReg->LegacySize) * 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, R_ITEM());
    ImGui::PushStyleColor(ImGuiCol_FrameBg, C_CARD2);
    ImGui::SetNextItemWidth(sw);
    {   // Ctrl+F = jump to search (#14)
        static bool wantFocus = false;
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F, false)) wantFocus = true;
        if (wantFocus) { ImGui::SetKeyboardFocusHere(); wantFocus = false; }
    }
    ImGui::InputTextWithHint("##gq", "Szukaj funkcji...  (Ctrl+F)", gsearch, sizeof gsearch);
    ImGui::PopStyleColor(); ImGui::PopStyleVar(2);
    ClearX(gsearch);
    if (!gsearch[0]) IconAt(DL(), ImVec2(hx + sw - 18 * S, hcy), ICN_SEARCH, U32(C_FAINT), 13 * S);
    if (s.swapped) {
        const char* lbl = ICN_HOME "  Wróć do bohatera";
        float tw = ButtonW(lbl);
        hx -= tw + 10 * S;
        if (Button("##unswap", lbl, ImVec2(hx, hcy - BtnH() * 0.5f))) Act_Unswap();
    }
    float crumbMax = hx - 14 * S;
    float ta = TextW(crumbA.c_str(), fTitle);
    PF(fTitle); float th = ImGui::CalcTextSize("Ag").y; ImGui::PopFont();
    TextClip(ImVec2(cx, std::floor(hcy - th * 0.5f)), fTitle, crumbB.empty() ? C_TEXT : C_DIM, crumbA, crumbMax);
    if (!crumbB.empty() && cx + ta + 34 * S < crumbMax) {
        IconAt(DL(), ImVec2(cx + ta + 16 * S, hcy), ICO_CHEVR, U32(C_FAINT), 12 * S);
        TextClip(ImVec2(cx + ta + 32 * S, std::floor(hcy - th * 0.5f)), fTitle, C_TEXT, crumbB, crumbMax);
    }
    if (T.testerMode && !gsearch[0] && !cfgPage) {   // ticket for the whole subcategory / category
        float tw2 = crumbB.empty() ? ta : ta + 32 * S + TextW(crumbB.c_str(), fTitle);
        std::string tgt = crumbB.empty() ? "kat:" + crumbA : "pod:" + crumbA + " › " + crumbB;
        if (cx + tw2 + 40 * S < crumbMax) TicketFlag(tgt.c_str(), ImVec2(cx + tw2 + 12 * S, hcy - 11 * S));
    }

    // content
    ImGui::SetCursorScreenPos(ImVec2(cx, wp.y + 72 * S));
    ImGui::BeginChild("##content", ImVec2(cw + 12 * S, ws.y - 72 * S - 18 * S), ImGuiChildFlags_None);
    float w = cw - 4 * S;
    if (gsearch[0]) {
        auto cards = CardsFor("", "", gsearch);
        if (cards.empty()) ImGui::TextColored(C_DIM, "Nic nie znaleziono.");
        Masonry(cards, w);
    } else if (cfgPage) {
        ConfigPage(w);
    } else if (testsPage && T.testerMode) {
        TesterPage(w);
    } else {
        std::string cat = sec.name, sub = sec.subs[curSub];
        auto cards = CardsFor(cat, sub);
        if (cat == "Postać" && (sub == "Informacje" || sub == "Wygląd")) {
            SubjectBar(s, w);
            if (sub == "Informacje") {   // build 13: name (filled in, button active only after a change) + portrait refresh
                static uintptr_t nameFor = 0; static std::string nameOrig;
                if (nameFor != s.subjectPtr || (!ImGui::IsAnyItemActive() && nameOrig != s.subjectName && std::string(nameBuf) == nameOrig)) {
                    nameFor = s.subjectPtr; nameOrig = s.subjectName; snprintf(nameBuf, sizeof nameBuf, "%s", s.subjectName.c_str());
                }
                CardSpec nm{ "Imię i portret", {}, nullptr, 0, false };
                nm.customH = 108 * S;
                std::string key = s.subjectInst; uintptr_t sp = s.subjectPtr;
                nm.custom = [key, sp](float x, float y, float cw2) -> float {
                    float pic = 84 * S;
                    ImVec2 pp(x + 18 * S, y + 12 * S);
                    DL()->AddRectFilled(pp, ImVec2(pp.x + pic, pp.y + pic), U32(C_CARD2), R_ITEM());
                    if (void* tex = PortraitFor(key)) DL()->AddImage((ImTextureID)(intptr_t)tex, pp, ImVec2(pp.x + pic, pp.y + pic));
                    else { IconAt(DL(), ImVec2(pp.x + pic * 0.5f, pp.y + pic * 0.5f), ICN_USER, U32(C_FAINT), 26 * S); AskPhoto(sp, key); }
                    float ix = pp.x + pic + 14 * S, iw = x + cw2 - 18 * S - ix;
                    float bw = ButtonW("Zmień");
                    ImGui::SetCursorScreenPos(ImVec2(ix, y + 14 * S));
                    InputBox("##name", nameBuf, sizeof nameBuf, "imię postaci", iw - bw - 8 * S);
                    bool changed = nameBuf[0] && nameOrig != nameBuf;
                    if (Button("##setn", "Zmień", ImVec2(x + cw2 - 18 * S - bw, y + 14 * S), changed) && changed) { Act_SetName(nameBuf); nameOrig = nameBuf; }
                    if (Button("##photo", "Odśwież portret", ImVec2(ix, y + 14 * S + BtnH() + 10 * S), false)) Act_Photo(0, true);
                    return 108 * S;
                };
                cards.insert(cards.begin(), nm);
                if (CardSpec* c = FindCard(cards, "Poziom umiejętności")) c->headRow = T.Get("alltal");
            }
            if (sub == "Wygląd") {
                // build 11: no stand-in clone - the card shows the character's portrait and an always visible apply button
                // build 13: one card - look sliders, preview on a stand-in (auto or on button), apply
                CardSpec pv{ "Wygląd", {}, nullptr, 296 * S, false };
                for (const char* id : { "vhead", "vface", "vbody", "vskin" }) if (Row* r = T.Get(id)) pv.rows.push_back(r);
                cards.erase(std::remove_if(cards.begin(), cards.end(), [](const CardSpec& c) { return c.title == "Twarz i ciało"; }), cards.end());
                int sig = (int)gx::Val("vhead") * 1000000 + (int)gx::Val("vface") * 100 + (int)gx::Val("vbody") * 10 + (int)gx::Val("vskin");
                if (lookSig == 0) lookSig = sig;
                if (sig != lookSig) { lookSig = sig; lookDirty = true; if (gx::On("pvauto")) lookAt = NowSec() + 0.6; }
                if (lookAt > 0 && NowSec() > lookAt && !ImGui::IsMouseDown(0)) { lookAt = 0; Act_LookPreview(); }
                std::string key = PortraitHas("LOOKPREVIEW") ? std::string("LOOKPREVIEW") : s.subjectInst; uintptr_t sp = s.subjectPtr;
                pv.custom = [key, sp](float x, float y, float cw2) -> float {
                    float sz = 170 * S;
                    ImVec2 c(x + cw2 * 0.5f, y + 8 * S + sz * 0.5f);
                    if (void* tex = PortraitFor(key)) DL()->AddImage((ImTextureID)(intptr_t)tex, ImVec2(c.x - sz * 0.5f, c.y - sz * 0.5f), ImVec2(c.x + sz * 0.5f, c.y + sz * 0.5f));
                    else IconAt(DL(), c, ICN_USER, U32(C_FAINT), 40 * S);
                    if (key == "LOOKPREVIEW") Text(ImVec2(x + 18 * S, y + 10 * S), fSmall, C_FAINT, "podgląd");
                    float by = y + sz + 16 * S;
                    float b1 = ButtonW("Zastosuj wygląd"), b2 = ButtonW("Odśwież podgląd");
                    float bx = x + (cw2 - b1 - b2 - 8 * S) * 0.5f;
                    if (Button("##lookpv", "Odśwież podgląd", ImVec2(bx, by), false)) { Act_LookPreview(); lookDirty = false; }
                    if (Button("##lookok", "Zastosuj wygląd", ImVec2(bx + b2 + 8 * S, by), true)) { Act_ApplyLook(); lookDirty = false; }
                    ImVec2 cp(x + 18 * S, by + BtnH() + 10 * S);
                    Row* pa = T.Get("pvauto");
                    if (pa && Check("##pva", pa->on, cp)) { pa->on = !pa->on; T.Save(); }
                    Text(ImVec2(cp.x + 26 * S, cp.y + 2 * S), fSmall, C_DIM, "Odświeżaj podgląd przy każdej zmianie (klon pojawia się na chwilę, ekran przygasa)");
                    return 296 * S;
                };
                cards.insert(cards.begin(), pv);
            }
            Masonry(cards, w);
        }
        else if (cat == "Postać" && sub == "Ruch") {
            TabbedCard(cards, "Latanie i ślizg", { { "No-clip", { "noclip", "ncspeed", "ncturn", "ncmouse", "ncmsens", "ncignore" } },
                                                   { "Ślizg", { "glide", "glcoll", "glideslope", "glspeed", "glturn", "glmouse", "glmsens", "glignore" } } });
            Masonry(cards, w);
        }
        else if (cat == "Postać" && sub == "Cheaty") {
            TabbedCard(cards, "Ukrycie", { { "Tryb ducha", { "ghost" } }, { "Tylko broń", { "noweapon" } } });
            Masonry(cards, w);
        }
        else if (cat == "Postać" && sub == "Walka") {
            TabbedCard(cards, "Celowanie", { { "Aimbot", { "aimbot", "aimkey", "aimdelay", "aimprio", "aimfov", "aimdist", "aimstick", "aimnonvis" } },
                                             { "Trigger bot", { "trigbot", "trigdelay", "trigcd", "trigkeys", "aimdist" } } });
            ChecksCard(cards, "Swobodne celowanie", { "freeranged", "freemagic" }, "Dla");
            ChecksCard(cards, "Cele aimbota i trigger bota", { "aimen", "aimne", "aimmo", "aimhu" });
            Masonry(cards, w);
        }
        else if (cat == "Postacie" && sub == "Cel") {
            TargetPage(s, w);
            if (CardSpec* c = FindCard(cards, "Magia na celu")) {
                c->customH = TargetSpells(0, 0, 0, false);
                c->custom = [](float x, float y, float cw2) { return TargetSpells(x, y, cw2, true); };
            }
            Masonry(cards, w);
        }
        else if (cat == "Ekwipunek" && sub == "Spawner") ItemSpawner(w);
        else if (cat == "Ekwipunek" && sub == "Zestawy") { Masonry(cards, w); SetsPage(w); }
        else if (cat == "Ekwipunek" && sub == "Zbieranie") {
            std::vector<const char*> ids = { "lootc0", "lootc1", "lootc2", "lootc3", "lootc4", "lootc5", "lootc6", "lootc7", "lootc8", "lootc9", "lootc10", "lootc11", "lootore" };
            ChecksCard(cards, "Co podnosić", ids);
            Masonry(cards, w); MapItemsPage(s, w);
        }
        else if (cat == "Postacie" && sub == "Spawner") { Masonry(cards, w); NpcSpawner(s, w); }
        else if (cat == "Postacie" && sub == "Drużyna") { Masonry(cards, w); PartyPage(s, w); }
        else if (cat == "Postacie" && sub == "Otoczenie") { Masonry(cards, w); EnvPage(s, w); }
        else if (cat == "Fun" && sub == "Walka") {
            // build 16: options split by meaning: who / how (apocalypse); whom / into what / effects (polymorph)
            // build 17: all options of a feature in its own card, grouped inside it
            ChecksSections(cards, "Apokalipsa", { { "Kto powstaje", { "zombz", "zombsk", "zombot" } }, { "Zachowanie", { "zombhunt", "zombmortal", "zombfat", "zombname" } } });
            ChecksSections(cards, "Polimorfia", { { "Kogo przemieniać", { "polyhum", "polymon", "polyparty", "polyimm" } }, { "W co", { "polyanim", "polybeast", "polyundead", "polyorc" } },
                                                  { "Efekty", { "polyfx", "polyhostile", "polykeep" } } });
            Masonry(cards, w);
        }
        else if (cat == "Fun" && sub == "Magia") {
            if (CardSpec* c = FindCard(cards, "Deszcz zaklęć")) {
                c->customH = RainSpells(0, 0, 0, false);
                c->custom = [](float x, float y, float cw2) { return RainSpells(x, y, cw2, true); };
            }
            Masonry(cards, w);
        }
        else if (cat == "Świat" && sub == "Teleport") { Masonry(cards, w); TeleportPage(s, w); }
        else if (cat == "Wizualia") {
            // build 14: every tab starts with a preview of what its settings draw
            CardSpec pv{ "Podgląd", {}, nullptr, 0, true };
            std::function<float(float, float, float, bool)> f;
            if (sub == "ESP postaci") f = EspNpcPreview;
            else if (sub == "ESP przedmiotów") f = EspItemPreview;
            else if (sub == "Kolory") f = ColorsPreview;
            else if (sub == "Widżety") f = WidgetsPreview;
            else if (sub == "Celownik") f = XhPreview;
            else if (sub == "Grafika") f = [](float x, float y, float cw, bool draw) -> float {
                float h = 80 * S; if (!draw) return h;
                Text(ImVec2(x + 18 * S, y + 10 * S), fReg, C_TEXT, "Podgląd to sama gra - zmiany widać od razu za menu.");
                Text(ImVec2(x + 18 * S, y + 32 * S), fSmall, C_DIM, "Filtr obrazu nie obejmuje menu, widżetów i ESP. Silnik GD3D11: zasięgi, mgła, słońce, cienie, wiatr.");
                std::string gi = "GD3D11: " + GdxInfo();
                Text(ImVec2(x + 18 * S, y + 52 * S), fSmall, GdxAvailable() ? C_FAINT : C_ST_BUG, gi.c_str());
                return h; };
            if (f) { pv.customH = f(0, 0, w, false); pv.custom = [f](float x, float y, float cw2) { return f(x, y, cw2, true); }; cards.insert(cards.begin(), pv); }
            if (sub == "Grafika") {
                if (!g_gfxPresets.loaded) PresetLoad(g_gfxPresets, GfxBuiltins());
                CardSpec gp{ "Presety grafiki", {}, nullptr, PresetsCard(g_gfxPresets, 0, 0, w * 0.5f, false), false };
                gp.custom = [](float x, float y, float cw2) { return PresetsCard(g_gfxPresets, x, y, cw2, true); };
                cards.insert(cards.begin() + std::min<size_t>(1, cards.size()), gp);
                if (!GdxDistances()) for (auto& c : cards) c.rows.erase(std::remove_if(c.rows.begin(), c.rows.end(), [](Row* r) {
                    return r->id == "gdxdraw" || r->id == "gdxsmall" || r->id == "gdxnpc" || r->id == "gdxfx" || r->id == "gdxsect"; }), c.rows.end());
                if (!GdxAvailable()) for (auto& c : cards) c.rows.erase(std::remove_if(c.rows.begin(), c.rows.end(), [](Row* r) { return r->id.rfind("gdx", 0) == 0; }), c.rows.end());
                cards.erase(std::remove_if(cards.begin(), cards.end(), [](const CardSpec& k) { return k.rows.empty() && !k.custom; }), cards.end());
            }
            if (sub == "Celownik") {
                // build 17: settings card built from the chosen shape + colour inside it
                for (auto& c : cards) c.rows.erase(std::remove_if(c.rows.begin(), c.rows.end(), [](Row* r) { return r->id.rfind("xh", 0) == 0; }), c.rows.end());
                cards.erase(std::remove_if(cards.begin(), cards.end(), [](const CardSpec& k) { return k.rows.empty() && !k.custom; }), cards.end());
                CardSpec xs{ "Celownik", XhRows(), nullptr, XhColor(0, 0, 0, false), false };
                xs.custom = [](float x, float y, float cw2) { return XhColor(x, y, cw2, true); };
                cards.insert(cards.begin() + std::min<size_t>(1, cards.size()), xs);
                CardSpec ps{ "Presety", {}, nullptr, XhPresets(0, 0, w * 0.5f, false), false };
                ps.custom = [](float x, float y, float cw2) { return XhPresets(x, y, cw2, true); };
                cards.push_back(ps);
            }
            if (sub == "ESP postaci") ChecksCard(cards, "Informacje", { "espname", "espdist", "espguild", "esptype", "esphp", "esphpnum", "espeq", "esptarget", "espdead" }, "Co pokazywać");
            if (sub == "ESP przedmiotów") ChecksCard(cards, "Co pokazywać", { "espitems", "espchest", "espchestinv", "espactive", "espammo" });
            if (sub == "Widżety") {
                ChecksCard(cards, "Radar", { "raden", "radne", "radmo", "radpa", "radardead", "radaritems", "radarchests", "radarnames" }, "Co pokazywać");
                ChecksCard(cards, "Kompas", { "cmen", "cmne", "cmmo", "cmpa", "cmdead", "cmitems", "cmchests" }, "Co pokazywać");
                ChecksCard(cards, "Informacje", { "hudfps", "hudframe", "hudgraph", "hudtick", "hudpos", "hudspeed", "hudheading", "hudtime", "hudclock", "hudworld", "hudstats", "hudtarget", "hudmem", "hudcount" }, "Co pokazywać");
            }
            if (sub == "Kolory") {
                CardSpec col{ "Kolory", {}, nullptr, EspColors(0, 0, 0, false), false };
                col.custom = [](float x, float y, float cw2) { return EspColors(x, y, cw2, true); };
                cards.push_back(col);
            }
            Masonry(cards, w);
        }
        else if (cat == "Narzędzia" && sub == "Nakładka") {   // build 13: one card, checkmarks for the lines
            if (!cards.empty()) {
                static const char* ids[] = { "hudfps", "hudpos", "hudspeed", "hudtime", "hudworld", "hudstats", "hudtarget", "hudmods" };
                cards[0].full = true;
                cards[0].customH = 34 * S + ((int)(sizeof ids / sizeof ids[0]) + 1) / 2 * 34 * S + 8 * S;
                cards[0].custom = [](float x, float y, float cw2) -> float {
                    Text(ImVec2(x + 18 * S, y + 10 * S), fBold, C_TEXT, "Co pokazywać");
                    int i = 0;
                    for (auto id : ids) {
                        Row* r = T.Get(id); if (!r) continue;
                        float cx = x + 18 * S + (i % 2) * (cw2 - 36 * S) * 0.5f, cy = y + 40 * S + (i / 2) * 34 * S;
                        ImGui::PushID(id);
                        if (Check("##c", r->on, ImVec2(cx, cy))) { r->on = !r->on; T.Save(); }
                        ImGui::PopID();
                        Text(ImVec2(cx + 28 * S, cy + 1 * S), fReg, r->on ? C_TEXT : C_DIM, r->name.c_str());
                        if (!r->note.empty()) Text(ImVec2(cx + 28 * S + TextW(r->name.c_str(), fReg) + 10 * S, cy + 3 * S), fSmall, C_FAINT, r->note.c_str());
                        i++;
                    }
                    return 34 * S + (i + 1) / 2 * 34 * S + 8 * S;
                };
            }
            Masonry(cards, w);
        }
        else if (cat == "Narzędzia" && sub == "Konsola") { Masonry(cards, w); ConsolePanel(s, w); }
        else if (cat == "Narzędzia" && sub == "Kompendium") CompendiumPage(w);
        else if (cat == "Narzędzia" && sub == "Diagnostyka") { Masonry(cards, w); DiagPage(s, w); }
        else Masonry(cards, w);
    }
    ImGui::Dummy(ImVec2(0, 16 * S));
    ImGui::EndChild();
    // resize grip: always-visible diagonal lines in the bottom-right corner
    {
        ImVec2 br(wp.x + ws.x - 7 * S, wp.y + ws.y - 7 * S);
        bool hv = ImGui::IsMouseHoveringRect(ImVec2(br.x - 18 * S, br.y - 18 * S), ImVec2(br.x + 7 * S, br.y + 7 * S), false);
        ImU32 gc = U32(hv ? C_ACC : C_FAINT);
        for (int k = 1; k <= 3; k++) DL()->AddLine(ImVec2(br.x - k * 5 * S, br.y), ImVec2(br.x, br.y - k * 5 * S), gc, 1.4f * S);
    }
    ImGui::End();
    if (testsWin && T.testerMode) TestsWindow(dispW);
}
