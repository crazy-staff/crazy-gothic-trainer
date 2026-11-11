// Entry point: loaded by Union from System\Autorun. Hooks the GD3D11 swap chain for the
// menu and oCGame::Render for game-thread work.
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <cstdio>
#include <cstdarg>
#include <algorithm>
#include "MinHook.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "trainer.h"
#include "ui.h"
#include "portrait.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static HMODULE g_self;
static FILE* g_log;
void Log(const char* fmt, ...) {
    if (!g_log) return;
    va_list a; va_start(a, fmt); vfprintf(g_log, fmt, a); va_end(a);
    fputc('\n', g_log); fflush(g_log);
}

using PresentFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);
using ResizeFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
static PresentFn oPresent;
static ResizeFn oResize;
static ID3D11Device* g_dev;
static ID3D11DeviceContext* g_ctx;
static ID3D11RenderTargetView* g_rtv;
HWND g_gameHwnd;
#define g_hwnd g_gameHwnd
static WNDPROC g_oldProc;
static bool g_imgui;
static IDXGISwapChain* g_sc;

static void MakeRTV(IDXGISwapChain* sc) {
    ID3D11Texture2D* bb = nullptr;
    if (SUCCEEDED(sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb)) && bb) {
        g_dev->CreateRenderTargetView(bb, nullptr, &g_rtv);
        bb->Release();
    }
}

static DWORD g_hookThread;
static void SetMenu(bool open) {
    if (T.menuOpen == open) return;
    T.menuOpen = open;
    ImGui::GetIO().MouseDrawCursor = open;
    if (g_hookThread) PostThreadMessageA(g_hookThread, open ? WM_APP + 1 : WM_APP + 2, 0, 0);
    if (!open) T.SaveNow();
}

// Low-level mouse hook: after alt-tab the game grabs the mouse through DirectInput and the
// window stops getting WM_MOUSEWHEEL, so the menu reads the wheel here instead.
static HHOOK g_mouseHook;
static volatile LONG g_wheel = 0;
static LRESULT CALLBACK LLMouse(int code, WPARAM w, LPARAM l);
// The hook lives on its own thread with its own message loop (a low-level hook on a thread that does
// not pump messages lags the mouse for the whole system - that broke Win+Shift+S) and is installed
// only while the menu is open.
static DWORD WINAPI HookThread(LPVOID) {
    MSG m;
    PeekMessageA(&m, nullptr, 0, 0, PM_NOREMOVE);
    while (GetMessageA(&m, nullptr, 0, 0) > 0) {
        if (m.message == WM_APP + 1 && !g_mouseHook) g_mouseHook = SetWindowsHookExA(WH_MOUSE_LL, LLMouse, g_self, 0);
        if (m.message == WM_APP + 2 && g_mouseHook) { UnhookWindowsHookEx(g_mouseHook); g_mouseHook = nullptr; }
    }
    return 0;
}
static LRESULT CALLBACK LLMouse(int code, WPARAM w, LPARAM l) {
    if (code == HC_ACTION && w == WM_MOUSEWHEEL && T.menuOpen && GetForegroundWindow() == g_gameHwnd) {
        auto* m = (MSLLHOOKSTRUCT*)l;
        InterlockedExchangeAdd(&g_wheel, (short)HIWORD(m->mouseData));
        return 1;
    }
    return CallNextHookEx(g_mouseHook, code, w, l);
}

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (g_imgui) {
        if (m == WM_KEYDOWN || m == WM_SYSKEYDOWN) {
            int vk = (int)w;
            bool repeat = (l >> 30) & 1;
            if (UiCapturingKey()) { if (!repeat) UiBindKey(vk); return 0; }
            if (!repeat && vk == VK_INSERT) { SetMenu(!T.menuOpen); return 0; }
            if (!repeat && vk == VK_ESCAPE && T.menuOpen && !UiTyping()) { SetMenu(false); return 0; }
            if (!repeat && vk == VK_ESCAPE && T.snap.photoBatch) { Act_PhotoCancel(); return 0; }
            if (!repeat && vk == VK_ESCAPE && !T.menuOpen) {   // Esc leaves no-clip first (game menu/saving then sees a normal hero)
                Row* nc = T.Get("noclip");
                if (nc && nc->on) { ApplyRow(*nc); return 0; }
            }
            bool typing = T.menuOpen && ImGui::GetIO().WantTextInput;
            if (!repeat && !typing && GetForegroundWindow() == h) T.Hotkey(vk);
        }
        if (T.menuOpen) {
            if (m != WM_MOUSEWHEEL || !g_mouseHook) ImGui_ImplWin32_WndProcHandler(h, m, w, l);
            else if (m == WM_MOUSEWHEEL) { /* delivered by the hook */ }
            switch (m) {
            case WM_KEYDOWN: case WM_KEYUP: case WM_SYSKEYDOWN: case WM_SYSKEYUP: case WM_CHAR:
            case WM_MOUSEMOVE: case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_RBUTTONDOWN: case WM_RBUTTONUP:
            case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MOUSEWHEEL: case WM_LBUTTONDBLCLK: case WM_RBUTTONDBLCLK:
            case WM_SETCURSOR: case WM_INPUT: case WM_MOUSEHWHEEL:
                return m == WM_SETCURSOR ? TRUE : 0;
            }
        }
    }
    return CallWindowProcA(g_oldProc, h, m, w, l);
}

