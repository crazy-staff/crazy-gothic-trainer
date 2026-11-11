// Menu (Dear ImGui over GD3D11). The user's trainer design: dark/purple(accent), tabs,
// 4-column rows, rounded corners, status pills; plus cards for target/player/ESP.
#include "ui.h"
#include "trainer.h"
#include "core/catalog.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

extern unsigned g_espColors[7];

#define ICO_SEARCH "\xef\x80\x82"
#define ICO_COG "\xef\x80\x93"
#define ICO_HOME "\xef\x80\x95"
#define ICO_USER "\xef\x80\x87"
#define ICO_PAW "\xef\x86\xb0"
#define ICO_TIMES "\xef\x80\x8d"
#define ICO_PLUS "\xef\x81\xa7"
#define ICO_PLAY "\xef\x81\x8b"
#define ICO_TRASH "\xef\x87\xb8"
#define ICO_BOOKMARK "\xef\x80\xae"
#define ICO_EDIT "\xef\x81\x84"
#define ICO_SAVE "\xef\x83\x87"

static inline void PF(ImFont* f) { ImGui::PushFont(f, f->LegacySize); }
namespace {
const ImVec4 C_BG(0.07f, 0.07f, 0.10f, 0.95f), C_ROW1(0.12f, 0.12f, 0.17f, 1), C_ROW2(0.10f, 0.10f, 0.14f, 1),
    C_HOVER(0.18f, 0.17f, 0.27f, 1), C_TEXT(0.93f, 0.93f, 0.97f, 1), C_DIM(0.60f, 0.60f, 0.68f, 1),
    C_ON(0.45f, 0.95f, 0.55f, 1), C_OFF(0.85f, 0.85f, 0.90f, 1), C_NUM(0.40f, 0.85f, 1.00f, 1),
    C_TRACK(0.25f, 0.25f, 0.33f, 1), C_BTN(0.16f, 0.15f, 0.23f, 1), C_BTNH(0.24f, 0.21f, 0.38f, 1), C_CARD(0.11f, 0.11f, 0.155f, 1),
    C_ST_OK(0.45f, 0.95f, 0.55f, 1), C_ST_BUG(0.98f, 0.80f, 0.30f, 1), C_ST_CRASH(1.0f, 0.38f, 0.38f, 1), C_ST_NEW(0.42f, 0.70f, 1.0f, 1);
ImVec4 C_ACC(0.66f, 0.55f, 1.00f, 1), C_EDIT(0.25f, 0.20f, 0.45f, 1);
inline ImU32 U32(const ImVec4& c, float a = -1) { return ImGui::GetColorU32(ImVec4(c.x, c.y, c.z, a < 0 ? c.w : a)); }
inline ImVec4 Mix(const ImVec4& a, const ImVec4& b, float t) { return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, 1); }

ImFont *fReg, *fBold, *fBig, *fSmall, *fIcon;
float S = 1.f;
int tab = 0;
const char* kTabs[] = { "Gracz", "Przedmioty", "Potwory i NPC", "Cel", "Chaos", "Świat", "Render", "Teleport", "Marvin" };
const int kTabCount = sizeof(kTabs) / sizeof(*kTabs);
Row* editRow = nullptr; char editBuf[32]; bool editFirst = false;
Row* bindRow = nullptr;
char search[96] = "", gsearch[96] = "";
bool globalSearch = false, gsearchFocus = false, settingsOpen = false;
int itemCat = -1, npcGroup = -1, itemSort = 0; std::string itemSub;
char consoleBuf[2048] = "", scriptBuf[96] = "", posName[64] = "", macroName[64] = "", setName[64] = "";
int macroRepeat = 1;
float manX = 0, manY = 0, manZ = 0;
bool iconsOk = false;

std::string KeyName(int vk) {
    if (!vk) return "-";
    UINT sc = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
    switch (vk) {
    case VK_INSERT: case VK_DELETE: case VK_HOME: case VK_END: case VK_PRIOR: case VK_NEXT:
    case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN: sc |= 0x100; break;
    }
    char b[64];
    if (GetKeyNameTextA((LONG)(sc << 16), b, sizeof b) > 0) return b;
    snprintf(b, sizeof b, "VK %d", vk);
    return b;
}

bool TextButton(const char* id, const char* label, const ImVec4& col, ImFont* font, ImVec2 pos, bool pill = true) {
    PF(font);
    ImVec2 ts = ImGui::CalcTextSize(label);
    ImVec2 pad(8 * S, 4 * S);
    ImGui::SetCursorScreenPos(pos);
    bool clicked = ImGui::InvisibleButton(id, ImVec2(ts.x + pad.x * 2, ts.y + pad.y * 2));
    bool hov = ImGui::IsItemHovered();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (pill) dl->AddRectFilled(pos, ImVec2(pos.x + ts.x + pad.x * 2, pos.y + ts.y + pad.y * 2), U32(hov ? C_BTNH : C_BTN), 6 * S);
    dl->AddText(ImVec2(pos.x + pad.x, pos.y + pad.y), U32(col), label);
    ImGui::PopFont();
    return clicked;
}
float TextW(const char* label, ImFont* f) { PF(f); float w = ImGui::CalcTextSize(label).x; ImGui::PopFont(); return w; }

// square icon button (Fork Awesome glyph) with tooltip
bool IconButton(const char* id, const char* glyph, const char* tip, ImVec2 pos, float size, const ImVec4& col, bool accentBg = false) {
    ImGui::SetCursorScreenPos(pos);
    bool c = ImGui::InvisibleButton(id, ImVec2(size, size));
    bool hov = ImGui::IsItemHovered();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pos, ImVec2(pos.x + size, pos.y + size), U32(accentBg ? C_EDIT : (hov ? C_BTNH : C_BTN)), 7 * S);
    if (iconsOk) {
        PF(fIcon);
        ImVec2 ts = ImGui::CalcTextSize(glyph);
        dl->AddText(ImVec2(pos.x + (size - ts.x) * 0.5f, pos.y + (size - ts.y) * 0.5f), U32(hov ? C_TEXT : col), glyph);
        ImGui::PopFont();
    } else {
        PF(fSmall); ImVec2 ts = ImGui::CalcTextSize(tip); dl->AddText(ImVec2(pos.x + 3 * S, pos.y + (size - ts.y) * 0.5f), U32(col), tip); ImGui::PopFont();
    }
    if (hov && tip) ImGui::SetTooltip("%s", tip);
    return c;
}

void StatusPill(ImDrawList* dl, ImVec2 p, Status st, const std::string& hist, float* outW) {
    const char* t; ImVec4 c;
    switch (st) { case ST_OK: t = "OK"; c = C_ST_OK; break; case ST_BUG: t = "BUG"; c = C_ST_BUG; break;
                  case ST_CRASH: t = "CRASH"; c = C_ST_CRASH; break; default: t = "TEST"; c = C_ST_NEW; }
    PF(fSmall);
    ImVec2 ts = ImGui::CalcTextSize(t);
    ImVec2 a(p.x, p.y), b(p.x + ts.x + 10 * S, p.y + ts.y + 2 * S);
    dl->AddRectFilled(a, b, U32(c, 0.18f), 4 * S);
    dl->AddText(ImVec2(p.x + 5 * S, p.y + 1 * S), U32(c), t);
    ImGui::PopFont();
    if (ImGui::IsMouseHoveringRect(a, b) && !hist.empty()) ImGui::SetTooltip("%s", hist.c_str());
    *outW = b.x - a.x;
}

void BeginEdit(Row& r) { editRow = &r; editFirst = true; snprintf(editBuf, sizeof editBuf, r.isFloat ? "%.2f" : "%.0f", r.val); }
void EditField(Row& r, ImVec2 pos, float w) {
    ImGui::SetCursorScreenPos(pos);
    ImGui::SetNextItemWidth(w);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, C_EDIT);
    if (editFirst) ImGui::SetKeyboardFocusHere();
    bool done = ImGui::InputText("##ed", editBuf, sizeof editBuf,
        ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
    ImGui::PopStyleColor();
    if (done || (!ImGui::IsItemActive() && !editFirst)) {
        if (!ImGui::IsKeyPressed(ImGuiKey_Escape)) r.val = std::max(r.vmin, std::min(r.vmax, (float)atof(editBuf)));
        editRow = nullptr; T.Save();
    }
    editFirst = false;
}
void PushComboStyle() {
    ImGui::PushStyleColor(ImGuiCol_FrameBg, C_BTN); ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, C_BTNH);
    ImGui::PushStyleColor(ImGuiCol_Text, C_NUM); ImGui::PushStyleColor(ImGuiCol_Button, C_BTN);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, C_BTNH); ImGui::PushStyleColor(ImGuiCol_Header, C_EDIT);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, C_HOVER);
}
bool Combo(Row& r, ImVec2 pos, float w) {
    std::string cur;
    for (auto& o : r.options) if (o.first == (int)r.val) cur = o.second;
    ImGui::SetCursorScreenPos(pos);
    ImGui::SetNextItemWidth(w);
    PushComboStyle();
    bool changed = false;
    if (ImGui::BeginCombo("##cmb", cur.c_str())) {
        for (auto& o : r.options) {
            bool sel = o.first == (int)r.val;
            ImGui::PushStyleColor(ImGuiCol_Text, sel ? C_ACC : C_TEXT);
            if (ImGui::Selectable(o.second.c_str(), sel)) { r.val = (float)o.first; changed = true; T.Save(); }
            ImGui::PopStyleColor();
        }
        ImGui::EndCombo();
    }
    ImGui::PopStyleColor(7);
    return changed;
}
void Slider(Row& r, float x0, float x1, float ty) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(ImVec2(x0, ty - 2 * S), ImVec2(x1, ty + 2 * S), U32(C_TRACK), 2 * S);
    float t = (r.val - r.vmin) / (r.vmax - r.vmin);
    float kx = x0 + (x1 - x0) * t;
    dl->AddRectFilled(ImVec2(x0, ty - 2 * S), ImVec2(kx, ty + 2 * S), U32(C_ACC), 2 * S);
    dl->AddCircleFilled(ImVec2(kx, ty), 6 * S, U32(C_TEXT), 16);
    ImGui::SetCursorScreenPos(ImVec2(x0 - 6 * S, ty - 9 * S));
    ImGui::InvisibleButton("##trk", ImVec2(x1 - x0 + 12 * S, 18 * S));
    if (ImGui::IsItemActive()) {
        float nt = std::max(0.f, std::min(1.f, (ImGui::GetIO().MousePos.x - x0) / (x1 - x0)));
        float nv = r.vmin + nt * (r.vmax - r.vmin);
        r.val = r.isFloat ? std::round(nv * 100.f) / 100.f : std::round(nv);
    }
    if (ImGui::IsItemDeactivated()) T.Save();
}

