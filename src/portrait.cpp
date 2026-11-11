#include "portrait.h"
#include "trainer.h"
#include <d3d11.h>
#include <dxgi.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdio>

volatile int g_photoStage = 0;
char g_photoKey[128] = "";
volatile int g_photoItem = 0;

namespace {
struct Pic { ID3D11ShaderResourceView* srv = nullptr; };
std::unordered_map<std::string, Pic> g_pics;
std::unordered_set<std::string> g_missing;
std::recursive_mutex g_mx;
ID3D11Device* g_dev = nullptr;
const int kPicSz = 160;
std::vector<uint32_t> g_a, g_b, g_a2;

std::string Dir() { std::string d = DataPath("G1CrazyTrainer_obrazki"); CreateDirectoryA(d.c_str(), nullptr); return d + "\\"; }
std::string FileOf(const std::string& key) {
    std::string k;
    for (char c : key) k += (isalnum((unsigned char)c) || c == '_' || c == '-') ? c : '_';
    return Dir() + k + ".rgba";
}
ID3D11ShaderResourceView* MakeTex(const uint32_t* px) {
    if (!g_dev) return nullptr;
    D3D11_TEXTURE2D_DESC od{}; od.Width = kPicSz; od.Height = kPicSz; od.MipLevels = 1; od.ArraySize = 1;
    od.Format = DXGI_FORMAT_R8G8B8A8_UNORM; od.SampleDesc.Count = 1; od.Usage = D3D11_USAGE_DEFAULT; od.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA init{ px, (UINT)(kPicSz * 4), 0 };
    ID3D11Texture2D* tex = nullptr;
    if (FAILED(g_dev->CreateTexture2D(&od, &init, &tex))) return nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    g_dev->CreateShaderResourceView(tex, nullptr, &srv);
    tex->Release();
    return srv;
}
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
            uint32_t acc[3] = { 0, 0, 0 };
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
float Diff(uint32_t x, uint32_t y) {
    int d = 0;
    for (int ch = 0; ch < 3; ch++) d += std::abs((int)((x >> (ch * 8)) & 0xFF) - (int)((y >> (ch * 8)) & 0xFF));
    return std::max(0.f, std::min(1.f, (d - 10) / 30.f));
}
// a = subject, b = background; a2/b2 = the same shot turned 180 degrees (different background):
// a pixel that matches the background colour in one shot almost never matches in the other
void Build(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b, const std::vector<uint32_t>& a2, const std::vector<uint32_t>& b2,
           const std::string& key, bool item) {
    int n = kPicSz;
    std::vector<float> mask(n * n, 0.f), tmp(n * n);
    int on = 0;
    bool two = a2.size() == a.size() && b2.size() == a.size();
    for (int i = 0; i < n * n; i++) {
        mask[i] = Diff(a[i], b[i]);
        if (two) mask[i] = std::max(mask[i], Diff(a2[i], b2[i]));
        if (mask[i] > 0.5f) on++;
    }
    if (on < n * n / 200) return;   // nothing captured: keep trying later instead of saving garbage
    for (int pass = 0; pass < 2; pass++) {
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
    std::vector<uint32_t> px(n * n);
    for (int y = 0; y < n; y++) {
        float fy = (float)y / (n - 1);
        float g = item ? 1.f : (fy < 0.55f ? 1.f : std::max(0.f, 1.f - (fy - 0.55f) / 0.43f));
        g = g * g * (3 - 2 * g);
        for (int x = 0; x < n; x++) {
            float al = mask[y * n + x] * g;
            px[y * n + x] = (a[y * n + x] & 0x00FFFFFF) | ((uint32_t)(al * 255.f) << 24);
        }
    }
    if (FILE* f = fopen(FileOf(key).c_str(), "wb")) { fwrite(px.data(), 4, px.size(), f); fclose(f); }
    ID3D11ShaderResourceView* srv = MakeTex(px.data());
    if (!srv) return;
    std::lock_guard<std::recursive_mutex> l(g_mx);
    Pic& p = g_pics[key];
    if (p.srv) p.srv->Release();
    p.srv = srv;
    g_missing.erase(key);
}
}

bool PortraitHas(const std::string& key) {
    std::lock_guard<std::recursive_mutex> l(g_mx);
    if (g_pics.count(key)) return true;
    return GetFileAttributesA(FileOf(key).c_str()) != INVALID_FILE_ATTRIBUTES;
}
void* PortraitFor(const std::string& key) {
    if (key.empty()) return nullptr;
    std::lock_guard<std::recursive_mutex> l(g_mx);
    auto it = g_pics.find(key);
    if (it != g_pics.end()) return it->second.srv;
    if (g_missing.count(key) || !g_dev) return nullptr;
    std::vector<uint32_t> px(kPicSz * kPicSz);
    FILE* f = fopen(FileOf(key).c_str(), "rb");
    if (!f) { g_missing.insert(key); return nullptr; }
    size_t rd = fread(px.data(), 4, px.size(), f); fclose(f);
    if (rd != px.size()) { g_missing.insert(key); return nullptr; }
    ID3D11ShaderResourceView* srv = MakeTex(px.data());
    g_pics[key].srv = srv;
    return srv;
}
void PortraitClear() {
    std::lock_guard<std::recursive_mutex> l(g_mx);
    for (auto& p : g_pics) if (p.second.srv) p.second.srv->Release();
    g_pics.clear(); g_missing.clear();
}
void PortraitPresent(ID3D11Device* dev, ID3D11DeviceContext* ctx, IDXGISwapChain* sc) {
    g_dev = dev;
    // 1 A (subject) -> 3 B (hidden) -> 5 A2 (turned) -> 7 B2 (hidden) -> build -> 8
    std::vector<uint32_t>* dst = g_photoStage == 1 ? &g_a : g_photoStage == 3 ? &g_b : g_photoStage == 5 ? &g_a2 : nullptr;
    if (dst) { if (!Grab(dev, ctx, sc, *dst)) { g_photoStage = 8; return; } g_photoStage = g_photoStage + 1; return; }
    if (g_photoStage == 7) {
        std::vector<uint32_t> b2;
        if (Grab(dev, ctx, sc, b2) && g_b.size() == g_a.size()) Build(g_a, g_b, g_a2, b2, g_photoKey, g_photoItem != 0);
        g_photoStage = 8;
    }
}