static HRESULT __stdcall hkPresent(IDXGISwapChain* sc, UINT sync, UINT flags) {
    if (!g_imgui) {
        if (SUCCEEDED(sc->GetDevice(__uuidof(ID3D11Device), (void**)&g_dev))) {
            g_dev->GetImmediateContext(&g_ctx);
            DXGI_SWAP_CHAIN_DESC d; sc->GetDesc(&d);
            g_hwnd = d.OutputWindow;
            MakeRTV(sc);
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.IniFilename = nullptr;
            io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
            ImGui_ImplWin32_Init(g_hwnd);
            ImGui_ImplDX11_Init(g_dev, g_ctx);
            RECT rc; GetClientRect(g_hwnd, &rc);
            UiInitFonts((float)std::max<LONG>(600, rc.bottom - rc.top));
            g_oldProc = (WNDPROC)SetWindowLongPtrA(g_hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
            g_imgui = true; g_sc = sc;
            CreateThread(nullptr, 0, HookThread, nullptr, 0, &g_hookThread);
            Log("imgui ready hwnd=%p %ldx%ld", g_hwnd, rc.right - rc.left, rc.bottom - rc.top);
            T.Toast("Gothic Crazy Trainer gotowy - Insert");
        }
    }
    if (g_imgui && g_rtv) {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        if (T.menuOpen && GetForegroundWindow() == g_hwnd) {
            // after alt-tab the game may re-grab the mouse: poll it ourselves so the menu always works
            ClipCursor(nullptr);
            POINT pt; GetCursorPos(&pt); ScreenToClient(g_hwnd, &pt);
            RECT rc; GetClientRect(g_hwnd, &rc);
            ImGuiIO& io2 = ImGui::GetIO();
            float sx = rc.right > 0 ? io2.DisplaySize.x / (float)rc.right : 1.f, sy = rc.bottom > 0 ? io2.DisplaySize.y / (float)rc.bottom : 1.f;
            io2.AddMousePosEvent(pt.x * sx, pt.y * sy);
            static bool lb = false, rb = false;
            bool nl = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0, nr = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
            if (nl != lb) { io2.AddMouseButtonEvent(0, nl); lb = nl; }
            if (nr != rb) { io2.AddMouseButtonEvent(1, nr); rb = nr; }
            LONG wh = InterlockedExchange(&g_wheel, 0);
            if (wh) io2.AddMouseWheelEvent(0, wh / 120.f);
        }
        {   // portraits: capture the clean game frame before our overlay is drawn
            static std::string lastWorld; std::string wn;
            { std::lock_guard<std::mutex> l(T.mtx); wn = T.snap.world; }
            PortraitPresent(g_dev, g_ctx, sc);
        }
        ImGui::NewFrame();
        ImGuiIO& io = ImGui::GetIO();
        UiDraw(T.menuOpen, io.DisplaySize.x, io.DisplaySize.y);
        ImGui::Render();
        ID3D11RenderTargetView* old = nullptr; ID3D11DepthStencilView* oldDs = nullptr;
        g_ctx->OMGetRenderTargets(1, &old, &oldDs);
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_ctx->OMSetRenderTargets(1, &old, oldDs);
        if (old) old->Release();
        if (oldDs) oldDs->Release();
    }
    return oPresent(sc, sync, flags);
}

static HRESULT __stdcall hkResize(IDXGISwapChain* sc, UINT n, UINT w, UINT h, DXGI_FORMAT f, UINT fl) {
    if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
    if (g_imgui) ImGui_ImplDX11_InvalidateDeviceObjects();
    HRESULT hr = oResize(sc, n, w, h, f, fl);
    if (g_dev) MakeRTV(sc);
    if (g_imgui) ImGui_ImplDX11_CreateDeviceObjects();
    return hr;
}

static bool HookDx11() {
    WNDCLASSEXA wc{ sizeof(wc) }; wc.lpfnWndProc = DefWindowProcA; wc.hInstance = g_self; wc.lpszClassName = "g1t_dummy";
    RegisterClassExA(&wc);
    HWND w = CreateWindowA("g1t_dummy", "", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr, g_self, nullptr);
    DXGI_SWAP_CHAIN_DESC d{};
    d.BufferCount = 1; d.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; d.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    d.OutputWindow = w; d.SampleDesc.Count = 1; d.Windowed = TRUE; d.BufferDesc.Width = 64; d.BufferDesc.Height = 64;
    IDXGISwapChain* sc = nullptr; ID3D11Device* dev = nullptr; ID3D11DeviceContext* ctx = nullptr;
    D3D_FEATURE_LEVEL fl;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
                                               &d, &sc, &dev, &fl, &ctx);
    if (FAILED(hr)) {
        hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
                                           &d, &sc, &dev, &fl, &ctx);
    }
    if (FAILED(hr)) { Log("dummy device failed %08lx", hr); DestroyWindow(w); return false; }
    void** vt = *(void***)sc;
    MH_CreateHook(vt[8], (LPVOID)&hkPresent, (LPVOID*)&oPresent);
    MH_CreateHook(vt[13], (LPVOID)&hkResize, (LPVOID*)&oResize);
    MH_EnableHook(vt[8]); MH_EnableHook(vt[13]);
    sc->Release(); ctx->Release(); dev->Release();
    DestroyWindow(w);
    Log("dx11 hooks installed");
    return true;
}