void ValueCell(Row& r, ImVec2 p, float w, float h) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (!r.options.empty()) { Combo(r, ImVec2(p.x, p.y + h * 0.5f - 13 * S), w * 0.95f); return; }
    char b[32];
    snprintf(b, sizeof b, r.isFloat ? "%.2f" : "%.0f", r.val);
    if (r.kind == K_SLIDER) {
        float x0 = p.x, x1 = p.x + w * 0.85f, ty = p.y + h * 0.68f;
        char lb[40]; snprintf(lb, sizeof lb, r.isFloat ? "x%.2f" : "%.0f", r.val);
        PF(fBold);
        if (editRow == &r) EditField(r, ImVec2(x0, p.y + h * 0.08f), 80 * S);
        else {
            ImGui::SetCursorScreenPos(ImVec2(x0, p.y + h * 0.12f));
            ImVec2 ts = ImGui::CalcTextSize(lb);
            if (ImGui::InvisibleButton("##vl", ts)) BeginEdit(r);
            dl->AddText(ImVec2(x0, p.y + h * 0.12f), U32(C_NUM), lb);
        }
        ImGui::PopFont();
        Slider(r, x0, x1, ty);
        return;
    }
    if (r.noValue || r.kind == K_TOGGLE) return;
    PF(fBold);
    if (editRow == &r) EditField(r, ImVec2(p.x, p.y + h * 0.5f - 13 * S), 110 * S);
    else {
        ImVec2 ts = ImGui::CalcTextSize(b);
        ImVec2 tp(p.x, p.y + h * 0.5f - ts.y * 0.5f);
        ImGui::SetCursorScreenPos(tp);
        if (ImGui::InvisibleButton("##val", ImVec2(std::max(ts.x, 40 * S), ts.y))) BeginEdit(r);
        if (ImGui::IsItemHovered()) dl->AddRectFilled(ImVec2(tp.x - 4 * S, tp.y - 2 * S), ImVec2(tp.x + ts.x + 4 * S, tp.y + ts.y + 2 * S), U32(C_EDIT, 0.6f), 4 * S);
        dl->AddText(tp, U32(C_NUM), b);
    }
    ImGui::PopFont();
}

void DrawRow(Row& r, int idx, float w, const char* tabHint = nullptr) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float h = 50 * S;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::PushID(r.id.c_str());
    bool hov = ImGui::IsMouseHoveringRect(p, ImVec2(p.x + w, p.y + h)) && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), U32(hov ? C_HOVER : (idx & 1 ? C_ROW2 : C_ROW1)), 6 * S);
    if (hov && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && r.kind == K_SET && r.on) { r.on = false; T.Toast(r.name + ": przywrócono wartość gry"); T.Save(); }
    float x = p.x + 10 * S;
    if (!r.icon.empty() && iconsOk) {
        PF(fIcon); dl->AddText(ImVec2(x, p.y + 8 * S), U32(C_ACC), r.icon.c_str()); ImGui::PopFont();
    }
    x += 20 * S;
    float pw;
    StatusPill(dl, ImVec2(x, p.y + 9 * S), r.status, r.hist, &pw);
    PF(fBold);
    dl->AddText(ImVec2(x + pw + 6 * S, p.y + 7 * S), U32(C_TEXT), r.name.c_str());
    float nameEnd = x + pw + 6 * S + ImGui::CalcTextSize(r.name.c_str()).x;
    ImGui::PopFont();
    if (r.kind == K_SET && r.on) {
        PF(fSmall);
        char a[40]; snprintf(a, sizeof a, "AKTYWNE: %.0f", r.val);
        ImVec2 ts = ImGui::CalcTextSize(a);
        ImVec2 ap(nameEnd + 8 * S, p.y + 9 * S);
        dl->AddRectFilled(ap, ImVec2(ap.x + ts.x + 10 * S, ap.y + ts.y + 2 * S), U32(C_ON, 0.16f), 4 * S);
        dl->AddText(ImVec2(ap.x + 5 * S, ap.y + 1 * S), U32(C_ON), a);
        ImGui::PopFont();
    }
    PF(fSmall);
    std::string note = r.note.empty() ? r.sub : r.note;
    if (tabHint) note = std::string(tabHint) + "  ›  " + note;
    ImVec2 clipMax(p.x + w * 0.39f, p.y + h);
    dl->PushClipRect(p, clipMax, true);
    dl->AddText(ImVec2(p.x + 30 * S, p.y + 29 * S), U32(C_DIM), note.c_str());
    dl->PopClipRect();
    ImGui::PopFont();
    if (hov && !r.note.empty() && ImGui::IsMouseHoveringRect(ImVec2(p.x, p.y + 26 * S), clipMax)) ImGui::SetTooltip("%s", r.note.c_str());
    ValueCell(r, ImVec2(p.x + w * 0.40f, p.y), w * 0.26f, h);
    if (r.kind != K_VALUE) {
        std::string kn = bindRow == &r ? "naciśnij klawisz..." : KeyName(r.hotkey);
        PF(fBold);
        ImVec2 ts = ImGui::CalcTextSize(kn.c_str());
        ImVec2 kp(p.x + w * 0.68f, p.y + h * 0.5f - ts.y * 0.5f);
        ImGui::SetCursorScreenPos(kp);
        if (ImGui::InvisibleButton("##hk", ImVec2(std::max(ts.x, 30 * S), ts.y))) bindRow = &r;
        dl->AddText(kp, U32(bindRow == &r ? C_ON : C_ACC), kn.c_str());
        ImGui::PopFont();
        const char* lbl; ImVec4 col = C_OFF; ImFont* f = fReg;
        if (r.kind == K_TOGGLE || r.kind == K_SLIDER) { lbl = r.on ? "[ ON ]" : "[ OFF ]"; if (r.on) { col = C_ON; f = fBold; } }
        else if (r.kind == K_SET) { lbl = "[ Ustaw ]"; if (r.on) col = C_ON; }
        else if (r.kind == K_CHOICE) lbl = "[ Ustaw ]";
        else lbl = "[ Wykonaj ]";
        PF(f); float bh = ImGui::CalcTextSize(lbl).y + 8 * S; ImGui::PopFont();
        float bx = p.x + w * 0.84f;
        if (TextButton("##btn", lbl, col, f, ImVec2(bx, p.y + h * 0.5f - bh * 0.5f))) ApplyRow(r);
        if (r.kind == K_SET && r.on) {
            float bw = TextW(lbl, f) + 16 * S;
            if (TextButton("##off", "x", C_ST_CRASH, fBold, ImVec2(bx + bw + 4 * S, p.y + h * 0.5f - bh * 0.5f))) {
                r.on = false; T.Toast(r.name + ": przywrócono wartość gry"); T.Save();
            }
        }
    }
    ImGui::PopID();
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + 4 * S));
    ImGui::Dummy(ImVec2(w, 0));
}

void SubHeader(const std::string& t) {
    ImGui::Dummy(ImVec2(0, 2 * S));
    PF(fSmall); ImGui::TextColored(C_ACC, "%s", t.c_str()); ImGui::PopFont();
}
void SectionTitle(const char* t) { ImGui::Dummy(ImVec2(0, 6 * S)); PF(fBold); ImGui::TextColored(C_ACC, "%s", t); ImGui::PopFont(); }

bool Chip(const char* label, bool active) {
    PF(active ? fBold : fReg);
    ImVec2 ts = ImGui::CalcTextSize(label, nullptr, true);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImVec2 sz(ts.x + 18 * S, ts.y + 8 * S);
    float right = ImGui::GetWindowPos().x + ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - 8 * S;
    if (p.x + sz.x > right) { ImGui::NewLine(); p = ImGui::GetCursorScreenPos(); }
    bool c = ImGui::InvisibleButton(label, sz);
    bool hov = ImGui::IsItemHovered();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2(p.x + sz.x, p.y + sz.y), U32(active ? C_EDIT : (hov ? C_BTNH : C_BTN)), sz.y * 0.5f);
    dl->AddText(nullptr, 0, ImVec2(p.x + 9 * S, p.y + 4 * S), U32(active ? C_ACC : (hov ? C_TEXT : C_DIM)), label, ImGui::FindRenderedTextEnd(label));
    ImGui::PopFont();
    ImGui::SameLine(0, 6 * S);
    return c;
}
void InputBox(const char* id, char* buf, size_t n, const char* hint, float w) {
    ImGui::SetNextItemWidth(w);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, C_ROW1);
    ImGui::InputTextWithHint(id, hint, buf, n);
    ImGui::PopStyleColor();
}
std::string Lower(std::string s) { for (auto& c : s) c = (char)tolower((unsigned char)c); return s; }
bool MatchQ(const char* q, const std::string& a, const std::string& b) {
    if (!q[0]) return true;
    std::string ql = Lower(q);
    return Lower(a).find(ql) != std::string::npos || Lower(b).find(ql) != std::string::npos;
}
void ListRowBg(ImVec2 p, float w, float h, int i) {
    bool hov = ImGui::IsMouseHoveringRect(p, ImVec2(p.x + w, p.y + h)) && ImGui::IsWindowHovered();
    ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + w, p.y + h), U32(hov ? C_HOVER : (i & 1 ? C_ROW2 : C_ROW1)), 6 * S);
}
void NumField(const char* id, int* v, int mn, int mx, float w) {
    ImGui::SetNextItemWidth(w);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, C_ROW1); ImGui::PushStyleColor(ImGuiCol_Text, C_NUM);
    ImGui::InputInt(id, v, 0, 0);
    ImGui::PopStyleColor(2);
    *v = std::max(mn, std::min(mx, *v));
}
void Pill(ImDrawList* dl, ImVec2 p, const char* t, const ImVec4& c, float* w = nullptr) {
    PF(fSmall); ImVec2 ts = ImGui::CalcTextSize(t);
    dl->AddRectFilled(p, ImVec2(p.x + ts.x + 10 * S, p.y + ts.y + 2 * S), U32(c, 0.18f), 4 * S);
    dl->AddText(ImVec2(p.x + 5 * S, p.y + 1 * S), U32(c), t); ImGui::PopFont();
    if (w) *w = ts.x + 10 * S;
}

