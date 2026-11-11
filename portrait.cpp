#include "portrait.h"
#include "trainer.h"
#include <d3d11.h>
#include <dxgi.h>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cmath>

namespace {
struct Pic { ID3D11ShaderResourceView* srv = nullptr; double at = 0; };
std::unordered_map<uintptr_t, Pic> g_pics;
std::mutex g_mx;
const int kPicSz = 96;
}

void* PortraitFor(uintptr_t npc) {
    std::lock_guard<std::mutex> l(g_mx);
    auto it = g_pics.find(npc);
    return it == g_pics.end() ? nullptr : it->second.srv;
}
void PortraitClear() {
    std::lock_guard<std::mutex> l(g_mx);
    for (auto& p : g_pics) if (p.second.srv) p.second.srv->Release();
    g_pics.clear();
}

void PortraitCapture(ID3D11Device* dev, ID3D11DeviceContext* ctx, IDXGISwapChain* sc, const Snapshot& s, bool menuOpen) {
    if (menuOpen || !s.inGame || s.saved.empty()) return;
    static double last = 0;
    double now = NowSec();
    if (now - last < 0.3) return;
    const SavedInfo* pick = nullptr;
    {
        std::lock_guard<std::mutex> l(g_mx);
        for (auto& si : s.saved) {
            if (!si.onScreen || si.headR <= 0) continue;
            auto it = g_pics.find(si.ptr);
            if (it != g_pics.end() && now - it->second.at < 20.0) continue;   // refresh every 20 s
            pick = &si; break;
        }
    }
    if (!pick) return;
    last = now;
    ID3D11Texture2D* bb = nullptr;
    if (FAILED(sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb)) || !bb) return;
    D3D11_TEXTURE2D_DESC bd; bb->GetDesc(&bd);
    bool bgra = bd.Format == DXGI_FORMAT_B8G8R8A8_UNORM || bd.Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    bool rgba = bd.Format == DXGI_FORMAT_R8G8B8A8_UNORM || bd.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    if ((!bgra && !rgba) || bd.SampleDesc.Count > 1) { bb->Release(); return; }
    float side = std::max(40.f, std::min(420.f, pick->headR * bd.Height * 2.4f));
    int sd = (int)side;
    int cx = (int)(pick->headX * bd.Width), cy = (int)(pick->headY * bd.Height);
    int x0 = std::max(0, std::min((int)bd.Width - sd, cx - sd / 2)), y0 = std::max(0, std::min((int)bd.Height - sd, cy - sd / 2));
    if (sd > (int)bd.Width || sd > (int)bd.Height) { bb->Release(); return; }
    D3D11_TEXTURE2D_DESC td{}; td.Width = sd; td.Height = sd; td.MipLevels = 1; td.ArraySize = 1; td.Format = bd.Format;
    td.SampleDesc.Count = 1; td.Usage = D3D11_USAGE_STAGING; td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Texture2D* stg = nullptr;
    if (FAILED(dev->CreateTexture2D(&td, nullptr, &stg))) { bb->Release(); return; }
    D3D11_BOX box{ (UINT)x0, (UINT)y0, 0, (UINT)(x0 + sd), (UINT)(y0 + sd), 1 };
    ctx->CopySubresourceRegion(stg, 0, 0, 0, 0, bb, 0, &box);
    bb->Release();
    D3D11_MAPPED_SUBRESOURCE m;
    if (FAILED(ctx->Map(stg, 0, D3D11_MAP_READ, 0, &m))) { stg->Release(); return; }
    std::vector<uint32_t> px(kPicSz * kPicSz);
    for (int y = 0; y < kPicSz; y++)
        for (int x = 0; x < kPicSz; x++) {
            int sx = x * sd / kPicSz, sy = y * sd / kPicSz;
            uint32_t c = *(const uint32_t*)((const uint8_t*)m.pData + sy * m.RowPitch + sx * 4);
            if (bgra) c = (c & 0xFF00FF00) | ((c & 0xFF) << 16) | ((c >> 16) & 0xFF);
            px[y * kPicSz + x] = c | 0xFF000000;   // backbuffer alpha is meaningless
        }
    ctx->Unmap(stg, 0);
    stg->Release();
    D3D11_TEXTURE2D_DESC od{}; od.Width = kPicSz; od.Height = kPicSz; od.MipLevels = 1; od.ArraySize = 1;
    od.Format = DXGI_FORMAT_R8G8B8A8_UNORM; od.SampleDesc.Count = 1; od.Usage = D3D11_USAGE_DEFAULT; od.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA init{ px.data(), kPicSz * 4, 0 };
    ID3D11Texture2D* tex = nullptr;
    if (FAILED(dev->CreateTexture2D(&od, &init, &tex))) return;
    ID3D11ShaderResourceView* srv = nullptr;
    dev->CreateShaderResourceView(tex, nullptr, &srv);
    tex->Release();
    if (!srv) return;
    std::lock_guard<std::mutex> l(g_mx);
    Pic& p = g_pics[pick->ptr];
    if (p.srv) p.srv->Release();
    p.srv = srv; p.at = now;
}
