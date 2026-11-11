#include "portrait.h"
#include "trainer.h"
#include <d3d11.h>
#include <dxgi.h>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cmath>

volatile int g_photoStage = 0;
volatile uintptr_t g_photoNpc = 0;

namespace {
struct Pic { ID3D11ShaderResourceView* srv = nullptr; };
std::unordered_map<uintptr_t, Pic> g_pics;
std::mutex g_mx;
const int kPicSz = 160;
std::vector<uint32_t> g_a;   // frame A crop (RGBA)

// crop of the backbuffer centre, resampled to kPicSz x kPicSz, RGBA
bool Grab(ID3D11Device* dev, ID3D11DeviceContext* ctx, IDXGISwapChain* sc, std::vector<uint32_t>& out) {
    ID3D11Texture2D* bb = nullptr;
    if (FAILED(sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb)) || !bb) return false;
    D3D11_TEXTURE2D_DESC bd; bb->GetDesc(&bd);
    bool bgra = bd.Format == DXGI_FORMAT_B8G8R8A8_UNORM || bd.Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    bool rgba = bd.Format == DXGI_FORMAT_R8G8B8A8_UNORM || bd.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    if ((!bgra && !rgba) || bd.SampleDesc.Count > 1) { bb->Release(); return false; }
    int sd = (int)(bd.Height * 0.56f);
    int x0 = ((int)bd.Width - sd) / 2, y0 = ((int)bd.Height - sd) / 2;
    D3D11_TEXTURE2D_DESC td{}; td.Width = sd; td.Height = sd; td.MipLevels = 1; td.ArraySize = 1; td.Format = bd.Format;
    td.SampleDesc.Count = 1; td.Usage = D3D11_USAGE_STAGING; td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Texture2D* stg = nullptr;
    if (FAILED(dev->CreateTexture2D(&td, nullptr, &stg))) { bb->Release(); return false; }
    D3D11_BOX box{ (UINT)x0, (UINT)y0, 0, (UINT)(x0 + sd), (UINT)(y0 + sd), 1 };
    ctx->CopySubresourceRegion(stg, 0, 0, 0, 0, bb, 0, &box);
    bb->Release();
    D3D11_MAPPED_SUBRESOURCE m;
    if (FAILED(ctx->Map(stg, 0, D3D11_MAP_READ, 0, &m))) { stg->Release(); return false; }
    out.assign(kPicSz * kPicSz, 0);
    for (int y = 0; y < kPicSz; y++)
        for (int x = 0; x < kPicSz; x++) {
            // 2x2 box filter for a smoother downscale
            uint32_t acc[4] = { 0, 0, 0, 0 };
            for (int k = 0; k < 4; k++) {
                int sx = std::min(sd - 1, (x * 2 + (k & 1)) * sd / (kPicSz * 2)), sy = std::min(sd - 1, (y * 2 + (k >> 1)) * sd / (kPicSz * 2));
                uint32_t c = *(const uint32_t*)((const uint8_t*)m.pData + sy * m.RowPitch + sx * 4);
                if (bgra) c = (c & 0xFF00FF00) | ((c & 0xFF) << 16) | ((c >> 16) & 0xFF);
                for (int ch = 0; ch < 3; ch++) acc[ch] += (c >> (ch * 8)) & 0xFF;
            }
            out[y * kPicSz + x] = (acc[0] / 4) | ((acc[1] / 4) << 8) | ((acc[2] / 4) << 16) | 0xFF000000u;
        }
    ctx->Unmap(stg, 0);
    stg->Release();
    return true;
}
void Build(ID3D11Device* dev, const std::vector<uint32_t>& a, const std::vector<uint32_t>& b, uintptr_t npc) {
    int n = kPicSz;
    std::vector<float> mask(n * n, 0.f), tmp(n * n);
    int on = 0;
    for (int i = 0; i < n * n; i++) {
        int d = 0;
        for (int ch = 0; ch < 3; ch++) d = std::max(d, std::abs((int)((a[i] >> (ch * 8)) & 0xFF) - (int)((b[i] >> (ch * 8)) & 0xFF)));
        mask[i] = std::max(0.f, std::min(1.f, (d - 6) / 22.f));
        if (mask[i] > 0.5f) on++;
    }
    bool keyed = on > n * n / 30;   // the character really disappeared in frame B
    if (keyed) {
        for (int pass = 0; pass < 2; pass++) {   // close small holes (dilate), then soften the edge (blur)
            for (int y = 0; y < n; y++)
                for (int x = 0; x < n; x++) {
                    float v = 0, sum = 0; int c = 0;
                    for (int dy = -1; dy <= 1; dy++)
                        for (int dx = -1; dx <= 1; dx++) {
                            int xx = std::max(0, std::min(n - 1, x + dx)), yy = std::max(0, std::min(n - 1, y + dy));
                            v = std::max(v, mask[yy * n + xx]); sum += mask[yy * n + xx]; c++;
                        }
                    tmp[y * n + x] = pass == 0 ? v : sum / c;
                }
            mask.swap(tmp);
        }
    } else std::fill(mask.begin(), mask.end(), 1.f);
    std::vector<uint32_t> px(n * n);
    for (int y = 0; y < n; y++) {
        float fy = (float)y / (n - 1);
        float g = fy < 0.52f ? 1.f : std::max(0.f, 1.f - (fy - 0.52f) / 0.46f);
        g = g * g * (3 - 2 * g);   // smooth fade to transparent at the bottom
        for (int x = 0; x < n; x++) {
            float al = mask[y * n + x] * g;
            if (!keyed) { float fx = (x - n * 0.5f) / (n * 0.5f); al *= std::max(0.f, std::min(1.f, 1.6f - fx * fx * 1.4f)); }
            px[y * n + x] = (a[y * n + x] & 0x00FFFFFF) | ((uint32_t)(al * 255.f) << 24);
        }
    }
    D3D11_TEXTURE2D_DESC od{}; od.Width = n; od.Height = n; od.MipLevels = 1; od.ArraySize = 1;
    od.Format = DXGI_FORMAT_R8G8B8A8_UNORM; od.SampleDesc.Count = 1; od.Usage = D3D11_USAGE_DEFAULT; od.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA init{ px.data(), (UINT)(n * 4), 0 };
    ID3D11Texture2D* tex = nullptr;
    if (FAILED(dev->CreateTexture2D(&od, &init, &tex))) return;
    ID3D11ShaderResourceView* srv = nullptr;
    dev->CreateShaderResourceView(tex, nullptr, &srv);
    tex->Release();
    if (!srv) return;
    std::lock_guard<std::mutex> l(g_mx);
    Pic& p = g_pics[npc];
    if (p.srv) p.srv->Release();
    p.srv = srv;
}
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
void PortraitPresent(ID3D11Device* dev, ID3D11DeviceContext* ctx, IDXGISwapChain* sc) {
    if (g_photoStage == 1) {
        if (!Grab(dev, ctx, sc, g_a)) { g_photoStage = 4; return; }
        g_photoStage = 2;
    } else if (g_photoStage == 3) {
        std::vector<uint32_t> b;
        if (Grab(dev, ctx, sc, b) && b.size() == g_a.size()) Build(dev, g_a, b, g_photoNpc);
        g_photoStage = 4;
    }
}