// ---------------------------------------------------------------------------- items
std::vector<std::string> SubsOf(int cat) {
    std::vector<std::string> subs;
    for (auto& e : g_cat.items) if (e.cat == cat && std::find(subs.begin(), subs.end(), e.sub) == subs.end()) subs.push_back(e.sub);
    std::sort(subs.begin(), subs.end());
    return subs;
}
void SetsPanel(float w) {
    SectionTitle("Moje zestawy");
    if (T.sets.empty()) ImGui::TextColored(C_DIM, "Nie masz jeszcze zestawów. Dodawaj przedmioty przyciskiem  +  na liście poniżej, potem zapisz zestaw.");
    float h = 38 * S;
    for (int i = 0; i < (int)T.sets.size(); i++) {
        ItemSet& s = T.sets[i];
        ImVec2 p = ImGui::GetCursorScreenPos();
        ListRowBg(p, w, h, i);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        PF(fBold); dl->AddText(ImVec2(p.x + 10 * S, p.y + 3 * S), U32(C_TEXT), s.name.c_str()); ImGui::PopFont();
        std::string sub; int n = 0;
        for (auto& it : s.items) { if (n++ < 5) { if (!sub.empty()) sub += ", "; sub += it.first + " x" + std::to_string(it.second); } }
        if (n > 5) sub += " ...";
        PF(fSmall); dl->AddText(ImVec2(p.x + 10 * S, p.y + 21 * S), U32(C_DIM), sub.c_str()); ImGui::PopFont();
        ImGui::PushID(i + 7000);
        float bs = 26 * S, by = p.y + (h - bs) * 0.5f;
        if (IconButton("##give", ICO_PLAY, "Daj zestaw", ImVec2(p.x + w - bs * 3 - 16 * S, by), bs, C_ON)) Act_GiveSet(s);
        if (IconButton("##edit", ICO_EDIT, "Edytuj (wczytaj do edytora)", ImVec2(p.x + w - bs * 2 - 10 * S, by), bs, C_TEXT)) {
            T.editSet = s; snprintf(setName, sizeof setName, "%s", s.name.c_str());
        }
        bool del = IconButton("##del", ICO_TRASH, "Usuń zestaw", ImVec2(p.x + w - bs - 4 * S, by), bs, C_ST_CRASH);
        ImGui::PopID();
        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + 3 * S)); ImGui::Dummy(ImVec2(w, 0));
        if (del) { T.sets.erase(T.sets.begin() + i); T.SaveSets(); break; }
    }
    // editor
    ImGui::Dummy(ImVec2(0, 4 * S));
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    float eh = 40 * S + (float)T.editSet.items.size() * 26 * S;
    ImGui::GetWindowDrawList()->AddRectFilled(p0, ImVec2(p0.x + w, p0.y + eh + 8 * S), U32(C_CARD), 8 * S);
    ImGui::SetCursorScreenPos(ImVec2(p0.x + 8 * S, p0.y + 6 * S));
    InputBox("##setname", setName, sizeof setName, "nazwa zestawu...", w * 0.45f);
    ImGui::SameLine();
    if (TextButton("##savez", "[ Zapisz zestaw ]", C_ON, fBold, ImGui::GetCursorScreenPos()) && setName[0] && !T.editSet.items.empty()) {
        T.editSet.name = setName;
        bool replaced = false;
        for (auto& s : T.sets) if (s.name == T.editSet.name) { s = T.editSet; replaced = true; }
        if (!replaced) T.sets.push_back(T.editSet);
        T.SaveSets(); T.editSet = ItemSet(); setName[0] = 0;
        T.Toast("Zapisano zestaw");
    }
    ImGui::SameLine();
    if (TextButton("##clearz", "[ Wyczyść ]", C_OFF, fReg, ImGui::GetCursorScreenPos())) T.editSet = ItemSet();
    ImGui::SetCursorScreenPos(ImVec2(p0.x + 10 * S, p0.y + 40 * S));
    if (T.editSet.items.empty()) { PF(fSmall); ImGui::TextColored(C_DIM, "Edytor zestawu jest pusty - kliknij  +  przy przedmiocie na liście."); ImGui::PopFont(); }
    for (int i = 0; i < (int)T.editSet.items.size(); i++) {
        auto& it = T.editSet.items[i];
        ImGui::PushID(i + 8000);
        ImGui::SetCursorScreenPos(ImVec2(p0.x + 10 * S, p0.y + 40 * S + i * 26 * S));
        ImGui::TextColored(C_TEXT, "%s", it.first.c_str()); ImGui::SameLine(w * 0.55f);
        NumField("##cnt", &it.second, 1, 100000, 70 * S); ImGui::SameLine();
        bool rm = TextButton("##rm", "x", C_ST_CRASH, fBold, ImGui::GetCursorScreenPos());
        ImGui::PopID();
        if (rm) { T.editSet.items.erase(T.editSet.items.begin() + i); break; }
    }
    ImGui::SetCursorScreenPos(ImVec2(p0.x, p0.y + eh + 14 * S)); ImGui::Dummy(ImVec2(w, 0));
}
void ItemsPanel(float w) {
    SetsPanel(w);
    SectionTitle("Spawner przedmiotów");
    if (!g_cat.built) { ImGui::TextColored(C_DIM, "Wczytaj grę, żeby zobaczyć listę."); return; }
    InputBox("##isearch", search, sizeof search, "Szukaj: nazwa albo ID (np. miecz, ITMW_)...", w - 140 * S);
    ImGui::SameLine(); ImGui::TextColored(C_DIM, "Ilość"); ImGui::SameLine();
    NumField("##amt", &T.itemAmount, 1, 100000, 70 * S);
    if (Chip("Wszystkie", itemCat < 0)) { itemCat = -1; itemSub.clear(); }
    for (int c = 0; c < IC_COUNT; c++) if (Chip(kItemCatNames[c], itemCat == c)) { itemCat = c; itemSub.clear(); }
    ImGui::NewLine();
    if (itemCat >= 0) {
        auto subs = SubsOf(itemCat);
        if (subs.size() > 1) {
            ImGui::TextColored(C_DIM, "›"); ImGui::SameLine();
            if (Chip("wszystkie##sub", itemSub.empty())) itemSub.clear();
            for (auto& s : subs) if (Chip((s + "##s").c_str(), itemSub == s)) itemSub = s;
            ImGui::NewLine();
        }
    }
    ImGui::TextColored(C_DIM, "Sortuj:"); ImGui::SameLine();
    const char* sorts[] = { "Nazwa", "Wartość", "Obrażenia", "Ochrona" };
    for (int i = 0; i < 4; i++) if (Chip(sorts[i], itemSort == i)) itemSort = i;
    ImGui::NewLine();
    std::vector<const ItemEntry*> v;
    for (auto& e : g_cat.items)
        if ((itemCat < 0 || e.cat == itemCat) && (itemSub.empty() || e.sub == itemSub) && (MatchQ(search, e.name, e.id) || MatchQ(search, e.info, e.sub))) v.push_back(&e);
    auto prot = [](const ItemEntry* e) { int m = 0; for (int i = 0; i < 8; i++) m = std::max(m, e->prot[i]); return m; };
    if (itemSort == 1) std::stable_sort(v.begin(), v.end(), [](auto a, auto b) { return a->value > b->value; });
    if (itemSort == 2) std::stable_sort(v.begin(), v.end(), [](auto a, auto b) { return a->damage > b->damage; });
    if (itemSort == 3) std::stable_sort(v.begin(), v.end(), [&](auto a, auto b) { return prot(a) > prot(b); });
    ImGui::TextColored(C_DIM, "%d przedmiotów", (int)v.size());
    float h = 44 * S;
    ImGuiListClipper clip; clip.Begin((int)v.size(), h + 3 * S);
    while (clip.Step())
        for (int i = clip.DisplayStart; i < clip.DisplayEnd; i++) {
            const ItemEntry& e = *v[i];
            ImVec2 p = ImGui::GetCursorScreenPos();
            ListRowBg(p, w, h, i);
            ImDrawList* dl = ImGui::GetWindowDrawList();
            float x = p.x + 10 * S;
            if (T.Crashed("item:" + e.id)) { float pw; StatusPill(dl, ImVec2(x, p.y + 5 * S), ST_CRASH, "crashowało grę w poprzedniej sesji", &pw); x += pw + 6 * S; }
            PF(fBold); dl->AddText(ImVec2(x, p.y + 3 * S), U32(C_TEXT), e.name.c_str()); ImGui::PopFont();
            char sub[300];
            snprintf(sub, sizeof sub, "%s%s%s  ·  %d rudy  ·  %s", e.sub.empty() ? kItemCatNames[e.cat] : e.sub.c_str(),
                     e.info.empty() ? "" : "  ·  ", e.info.c_str(), e.value, e.id.c_str());
            dl->PushClipRect(p, ImVec2(p.x + w * 0.56f, p.y + h), true);
            PF(fSmall); dl->AddText(ImVec2(p.x + 10 * S, p.y + 23 * S), U32(C_DIM), sub); ImGui::PopFont();
            dl->PopClipRect();
            if (ImGui::IsMouseHoveringRect(p, ImVec2(p.x + w * 0.56f, p.y + h)) && ImGui::IsWindowHovered()) {
                std::string tip = e.name + "\n" + sub;
                for (int k = 0; k < 6; k++) if (!e.text[k].empty()) tip += "\n" + e.text[k] + (e.count[k] ? " " + std::to_string(e.count[k]) : "");
                ImGui::SetTooltip("%s", tip.c_str());
            }
            ImGui::PushID(e.sym);
            float by = p.y + h * 0.5f - 12 * S;
            if (TextButton("##inv", "[ Do ekwipunku ]", C_ON, fReg, ImVec2(p.x + w * 0.57f, by))) Act_GiveItem(e.sym, T.itemAmount);
            if (TextButton("##drop", "[ Pod nogi ]", C_OFF, fReg, ImVec2(p.x + w * 0.735f, by))) Act_DropItem(e.sym, T.itemAmount);
            if (TextButton("##tgt", "[ Celowi ]", C_OFF, fReg, ImVec2(p.x + w * 0.855f, by))) Act_GiveItemTarget(e.sym, T.itemAmount);
            if (IconButton("##addset", ICO_PLUS, "Dodaj do edytowanego zestawu", ImVec2(p.x + w - 30 * S, p.y + (h - 24 * S) * 0.5f), 24 * S, C_ACC)) {
                bool found = false;
                for (auto& it : T.editSet.items) if (it.first == e.id) { it.second += T.itemAmount; found = true; }
                if (!found) T.editSet.items.push_back({ e.id, T.itemAmount });
                T.Toast("Do zestawu: " + e.name);
            }
            ImGui::PopID();
            ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + 3 * S)); ImGui::Dummy(ImVec2(w, 0));
        }
}