static DWORD WINAPI InitThread(LPVOID) {
    std::string logPath = IniPath(); logPath.replace(logPath.size() - 3, 3, "log");
    {   // keep previous sessions (crash info!) - append, rotate when big
        WIN32_FILE_ATTRIBUTE_DATA fa{};
        if (GetFileAttributesExA(logPath.c_str(), GetFileExInfoStandard, &fa) && fa.nFileSizeLow > 2 * 1024 * 1024) {
            std::string old = logPath + ".old"; DeleteFileA(old.c_str()); MoveFileA(logPath.c_str(), old.c_str());
        }
    }
    g_log = fopen(logPath.c_str(), "a");
    if (g_log) { SYSTEMTIME t; GetLocalTime(&t); fprintf(g_log, "\n===== sesja %04d-%02d-%02d %02d:%02d:%02d =====\n", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond); }
    Log("Gothic Crazy Trainer build %d loaded", TRAINER_BUILD);
    CrashGuardInit();
    MH_Initialize();
    T.Init();
    T.Load();
    InstallGameHooks();
    Log("game hooks installed");
    Sleep(300);
    HookDx11();
    return 0;
}

BOOL APIENTRY DllMain(HMODULE m, DWORD r, LPVOID) {
    if (r == DLL_PROCESS_ATTACH) {
        g_self = m;
        DisableThreadLibraryCalls(m);
        char exe[MAX_PATH]; GetModuleFileNameA(nullptr, exe, MAX_PATH);
        CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