// ---------------------------------------------------------------------------- npcs
void NpcPanel(float w) {
    SectionTitle("Spawner NPC i potworów");
    if (!g_cat.built) { ImGui::TextColored(C_DIM, "Wczytaj grę, żeby zobaczyć listę."); return; }
    InputBox("##nsearch", search, sizeof search, "Szukaj: Gomez, troll, ORCWARRIOR...", w - 150 * S);
    ImGui::SameLine(); ImGui::TextColored(C_DIM, "Ile"); ImGui::SameLine();
    NumField("##cnt", &T.spawnCount, 1, 30, 70 * S);
    ImGui::TextColored(C_DIM, "Tryb:"); ImGui::SameLine();
    const char* modes[] = { "Wrogi (od razu atakuje)", "Sojusznik", "Zamrożony", "Neutralny" };
    for (int m = 0; m < 4; m++) if (Chip(modes[m], T.spawnMode == m)) T.spawnMode = m;
    ImGui::NewLine();
    if (Chip("Wszyscy##g", npcGroup < 0)) npcGroup = -1;
    for (int g = 0; g < NG_COUNT; g++) if (Chip(kNpcGroupNames[g], npcGroup == g)) npcGroup = g;
    ImGui::NewLine();
    std::vector<const NpcEntry*> v;
    for (auto& e : g_cat.npcs) if ((npcGroup < 0 || e.group == npcGroup) && MatchQ(search, e.name, e.id)) v.push_back(&e);
    ImGui::TextColored(C_DIM, "%d instancji", (int)v.size());
    float h = 40 * S;
    ImGuiListClipper clip; clip.Begin((int)v.size(), h + 3 * S);
    while (clip.Step())
        for (int i = clip.DisplayStart; i < clip.DisplayEnd; i++) {
            const NpcEntry& e = *v[i];
            ImVec2 p = ImGui::GetCursorScreenPos();
            ListRowBg(p, w, h, i);
            ImDrawList* dl = ImGui::GetWindowDrawList();
            float x = p.x + 10 * S;
            if (T.Crashed("npc:" + e.id)) { float pw; StatusPill(dl, ImVec2(x, p.y + 5 * S), ST_CRASH, "spawn tej instancji crashował grę", &pw); x += pw + 6 * S; }
            PF(fBold); dl->AddText(ImVec2(x, p.y + 3 * S), U32(C_TEXT), e.name.c_str()); ImGui::PopFont();
            char sub[220]; snprintf(sub, sizeof sub, "%s  ·  %s  ·  poziom %d  ·  %s", kNpcGroupNames[e.group], GuildName(e.guild).c_str(), e.level, e.id.c_str());
            PF(fSmall); dl->AddText(ImVec2(p.x + 10 * S, p.y + 22 * S), U32(C_DIM), sub); ImGui::PopFont();
            ImGui::PushID(e.sym);
            if (TextButton("##sp", "[ Spawn ]", C_ON, fBold, ImVec2(p.x + w * 0.84f, p.y + h * 0.5f - 12 * S))) Act_SpawnNpc(e.sym, T.spawnCount, T.spawnMode);
            ImGui::PopID();
            ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + 3 * S)); ImGui::Dummy(ImVec2(w, 0));
        }
}

// ---------------------------------------------------------------------------- target card
void Avatar(ImDrawList* dl, ImVec2 c, float r, bool monster, const std::string& name) {
    dl->AddCircleFilled(c, r, U32(Mix(C_CARD, C_ACC, 0.35f)), 40);
    dl->AddCircle(c, r, U32(C_ACC), 40, 2 * S);
    if (iconsOk) {
        PF(fIcon);
        const char* g = monster ? ICO_PAW : ICO_USER;
        float sc = r / 14.f;
        ImVec2 ts = ImGui::CalcTextSize(g);
        dl->AddText(fIcon, fIcon->LegacySize * sc, ImVec2(c.x - ts.x * sc * 0.5f, c.y - ts.y * sc * 0.5f), U32(C_TEXT), g);
        ImGui::PopFont();
    } else {
        std::string ini = name.substr(0, 1);
        PF(fBig); ImVec2 ts = ImGui::CalcTextSize(ini.c_str()); dl->AddText(ImVec2(c.x - ts.x / 2, c.y - ts.y / 2), U32(C_TEXT), ini.c_str()); ImGui::PopFont();
    }
}
void TargetCard(const Snapshot& s, float w) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    float h = 132 * S;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 left = U32(Mix(C_CARD, C_ACC, 0.22f)), right = U32(C_CARD);
    dl->AddRectFilledMultiColor(ImVec2(p.x + 6 * S, p.y), ImVec2(p.x + w, p.y + h), left, right, right, left);
    dl->AddRectFilled(p, ImVec2(p.x + 12 * S, p.y + h), left, 8 * S, ImDrawFlags_RoundTopLeft | ImDrawFlags_RoundBottomLeft);
    dl->AddRect(p, ImVec2(p.x + w, p.y + h), U32(C_ACC, 0.25f), 8 * S);
    float ax = p.x + 52 * S, ay = p.y + 50 * S;
    Avatar(dl, ImVec2(ax, ay), 34 * S, s.tMonster, s.tName);
    float ix = p.x + 100 * S;
    if (!s.tValid) {
        PF(fBold); dl->AddText(ImVec2(ix, p.y + 18 * S), U32(C_TEXT), "Brak celu"); ImGui::PopFont();
        PF(fSmall); dl->AddText(ImVec2(ix, p.y + 42 * S), U32(C_DIM), "Spójrz na NPC (pojawi się jego imię) albo zapisz cel przyciskiem z zakładką."); ImGui::PopFont();
    } else {
        PF(fBig); dl->AddText(ImVec2(ix, p.y + 12 * S), U32(C_TEXT), s.tName.c_str());
        float nx = ix + ImGui::CalcTextSize(s.tName.c_str()).x + 8 * S; ImGui::PopFont();
        float pw; Pill(dl, ImVec2(nx, p.y + 17 * S), s.tSaved ? "ZAPISANY" : "POD WZROKIEM", s.tSaved ? C_ON : C_DIM, &pw);
        if (s.tDead) Pill(dl, ImVec2(nx + pw + 6 * S, p.y + 17 * S), "MARTWY", C_ST_CRASH);
        char b[200]; snprintf(b, sizeof b, "%s  ·  %s  ·  poziom %d", s.tGroup.c_str(), s.tGuild.c_str(), s.tLevel);
        PF(fSmall); dl->AddText(ImVec2(ix, p.y + 40 * S), U32(C_DIM), b);
        float bx0 = ix, bx1 = ix + (w - (ix - p.x)) * 0.42f, by = p.y + 62 * S;
        float t = s.tHpMax > 0 ? std::max(0.f, std::min(1.f, (float)s.tHp / s.tHpMax)) : 0;
        dl->AddRectFilled(ImVec2(bx0, by), ImVec2(bx1, by + 6 * S), U32(C_TRACK), 3 * S);
        dl->AddRectFilled(ImVec2(bx0, by), ImVec2(bx0 + (bx1 - bx0) * t, by + 6 * S), U32(C_ON), 3 * S);
        snprintf(b, sizeof b, "%d / %d HP", s.tHp, s.tHpMax);
        dl->AddText(ImVec2(bx1 + 8 * S, by - 4 * S), U32(C_NUM), b);
        ImGui::PopFont();
    }
    // action icons (right side)
    struct A { const char* id; } acts[] = { { "swap" }, { "mind" }, { "tpull" }, { "tpto" }, { "yeet" }, { "tkill" },
                                            { "trevive" }, { "trob" }, { "tstorm" }, { "tlight" }, { "tice" }, { "tremove" } };
    float bs = 30 * S, gap = 5 * S;
    int perRow = 6;
    float gx = p.x + w - perRow * (bs + gap) - 8 * S, gy = p.y + 10 * S;
    int i = 0;
    for (auto& a : acts) {
        Row* r = T.Get(a.id);
        if (!r) continue;
        ImGui::PushID(a.id);
        ImVec2 bp(gx + (i % perRow) * (bs + gap), gy + (i / perRow) * (bs + gap));
        std::string tip = r->name + (r->note.empty() ? "" : "\n" + r->note);
        if (IconButton("##ta", r->icon.c_str(), tip.c_str(), bp, bs, a.id == std::string("swap") ? C_ACC : C_TEXT)) ApplyRow(*r);
        ImGui::PopID();
        i++;
    }
    // saved targets chips
    ImGui::SetCursorScreenPos(ImVec2(p.x + 14 * S, p.y + h - 34 * S));
    PF(fSmall); ImGui::TextColored(C_DIM, "Zapisane:"); ImGui::PopFont(); ImGui::SameLine();
    if (TextButton("##tsave", ICO_BOOKMARK " zapisz", C_ACC, iconsOk ? fReg : fReg, ImGui::GetCursorScreenPos())) { if (Row* r = T.Get("tsave")) ApplyRow(*r); }
    ImGui::SameLine();
    if (Chip("pod wzrokiem##tw", s.activeSaved < 0)) { if (Row* r = T.Get("tforget")) ApplyRow(*r); }
    for (int k = 0; k < (int)s.savedTargets.size(); k++) {
        ImGui::PushID(k + 6000);
        std::string lbl = s.savedTargets[k] + "##st";
        if (Chip(lbl.c_str(), s.activeSaved == k)) Act_SelectSavedTarget(k);
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) Act_RemoveSavedTarget(k);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("klik = wybierz, prawy klik = usuń z listy");
        ImGui::PopID();
    }
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + 8 * S)); ImGui::Dummy(ImVec2(w, 0));
}

// ---------------------------------------------------------------------------- teleport
void TeleportPanel(const Snapshot& s, float w) {
    SectionTitle("Twoje pozycje");
    PF(fSmall); ImGui::TextColored(C_DIM, "Świat: %s   ·   teraz: X %.0f  Y %.0f  Z %.0f", s.world.c_str(), s.px, s.py, s.pz); ImGui::PopFont();
    InputBox("##posname", posName, sizeof posName, "nazwa nowej pozycji (np. Kopalnia, Xardas)...", w - 190 * S);
    ImGui::SameLine();
    if (TextButton("##savepos", "[ Zapisz obecną ]", C_ON, fBold, ImGui::GetCursorScreenPos())) { Act_SavePos(posName); posName[0] = 0; }
    ImGui::NewLine();
    std::vector<SavedPos> pos;
    { std::lock_guard<std::mutex> l(T.mtx); pos = T.positions; }
    if (pos.empty()) ImGui::TextColored(C_DIM, "Brak zapisanych pozycji w tym świecie.");
    float h = 36 * S;
    for (int i = 0; i < (int)pos.size(); i++) {
        ImVec2 p = ImGui::GetCursorScreenPos();
        ListRowBg(p, w, h, i);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        PF(fBold); dl->AddText(ImVec2(p.x + 10 * S, p.y + 3 * S), U32(C_TEXT), pos[i].name.c_str()); ImGui::PopFont();
        char c[96]; snprintf(c, sizeof c, "X %.0f  Y %.0f  Z %.0f", pos[i].x, pos[i].y, pos[i].z);
        PF(fSmall); dl->AddText(ImVec2(p.x + 10 * S, p.y + 20 * S), U32(C_DIM), c); ImGui::PopFont();
        ImGui::PushID(i + 9000);
        if (TextButton("##go", "[ Teleport ]", C_ON, fBold, ImVec2(p.x + w * 0.66f, p.y + h * 0.5f - 12 * S))) Act_Teleport(pos[i].x, pos[i].y, pos[i].z, pos[i].name);
        if (TextButton("##del", "[ Usuń ]", C_ST_CRASH, fReg, ImVec2(p.x + w * 0.84f, p.y + h * 0.5f - 12 * S))) Act_DeletePos(i);
        ImGui::PopID();
        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + 3 * S)); ImGui::Dummy(ImVec2(w, 0));
    }
    SectionTitle("Ręczne koordynaty");
    ImGui::PushStyleColor(ImGuiCol_FrameBg, C_ROW1); ImGui::PushStyleColor(ImGuiCol_Text, C_NUM);
    float fw = (w - 260 * S) / 3;
    ImGui::SetNextItemWidth(fw); ImGui::InputFloat("X##m", &manX, 0, 0, "%.0f"); ImGui::SameLine();
    ImGui::SetNextItemWidth(fw); ImGui::InputFloat("Y##m", &manY, 0, 0, "%.0f"); ImGui::SameLine();
    ImGui::SetNextItemWidth(fw); ImGui::InputFloat("Z##m", &manZ, 0, 0, "%.0f"); ImGui::SameLine();
    ImGui::PopStyleColor(2);
    if (TextButton("##cur", "[ Obecna ]", C_OFF, fReg, ImGui::GetCursorScreenPos())) { manX = s.px; manY = s.py; manZ = s.pz; }
    ImGui::SameLine();
    if (TextButton("##mgo", "[ Teleport ]", C_ON, fBold, ImGui::GetCursorScreenPos())) Act_Teleport(manX, manY, manZ, "ręczne koordynaty");
    ImGui::NewLine();
    SectionTitle("Waypointy świata");
    if (g_cat.wps.empty()) { ImGui::TextColored(C_DIM, "Brak waypointów (wczytaj grę)."); return; }
    InputBox("##wsearch", search, sizeof search, "Szukaj waypointa: OC, PSI, OW_, NC_...", w);
    std::vector<const WpEntry*> v;
    for (auto& e : g_cat.wps) if (MatchQ(search, e.name, e.name)) v.push_back(&e);
    ImGui::TextColored(C_DIM, "%d waypointów", (int)v.size());
    float wh = 32 * S;
    ImGuiListClipper clip; clip.Begin((int)v.size(), wh + 3 * S);
    while (clip.Step())
        for (int i = clip.DisplayStart; i < clip.DisplayEnd; i++) {
            const WpEntry& e = *v[i];
            ImVec2 p = ImGui::GetCursorScreenPos();
            ListRowBg(p, w, wh, i);
            PF(fBold); ImGui::GetWindowDrawList()->AddText(ImVec2(p.x + 10 * S, p.y + 7 * S), U32(C_TEXT), e.name.c_str()); ImGui::PopFont();
            ImGui::PushID(i);
            if (TextButton("##tp", "[ Teleport ]", C_ON, fReg, ImVec2(p.x + w * 0.84f, p.y + wh * 0.5f - 12 * S))) Act_Teleport(e.x, e.y, e.z, e.name);
            ImGui::PopID();
            ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + wh + 3 * S)); ImGui::Dummy(ImVec2(w, 0));
        }
}

// ---------------------------------------------------------------------------- marvin
void MarvinPanel(const Snapshot& s, float w) {
    SectionTitle("Konsola i makra");
    PF(fSmall); ImGui::TextColored(C_DIM, "Jedna komenda w linii, np.: insert ITMW_2H_SWORD_HEAVY_04 / goto waypoint OCC_BARONS_GREATHALL_CENTER / cheat full"); ImGui::PopFont();
    ImGui::PushStyleColor(ImGuiCol_FrameBg, C_ROW1);
    ImGui::InputTextMultiline("##con", consoleBuf, sizeof consoleBuf, ImVec2(w, 90 * S));
    ImGui::PopStyleColor();
    ImGui::TextColored(C_DIM, "Powtórz x"); ImGui::SameLine(); NumField("##rep", &macroRepeat, 1, 500, 60 * S); ImGui::SameLine();
    if (TextButton("##run", "[ Wykonaj ]", C_ON, fBold, ImGui::GetCursorScreenPos()) && consoleBuf[0]) Act_RunMacro({ "", consoleBuf, macroRepeat });
    ImGui::SameLine();
    InputBox("##mname", macroName, sizeof macroName, "nazwa makra...", w * 0.25f); ImGui::SameLine();
    if (TextButton("##msave", "[ Zapisz makro ]", C_ACC, fReg, ImGui::GetCursorScreenPos()) && macroName[0] && consoleBuf[0]) {
        bool rep = false;
        for (auto& m : T.macros) if (m.name == macroName) { m.cmds = consoleBuf; m.repeat = macroRepeat; rep = true; }
        if (!rep) T.macros.push_back({ macroName, consoleBuf, macroRepeat });
        T.SaveMacros(); macroName[0] = 0; T.Toast("Zapisano makro");
    }
    ImGui::NewLine();
    float h = 34 * S;
    for (int i = 0; i < (int)T.macros.size(); i++) {
        Macro& m = T.macros[i];
        ImVec2 p = ImGui::GetCursorScreenPos();
        ListRowBg(p, w, h, i);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        PF(fBold); dl->AddText(ImVec2(p.x + 10 * S, p.y + 3 * S), U32(C_TEXT), m.name.c_str()); ImGui::PopFont();
        std::string first = m.cmds.substr(0, m.cmds.find('\n'));
        char sub[200]; snprintf(sub, sizeof sub, "x%d  ·  %s%s", m.repeat, first.c_str(), m.cmds.find('\n') != std::string::npos ? " ..." : "");
        PF(fSmall); dl->AddText(ImVec2(p.x + 10 * S, p.y + 19 * S), U32(C_DIM), sub); ImGui::PopFont();
        ImGui::PushID(i + 5000);
        float bs = 24 * S, by = p.y + (h - bs) * 0.5f;
        if (IconButton("##run", ICO_PLAY, "Uruchom", ImVec2(p.x + w - bs * 3 - 16 * S, by), bs, C_ON)) Act_RunMacro(m);
        if (IconButton("##edit", ICO_EDIT, "Wczytaj do edytora", ImVec2(p.x + w - bs * 2 - 10 * S, by), bs, C_TEXT)) {
            snprintf(consoleBuf, sizeof consoleBuf, "%s", m.cmds.c_str()); snprintf(macroName, sizeof macroName, "%s", m.name.c_str()); macroRepeat = m.repeat;
        }
        bool del = IconButton("##del", ICO_TRASH, "Usuń", ImVec2(p.x + w - bs - 4 * S, by), bs, C_ST_CRASH);
        ImGui::PopID();
        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + 3 * S)); ImGui::Dummy(ImVec2(w, 0));
        if (del) { T.macros.erase(T.macros.begin() + i); T.SaveMacros(); break; }
    }
    if (!s.marvinOut.empty()) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, C_ROW2);
        ImGui::BeginChild("##out", ImVec2(w, 110 * S), false);
        PF(fSmall); ImGui::TextWrapped("%s", s.marvinOut.c_str()); ImGui::PopFont();
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
    SectionTitle("Wywołaj funkcję skryptu (bez parametrów)");
    InputBox("##scr", scriptBuf, sizeof scriptBuf, "nazwa funkcji Daedalusa...", w - 120 * S);
    ImGui::SameLine();
    if (TextButton("##run2", "[ Wywołaj ]", C_ON, fBold, ImGui::GetCursorScreenPos()) && scriptBuf[0]) Act_RunScript(scriptBuf);
    ImGui::NewLine();
    SectionTitle("Diagnostyka rdzenia (autotest przy wczytaniu świata)");
    if (s.diag.empty()) ImGui::TextColored(C_DIM, "Wczytaj grę - autotest uruchomi się sam.");
    for (auto& d : s.diag) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos(); float pw;
        StatusPill(dl, p, d.ok ? ST_OK : ST_CRASH, "", &pw);
        ImGui::SetCursorScreenPos(ImVec2(p.x + pw + 8 * S, p.y));
        PF(fSmall); ImGui::TextColored(d.ok ? C_TEXT : C_ST_CRASH, "%s", d.text.c_str()); ImGui::PopFont();
    }
}

// ---------------------------------------------------------------------------- render (ESP) cards
bool Switch(const char* id, bool* v, ImVec2 pos) {
    float wdt = 34 * S, hgt = 18 * S;
    ImGui::SetCursorScreenPos(pos);
    bool c = ImGui::InvisibleButton(id, ImVec2(wdt, hgt));
    if (c) *v = !*v;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pos, ImVec2(pos.x + wdt, pos.y + hgt), U32(*v ? C_ACC : C_TRACK), hgt * 0.5f);
    float kx = *v ? pos.x + wdt - hgt * 0.5f : pos.x + hgt * 0.5f;
    dl->AddCircleFilled(ImVec2(kx, pos.y + hgt * 0.5f), hgt * 0.38f, U32(C_TEXT), 16);
    return c;
}
float CardRow(Row* r, float x, float y, float w) {
    if (!r) return 0;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float h = 34 * S;
    ImGui::PushID(r->id.c_str());
    PF(fReg); dl->AddText(ImVec2(x + 12 * S, y + 8 * S), U32(C_TEXT), r->name.c_str());
    float tw = ImGui::CalcTextSize(r->name.c_str()).x; ImGui::PopFont();
    float pw; StatusPill(dl, ImVec2(x + 18 * S + tw, y + 10 * S), r->status, r->hist, &pw);
    if (r->kind == K_TOGGLE) {
        if (Switch("##sw", &r->on, ImVec2(x + w - 46 * S, y + 8 * S))) { T.Save(); }
    } else if (!r->options.empty()) {
        Combo(*r, ImVec2(x + w - w * 0.42f - 10 * S, y + 4 * S), w * 0.42f);
    } else {
        char lb[32]; snprintf(lb, sizeof lb, "%.0f %s", r->val, r->note.c_str());
        PF(fSmall); ImVec2 ts = ImGui::CalcTextSize(lb);
        dl->AddText(ImVec2(x + w - ts.x - 12 * S, y + 2 * S), U32(C_DIM), lb); ImGui::PopFont();
        Slider(*r, x + w * 0.45f, x + w - 14 * S, y + 24 * S);
    }
    ImGui::PopID();
    dl->AddLine(ImVec2(x + 10 * S, y + h), ImVec2(x + w - 10 * S, y + h), U32(C_TRACK, 0.5f));
    return h;
}
float Card(const char* title, std::vector<const char*> ids, float x, float y, float w) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    PF(fSmall); dl->AddText(ImVec2(x + 2 * S, y), U32(C_DIM), title); ImGui::PopFont();
    float top = y + 18 * S;
    float h = 6 * S;
    for (auto id : ids) { Row* r = T.Get(id); if (r) h += 34 * S; }
    dl->AddRectFilled(ImVec2(x, top), ImVec2(x + w, top + h + 4 * S), U32(C_CARD), 8 * S);
    float yy = top + 4 * S;
    for (auto id : ids) yy += CardRow(T.Get(id), x, yy, w);
    return top + h + 18 * S - y;
}
void ColorCard(float x, float y, float w, float* outH) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const char* names[7] = { "Wrogowie", "Sojusznicy", "Neutralni", "Potwory", "Martwi", "Przedmioty", "Skrzynie" };
    PF(fSmall); dl->AddText(ImVec2(x + 2 * S, y), U32(C_DIM), "Kolory ESP"); ImGui::PopFont();
    float top = y + 18 * S, h = 7 * 30 * S + 8 * S;
    dl->AddRectFilled(ImVec2(x, top), ImVec2(x + w, top + h), U32(C_CARD), 8 * S);
    for (int i = 0; i < 7; i++) {
        float yy = top + 6 * S + i * 30 * S;
        PF(fReg); dl->AddText(ImVec2(x + 12 * S, yy + 4 * S), U32(C_TEXT), names[i]); ImGui::PopFont();
        ImVec4 col = ImGui::ColorConvertU32ToFloat4(g_espColors[i]);
        ImGui::SetCursorScreenPos(ImVec2(x + w - 34 * S, yy + 2 * S));
        ImGui::PushID(i + 4000);
        if (ImGui::ColorEdit3("##c", &col.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
            g_espColors[i] = ImGui::ColorConvertFloat4ToU32(ImVec4(col.x, col.y, col.z, 1));
            char k[16], v[16]; snprintf(k, sizeof k, "c%d", i); snprintf(v, sizeof v, "%u", g_espColors[i]);
            WritePrivateProfileStringA("espcolors", k, v, IniPath().c_str());
        }
        ImGui::PopID();
        char hex[12]; snprintf(hex, sizeof hex, "#%02X%02X%02X", (int)(col.x * 255), (int)(col.y * 255), (int)(col.z * 255));
        PF(fSmall); ImVec2 ts = ImGui::CalcTextSize(hex); dl->AddText(ImVec2(x + w - 44 * S - ts.x, yy + 6 * S), U32(C_DIM), hex); ImGui::PopFont();
    }
    *outH = h + 26 * S;
}
void RenderPanel(float w) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    float colW = (w - 12 * S) * 0.5f;
    float lx = p.x, rx = p.x + colW + 12 * S;
    float ly = p.y, ry = p.y;
    ly += Card("ESP", { "espnpc", "espitems", "espchest", "espchestinv", "espeq", "esprange" }, lx, ly, colW);
    ly += Card("Filtry", { "espfilter", "espdead", "espitemval" }, lx, ly, colW);
    ry += Card("Wygląd", { "espname", "espdist", "espguild", "esphp", "esphpnum", "espbox", "espboxmode", "espline" }, rx, ry, colW);
    float ch; ColorCard(rx, ry, colW, &ch); ry += ch;
    ImGui::SetCursorScreenPos(ImVec2(p.x, std::max(ly, ry)));
    ImGui::Dummy(ImVec2(w, 4 * S));
}

void DrawRowsGrouped(const std::string& cat, float w) {
    std::string lastSub = "\x01";
    int idx = 0;
    for (auto& r : T.rows) {
        if (r.cat != cat || r.hidden) continue;
        if (r.sub != lastSub) { SubHeader(r.sub); lastSub = r.sub; }
        DrawRow(r, idx++, w);
    }
}

// ---------------------------------------------------------------------------- overlays
void DrawEsp(const Snapshot& s, float W, float H) {
    ImDrawList* bg = ImGui::GetBackgroundDrawList();
    PF(fSmall);
    for (auto& L : s.labels) {
        ImVec2 a(L.x * W, L.y * H);
        ImU32 col = L.color;
        if (L.box == 1) bg->AddRect(ImVec2(L.bx0 * W, L.by0 * H), ImVec2(L.bx1 * W, L.by1 * H), col, 3 * S, 1.5f * S);
        if (L.box == 2) {
            static const int E[12][2] = { {0,1},{0,2},{0,4},{1,3},{1,5},{2,3},{2,6},{3,7},{4,5},{4,6},{5,7},{6,7} };
            for (auto& e : E) bg->AddLine(ImVec2(L.corners[e[0] * 2] * W, L.corners[e[0] * 2 + 1] * H), ImVec2(L.corners[e[1] * 2] * W, L.corners[e[1] * 2 + 1] * H), col, 1.3f * S);
        }
        if (s.espLines && L.type == 0) bg->AddLine(ImVec2(W * 0.5f, H), ImVec2(L.box ? (L.bx0 + L.bx1) * 0.5f * W : a.x, L.box ? L.by1 * H : a.y), (col & 0x00FFFFFF) | 0x90000000, 1.2f * S);
        if (L.text.empty() && L.sub.empty() && L.hp < 0 && L.lines.empty()) continue;
        ImVec2 ts = L.text.empty() ? ImVec2(0, 0) : ImGui::CalcTextSize(L.text.c_str());
        ImVec2 ss = L.sub.empty() ? ImVec2(0, 0) : ImGui::CalcTextSize(L.sub.c_str());
        float bw = std::max(ts.x, ss.x);
        for (auto& l : L.lines) bw = std::max(bw, ImGui::CalcTextSize(l.c_str()).x);
        bw += 12 * S;
        float lh = ImGui::GetTextLineHeight();
        float bh = ts.y + ss.y + (L.hp >= 0 ? 6 * S : 0) + L.lines.size() * lh + 6 * S;
        float topY = (L.box ? L.by0 * H : a.y) - bh - 4 * S;
        ImVec2 p0(a.x - bw * 0.5f, topY);
        bg->AddRectFilled(p0, ImVec2(p0.x + bw, p0.y + bh), IM_COL32(15, 14, 24, 185), 5 * S);
        float y = p0.y + 3 * S;
        if (!L.text.empty()) { bg->AddText(ImVec2(a.x - ts.x * 0.5f, y), col, L.text.c_str()); y += ts.y; }
        if (!L.sub.empty()) { bg->AddText(ImVec2(a.x - ss.x * 0.5f, y), IM_COL32(170, 170, 190, 255), L.sub.c_str()); y += ss.y; }
        if (L.hp >= 0) {
            float x0 = p0.x + 6 * S, x1 = p0.x + bw - 6 * S;
            bg->AddRectFilled(ImVec2(x0, y + 1 * S), ImVec2(x1, y + 4 * S), IM_COL32(60, 60, 80, 255), 2 * S);
            bg->AddRectFilled(ImVec2(x0, y + 1 * S), ImVec2(x0 + (x1 - x0) * L.hp, y + 4 * S), U32(C_ON), 2 * S);
            y += 6 * S;
        }
        for (auto& l : L.lines) { bg->AddText(ImVec2(p0.x + 6 * S, y), IM_COL32(210, 210, 225, 255), l.c_str()); y += lh; }
    }
    ImGui::PopFont();
}

void PlayerCard(const Snapshot& s, ImVec2 p, float w) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float h = 58 * S;
    ImU32 left = U32(Mix(C_CARD, C_ACC, 0.18f)), right = U32(C_CARD);
    dl->AddRectFilledMultiColor(p, ImVec2(p.x + w, p.y + h), left, right, right, left);
    dl->AddRect(p, ImVec2(p.x + w, p.y + h), U32(C_ACC, 0.2f), 8 * S);
    Avatar(dl, ImVec2(p.x + 30 * S, p.y + h * 0.5f), 20 * S, false, s.bodyName);
    float x = p.x + 60 * S;
    if (!s.inGame) {
        PF(fBold); dl->AddText(ImVec2(x, p.y + 10 * S), U32(C_ACC), "GOTHIC CONNECTED"); ImGui::PopFont();
        PF(fSmall); dl->AddText(ImVec2(x, p.y + 32 * S), U32(C_DIM), "w menu gry - wczytaj albo zacznij grę"); ImGui::PopFont();
        return;
    }
    PF(fBold);
    std::string nm = s.bodyName.empty() ? "Bezimienny" : s.bodyName;
    dl->AddText(ImVec2(x, p.y + 6 * S), U32(C_TEXT), nm.c_str());
    float nx = x + ImGui::CalcTextSize(nm.c_str()).x + 8 * S;
    ImGui::PopFont();
    if (s.swapped) Pill(dl, ImVec2(nx, p.y + 9 * S), "OBCE CIAŁO", C_ST_BUG);
    char b[220];
    snprintf(b, sizeof b, "%s  ·  poziom %d  ·  PD %d/%d  ·  PN %d  ·  Ruda %d  ·  Dzień %d, %02d:%02d", s.guildName.c_str(), s.level, s.exp,
             s.expNext, s.lp, s.ore, s.day, s.hour, s.minute);
    PF(fSmall); dl->AddText(ImVec2(x, p.y + 26 * S), U32(C_DIM), b); ImGui::PopFont();
    float bw = (w - (x - p.x) - 20 * S) * 0.5f - 6 * S, by = p.y + 44 * S;
    auto bar = [&](float bx, int v, int m, const ImVec4& c, const char* t) {
        float f = m > 0 ? std::max(0.f, std::min(1.f, (float)v / m)) : 0;
        dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx + bw, by + 6 * S), U32(C_TRACK), 3 * S);
        dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx + bw * f, by + 6 * S), U32(c), 3 * S);
        char l[48]; snprintf(l, sizeof l, "%s %d/%d", t, v, m);
        PF(fSmall); ImVec2 ts = ImGui::CalcTextSize(l); dl->AddText(ImVec2(bx + bw - ts.x, by - ts.y - 1 * S), U32(C_TEXT, 0.8f), l); ImGui::PopFont();
    };
    bar(x, s.hp, s.hpmax, ImVec4(0.92f, 0.36f, 0.40f, 1), "HP");
    bar(x + bw + 12 * S, s.mana, s.manamax, ImVec4(0.40f, 0.62f, 1.0f, 1), "Mana");
}

void SettingsPopup(ImVec2 anchor) {
    ImGui::SetNextWindowPos(anchor, ImGuiCond_Always, ImVec2(1, 0));
    ImGui::SetNextWindowSize(ImVec2(320 * S, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.09f, 0.09f, 0.13f, 0.99f));
    ImGui::Begin("##settings", &settingsOpen, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
    PF(fBold); ImGui::TextColored(C_ACC, "Ustawienia menu"); ImGui::PopFont();
    ImGui::TextColored(C_DIM, "Kolor akcentu");
    const float presets[][3] = { { 0.66f, 0.55f, 1.f }, { 0.36f, 0.66f, 1.f }, { 0.35f, 0.85f, 0.60f }, { 1.f, 0.72f, 0.30f }, { 1.f, 0.42f, 0.45f }, { 0.95f, 0.45f, 0.85f } };
    for (int i = 0; i < 6; i++) {
        ImGui::PushID(i);
        ImVec2 p = ImGui::GetCursorScreenPos();
        if (ImGui::InvisibleButton("##pre", ImVec2(26 * S, 26 * S))) { memcpy(T.accent, presets[i], sizeof(float) * 3); T.Save(); }
        ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(p.x + 13 * S, p.y + 13 * S), 11 * S, ImGui::GetColorU32(ImVec4(presets[i][0], presets[i][1], presets[i][2], 1)), 20);
        ImGui::PopID();
        ImGui::SameLine();
    }
    ImGui::NewLine();
    if (ImGui::ColorEdit3("Własny", T.accent, ImGuiColorEditFlags_NoInputs)) T.Save();
    ImGui::Dummy(ImVec2(0, 4 * S));
    if (Row* r = T.Get("pausemenu")) { bool v = r->on; if (Switch("##pause", &v, ImGui::GetCursorScreenPos())) { r->on = v; T.Save(); } ImGui::SameLine(46 * S); ImGui::Text("Pauza gry przy otwartym menu"); }
    ImGui::Dummy(ImVec2(0, 4 * S));
    ImGui::TextColored(C_DIM, "Esc zamyka menu, Insert otwiera/zamyka.");
    ImGui::End();
    ImGui::PopStyleColor();
}
} // namespace

// ------------------------------------------------------------------------------
void UiInitFonts(float screenH) {
    S = std::max(0.75f, screenH / 1080.f);
    ImGuiIO& io = ImGui::GetIO();
    static const ImWchar ranges[] = { 0x0020, 0x00FF, 0x0100, 0x017F, 0x2010, 0x203A, 0 };
    static const ImWchar iconRanges[] = { 0xF000, 0xF2E0, 0 };
    char win[MAX_PATH]; GetWindowsDirectoryA(win, MAX_PATH);
    std::string reg = std::string(win) + "\\Fonts\\segoeui.ttf", bold = std::string(win) + "\\Fonts\\segoeuib.ttf";
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
    fReg = add(reg, 15); fBold = add(bold, 15); fBig = add(bold, 19); fSmall = add(reg, 12.5f);
    fIcon = iconsOk ? io.Fonts->AddFontFromFileTTF(icons.c_str(), 16 * S, nullptr, iconRanges) : fReg;
    io.FontDefault = fReg;
    for (int i = 0; i < 7; i++) {
        char k[16]; snprintf(k, sizeof k, "c%d", i);
        unsigned v = (unsigned)GetPrivateProfileIntA("espcolors", k, 0, IniPath().c_str());
        if (v) g_espColors[i] = v;
    }

    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowRounding = 10 * S; st.ChildRounding = 8 * S; st.FrameRounding = 6 * S; st.PopupRounding = 6 * S;
    st.ScrollbarSize = 6 * S; st.ScrollbarRounding = 3 * S; st.GrabRounding = 6 * S;
    st.WindowBorderSize = 0; st.FrameBorderSize = 0; st.PopupBorderSize = 0; st.WindowPadding = ImVec2(14 * S, 12 * S);
    st.FramePadding = ImVec2(8 * S, 5 * S); st.ItemSpacing = ImVec2(6 * S, 6 * S);
    ImVec4* c = st.Colors;
    c[ImGuiCol_WindowBg] = C_BG; c[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0); c[ImGuiCol_Text] = C_TEXT;
    c[ImGuiCol_TextDisabled] = C_DIM; c[ImGuiCol_FrameBg] = C_ROW1; c[ImGuiCol_FrameBgHovered] = C_HOVER;
    c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.10f, 0.14f, 0.98f); c[ImGuiCol_Border] = ImVec4(0, 0, 0, 0);
}

bool UiCapturingKey() { return bindRow != nullptr; }
bool UiTyping() { return editRow != nullptr || ImGui::GetIO().WantTextInput; }
void UiBindKey(int vk) {
    if (!bindRow) return;
    if (vk == VK_ESCAPE) { bindRow = nullptr; return; }
    if (vk == VK_BACK || vk == VK_DELETE) bindRow->hotkey = 0; else bindRow->hotkey = vk;
    bindRow = nullptr;
    T.Save();
}

void UiDraw(bool menuOpen, float dispW, float dispH) {
    C_ACC = ImVec4(T.accent[0], T.accent[1], T.accent[2], 1);
    C_EDIT = Mix(ImVec4(0.10f, 0.10f, 0.14f, 1), C_ACC, 0.35f);
    ImVec4* sc = ImGui::GetStyle().Colors;
    sc[ImGuiCol_FrameBgActive] = C_EDIT; sc[ImGuiCol_ScrollbarGrab] = C_ACC; sc[ImGuiCol_ScrollbarGrabHovered] = C_ACC;
    sc[ImGuiCol_ScrollbarGrabActive] = C_ACC; sc[ImGuiCol_TextSelectedBg] = ImVec4(C_ACC.x, C_ACC.y, C_ACC.z, 0.35f);
    sc[ImGuiCol_NavCursor] = C_ACC; sc[ImGuiCol_CheckMark] = C_ACC;

    Snapshot s;
    std::string toast; double toastUntil;
    { std::lock_guard<std::mutex> l(T.mtx); s = T.snap; toast = T.toast; toastUntil = T.toastUntil; }
    ImDrawList* bg = ImGui::GetBackgroundDrawList();
    DrawEsp(s, dispW, dispH);

    std::vector<std::string> hud;
    if (s.paused) hud.push_back("PAUZA  ·  menu trainera");
    if (s.swapped) hud.push_back("OBCE CIAŁO  ·  powrót: menu (Insert) -> przycisk w nagłówku");
    if (s.arena) hud.push_back("ARENA  ·  fala " + std::to_string(s.arenaWave) + "  ·  wrogów: " + std::to_string(s.arenaAlive));
    if (s.speed) { char b[48]; snprintf(b, sizeof b, "TEMPO ŚWIATA  x%.2f", s.speedVal); hud.push_back(b); }
    if (s.ghost) hud.push_back("TRYB DUCHA");
    if (s.noclip) hud.push_back("NO-CLIP  ·  WASD  Q/E  Spacja/Ctrl");
    float hy = 20 * S;
    PF(fBold);
    for (auto& t : hud) {
        ImVec2 ts = ImGui::CalcTextSize(t.c_str());
        ImVec2 p(dispW - ts.x - 34 * S, hy);
        bg->AddRectFilled(ImVec2(p.x - 12 * S, p.y - 7 * S), ImVec2(p.x + ts.x + 12 * S, p.y + ts.y + 7 * S), U32(C_BG), 8 * S);
        bg->AddRectFilled(ImVec2(p.x - 12 * S, p.y - 7 * S), ImVec2(p.x - 9 * S, p.y + ts.y + 7 * S), U32(C_ACC), 1.5f * S);
        bg->AddText(p, U32(C_TEXT), t.c_str());
        hy += ts.y + 20 * S;
    }
    ImGui::PopFont();
    if (!menuOpen && NowSec() < toastUntil && !toast.empty()) {
        PF(fBold);
        ImVec2 ts = ImGui::CalcTextSize(toast.c_str());
        ImVec2 p(dispW * 0.5f - ts.x * 0.5f, dispH * 0.80f);
        bg->AddRectFilled(ImVec2(p.x - 16 * S, p.y - 9 * S), ImVec2(p.x + ts.x + 16 * S, p.y + ts.y + 9 * S), U32(C_BG), 10 * S);
        bg->AddText(p, U32(C_ACC), toast.c_str());
        ImGui::PopFont();
    }
    if (!menuOpen) { editRow = nullptr; bindRow = nullptr; settingsOpen = false; return; }

    ImGui::SetNextWindowSize(ImVec2(800 * S, 800 * S), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(dispW * 0.5f - 400 * S, dispH * 0.5f - 400 * S), ImGuiCond_FirstUseEver);
    ImGui::Begin("##g1trainer", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 wp = ImGui::GetWindowPos(); float ww = ImGui::GetWindowWidth();
    PF(fBig); ImGui::TextColored(C_TEXT, "GOTHIC"); ImGui::PopFont();
    {
        char acc[48]; snprintf(acc, sizeof acc, "[ Crazy Trainer · build %d ]", TRAINER_BUILD);
        PF(fBold); ImVec2 ts = ImGui::CalcTextSize(acc);
        dl->AddText(ImVec2(wp.x + ww * 0.5f - ts.x * 0.5f, wp.y + 14 * S), U32(C_ACC), acc); ImGui::PopFont();
    }
    PF(fSmall); ImGui::TextColored(C_DIM, "TRAINER / Insert = MENU, Esc = zamknij | kliknij hotkey, żeby przypisać"); ImGui::PopFont();
    // header buttons (right): unswap (only when swapped), settings, search
    float bs = 30 * S, bx = wp.x + ww - 14 * S - bs, by = wp.y + 12 * S;
    if (IconButton("##gsearch", ICO_SEARCH, "Szukaj we wszystkich funkcjach", ImVec2(bx, by), bs, globalSearch ? C_ACC : C_DIM, globalSearch)) { globalSearch = !globalSearch; gsearchFocus = globalSearch; }
    bx -= bs + 6 * S;
    if (IconButton("##gset", ICO_COG, "Ustawienia menu", ImVec2(bx, by), bs, settingsOpen ? C_ACC : C_DIM, settingsOpen)) settingsOpen = !settingsOpen;
    ImVec2 settingsAnchor(bx + bs, by + bs + 6 * S);
    if (s.swapped) {
        const char* lbl = ICO_HOME "  Wróć do bohatera";
        float tw = TextW(lbl, fBold) + 20 * S;
        bx -= tw + 8 * S;
        ImGui::SetCursorScreenPos(ImVec2(bx, by));
        if (ImGui::InvisibleButton("##unswap", ImVec2(tw, bs))) Act_Unswap();
        bool hov = ImGui::IsItemHovered();
        dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx + tw, by + bs), U32(C_ACC, hov ? 1.f : 0.85f), 7 * S);
        PF(fBold); ImVec2 ts = ImGui::CalcTextSize(lbl); dl->AddText(ImVec2(bx + 10 * S, by + (bs - ts.y) * 0.5f), IM_COL32(20, 18, 30, 255), lbl); ImGui::PopFont();
    }
    ImGui::SetCursorScreenPos(ImVec2(wp.x + 14 * S, wp.y + 64 * S));
    for (int i = 0; i < kTabCount; i++) {
        bool act = tab == i && !globalSearch;
        PF(act ? fBold : fReg);
        ImVec2 ts = ImGui::CalcTextSize(kTabs[i]);
        ImVec2 p = ImGui::GetCursorScreenPos();
        if (p.x + ts.x > wp.x + ww - 14 * S) { ImGui::NewLine(); p = ImGui::GetCursorScreenPos(); }
        ImGui::PushID(i);
        if (ImGui::InvisibleButton("##tab", ImVec2(ts.x + 4 * S, ts.y + 8 * S))) { tab = i; search[0] = 0; globalSearch = false; }
        bool hov = ImGui::IsItemHovered();
        ImGui::PopID();
        dl->AddText(ImVec2(p.x + 2 * S, p.y), U32(act ? C_ACC : (hov ? C_TEXT : C_DIM)), kTabs[i]);
        if (act) dl->AddRectFilled(ImVec2(p.x, p.y + ts.y + 3 * S), ImVec2(p.x + ts.x + 4 * S, p.y + ts.y + 5 * S), U32(C_ACC), 1 * S);
        ImGui::PopFont();
        ImGui::SameLine(0, 14 * S);
    }
    ImGui::NewLine();
    ImGui::Dummy(ImVec2(0, 2 * S));

    float footerH = 96 * S;
    ImGui::BeginChild("##content", ImVec2(0, -footerH), false);
    float w = ImGui::GetContentRegionAvail().x - 8 * S;
    if (globalSearch) {
        if (gsearchFocus) { ImGui::SetKeyboardFocusHere(); gsearchFocus = false; }
        InputBox("##gq", gsearch, sizeof gsearch, "Szukaj funkcji we wszystkich zakładkach: np. ruda, teleport, esp, gildia...", w);
        int idx = 0;
        for (auto& r : T.rows)
            if (gsearch[0] && (MatchQ(gsearch, r.name, r.note) || MatchQ(gsearch, r.cat, r.sub))) DrawRow(r, idx++, w, r.cat.c_str());
        if (gsearch[0] && !idx) ImGui::TextColored(C_DIM, "Nic nie znaleziono.");
        if (!gsearch[0]) ImGui::TextColored(C_DIM, "Wpisz fragment nazwy, opisu albo zakładki.");
    } else {
        std::string cat = kTabs[tab];
        if (cat == "Cel") TargetCard(s, w);
        if (cat == "Render") RenderPanel(w);
        DrawRowsGrouped(cat, w);
        if (cat == "Przedmioty") ItemsPanel(w);
        if (cat == "Potwory i NPC") NpcPanel(w);
        if (cat == "Teleport") TeleportPanel(s, w);
        if (cat == "Marvin") MarvinPanel(s, w);
    }
    ImGui::EndChild();

    ImVec2 fp = ImGui::GetCursorScreenPos();
    PlayerCard(s, fp, ww - 28 * S);
    ImGui::SetCursorScreenPos(ImVec2(fp.x, fp.y + 64 * S));
    PF(fSmall);
    {
        ImVec2 p = ImGui::GetCursorScreenPos();
        float x = p.x, pw;
        struct { Status s; const char* t; } lg[] = { { ST_OK, "sprawdzone" }, { ST_BUG, "działa źle" }, { ST_CRASH, "crashuje" }, { ST_NEW, "do przetestowania" } };
        for (auto& l : lg) {
            StatusPill(dl, ImVec2(x, p.y + 2 * S), l.s, "", &pw);
            dl->AddText(ImVec2(x + pw + 4 * S, p.y + 2 * S), U32(C_DIM), l.t);
            x += pw + 12 * S + ImGui::CalcTextSize(l.t).x;
        }
        if (NowSec() < toastUntil + 6 && !toast.empty()) dl->AddText(ImVec2(x + 10 * S, p.y + 2 * S), U32(C_ACC), toast.c_str());
        const char* rl = "[ Restore toggles ]";
        ImVec2 ts = ImGui::CalcTextSize(rl);
        ImVec2 rp(wp.x + ww - ts.x - 16 * S, p.y + 2 * S);
        ImGui::SetCursorScreenPos(rp);
        if (ImGui::InvisibleButton("##restore", ts)) T.RestoreToggles();
        dl->AddText(rp, U32(ImGui::IsItemHovered() ? C_TEXT : C_DIM), rl);
    }
    ImGui::PopFont();
    ImGui::End();
    if (settingsOpen) SettingsPopup(settingsAnchor);
}
