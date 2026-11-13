// Graphics override (build 16): a full-screen colour pass over the finished game frame, before the trainer's
// overlay is drawn. The frame is copied to a texture and drawn back through a small pixel shader.
#include "gfx.h"
#include "trainer.h"
#include "core/common.h"
#include <algorithm>
#include <initializer_list>
#include <d3dcompiler.h>
#include <cstring>

namespace {
ID3D11VertexShader* g_vs = nullptr; ID3D11PixelShader* g_ps = nullptr; ID3D11Buffer* g_cb = nullptr; ID3D11SamplerState* g_smp = nullptr;
ID3D11Texture2D* g_copy = nullptr; ID3D11ShaderResourceView* g_srv = nullptr; D3D11_TEXTURE2D_DESC g_cd{};
bool g_failed = false;
const char* kSrc = R"(
cbuffer P : register(b0) { float bright; float contrast; float sat; float gamma; float temp; float vign; float mode; float sharp; float2 px; float2 pad; };
Texture2D tx : register(t0); SamplerState sm : register(s0);
struct VO { float4 pos : SV_Position; float2 uv : TEXCOORD0; };
VO vs(uint id : SV_VertexID) { VO o; float2 uv = float2((id << 1) & 2, id & 2); o.uv = uv; o.pos = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1); return o; }
float4 ps(VO i) : SV_Target {
    float3 c = tx.Sample(sm, i.uv).rgb;
    if (sharp > 0) {
        float3 n = tx.Sample(sm, i.uv + float2(px.x, 0)).rgb + tx.Sample(sm, i.uv - float2(px.x, 0)).rgb + tx.Sample(sm, i.uv + float2(0, px.y)).rgb + tx.Sample(sm, i.uv - float2(0, px.y)).rgb;
        c = c + (c - n * 0.25) * sharp;
    }
    c = pow(saturate(c), 1.0 / gamma);
    c = c + bright;
    c = (c - 0.5) * contrast + 0.5;
    float l = dot(saturate(c), float3(0.299, 0.587, 0.114));
    c = lerp(float3(l, l, l), c, sat);
    c.r += temp * 0.08; c.b -= temp * 0.08;
    if (mode > 0.5 && mode < 1.5) c = float3(l, l, l);
    else if (mode > 1.5 && mode < 2.5) c = float3(dot(c, float3(.393, .769, .189)), dot(c, float3(.349, .686, .168)), dot(c, float3(.272, .534, .131)));
    else if (mode > 2.5 && mode < 3.5) { float g = saturate(l * 2.4); c = float3(0.12 * g, g, 0.18 * g); }
    else if (mode > 3.5) c = 1 - c;
    if (vign > 0) { float2 d = i.uv - 0.5; c *= 1 - vign * dot(d, d) * 2.4; }
    return float4(saturate(c), 1);
}
)";
bool Init(ID3D11Device* dev) {
    if (g_ps) return true;
    if (g_failed) return false;
    ID3DBlob *v = nullptr, *p = nullptr, *e = nullptr;
    if (FAILED(D3DCompile(kSrc, strlen(kSrc), "gfx", nullptr, nullptr, "vs", "vs_4_0", 0, 0, &v, &e)) ||
        FAILED(D3DCompile(kSrc, strlen(kSrc), "gfx", nullptr, nullptr, "ps", "ps_4_0", 0, 0, &p, &e))) {
        if (e) { Log("grafika: blad shadera %s", (const char*)e->GetBufferPointer()); e->Release(); }
        g_failed = true; return false;
    }
    dev->CreateVertexShader(v->GetBufferPointer(), v->GetBufferSize(), nullptr, &g_vs);
    dev->CreatePixelShader(p->GetBufferPointer(), p->GetBufferSize(), nullptr, &g_ps);
    v->Release(); p->Release();
    D3D11_BUFFER_DESC bd{}; bd.ByteWidth = 48; bd.Usage = D3D11_USAGE_DYNAMIC; bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER; bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    dev->CreateBuffer(&bd, nullptr, &g_cb);
    D3D11_SAMPLER_DESC sd{}; sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT; sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP; sd.MaxLOD = D3D11_FLOAT32_MAX;
    dev->CreateSamplerState(&sd, &g_smp);
    if (!g_vs || !g_ps || !g_cb || !g_smp) { g_failed = true; return false; }
    return true;
}
}

bool GfxActive() {
    using namespace gx;
    if (!On("gfx")) return false;
    return Val("gfxbright") != 0 || Val("gfxcontrast") != 100 || Val("gfxsat") != 100 || Val("gfxgamma") != 100 || Val("gfxtemp") != 0 ||
           Val("gfxsharp") != 0 || Val("gfxvign") != 0 || Val("gfxmode") != 0;
}
void GfxApply(ID3D11Device* dev, ID3D11DeviceContext* ctx, IDXGISwapChain* sc) {
    if (!GfxActive() || !Init(dev)) return;
    ID3D11Texture2D* bb = nullptr;
    if (FAILED(sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb)) || !bb) return;
    D3D11_TEXTURE2D_DESC d; bb->GetDesc(&d);
    if (d.SampleDesc.Count > 1) { bb->Release(); return; }
    if (!g_copy || g_cd.Width != d.Width || g_cd.Height != d.Height || g_cd.Format != d.Format) {
        if (g_srv) { g_srv->Release(); g_srv = nullptr; } if (g_copy) { g_copy->Release(); g_copy = nullptr; }
        D3D11_TEXTURE2D_DESC td = d; td.BindFlags = D3D11_BIND_SHADER_RESOURCE; td.Usage = D3D11_USAGE_DEFAULT; td.CPUAccessFlags = 0; td.MiscFlags = 0; td.MipLevels = 1; td.ArraySize = 1;
        if (FAILED(dev->CreateTexture2D(&td, nullptr, &g_copy)) || FAILED(dev->CreateShaderResourceView(g_copy, nullptr, &g_srv))) { bb->Release(); g_failed = true; return; }
        g_cd = d;
    }
    ID3D11RenderTargetView* rtv = nullptr;
    if (FAILED(dev->CreateRenderTargetView(bb, nullptr, &rtv))) { bb->Release(); return; }
    ctx->CopyResource(g_copy, bb);
    bb->Release();
    // constants
    using namespace gx;
    D3D11_MAPPED_SUBRESOURCE m;
    if (SUCCEEDED(ctx->Map(g_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) {
        float* f = (float*)m.pData;
        f[0] = Val("gfxbright") / 100.f; f[1] = Val("gfxcontrast") / 100.f; f[2] = Val("gfxsat") / 100.f; f[3] = std::max(0.2f, Val("gfxgamma") / 100.f);
        f[4] = Val("gfxtemp") / 100.f; f[5] = Val("gfxvign") / 100.f; f[6] = Val("gfxmode"); f[7] = Val("gfxsharp") / 100.f;
        f[8] = 1.f / d.Width; f[9] = 1.f / d.Height; f[10] = f[11] = 0;
        ctx->Unmap(g_cb, 0);
    }
    // save state
    ID3D11RenderTargetView* oRtv = nullptr; ID3D11DepthStencilView* oDsv = nullptr; ctx->OMGetRenderTargets(1, &oRtv, &oDsv);
    UINT nvp = 1; D3D11_VIEWPORT ovp{}; ctx->RSGetViewports(&nvp, &ovp);
    D3D11_PRIMITIVE_TOPOLOGY otop; ctx->IAGetPrimitiveTopology(&otop);
    ID3D11InputLayout* oil = nullptr; ctx->IAGetInputLayout(&oil);
    ID3D11VertexShader* ovs = nullptr; ctx->VSGetShader(&ovs, nullptr, nullptr);
    ID3D11PixelShader* ops = nullptr; ctx->PSGetShader(&ops, nullptr, nullptr);
    ID3D11GeometryShader* ogs = nullptr; ctx->GSGetShader(&ogs, nullptr, nullptr);
    ID3D11ShaderResourceView* osrv = nullptr; ctx->PSGetShaderResources(0, 1, &osrv);
    ID3D11SamplerState* osmp = nullptr; ctx->PSGetSamplers(0, 1, &osmp);
    ID3D11Buffer* ocb = nullptr; ctx->PSGetConstantBuffers(0, 1, &ocb);
    ID3D11BlendState* obs = nullptr; float obf[4]; UINT osm; ctx->OMGetBlendState(&obs, obf, &osm);
    ID3D11DepthStencilState* ods = nullptr; UINT oref; ctx->OMGetDepthStencilState(&ods, &oref);
    ID3D11RasterizerState* ors = nullptr; ctx->RSGetState(&ors);
    // draw
    D3D11_VIEWPORT vp{ 0, 0, (float)d.Width, (float)d.Height, 0, 1 };
    ctx->OMSetRenderTargets(1, &rtv, nullptr);
    ctx->RSSetViewports(1, &vp);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetInputLayout(nullptr);
    ctx->VSSetShader(g_vs, nullptr, 0); ctx->PSSetShader(g_ps, nullptr, 0); ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->PSSetShaderResources(0, 1, &g_srv); ctx->PSSetSamplers(0, 1, &g_smp); ctx->PSSetConstantBuffers(0, 1, &g_cb);
    ctx->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF); ctx->OMSetDepthStencilState(nullptr, 0); ctx->RSSetState(nullptr);
    ctx->Draw(3, 0);
    // restore
    ID3D11ShaderResourceView* nullSrv = nullptr; ctx->PSSetShaderResources(0, 1, &nullSrv);
    ctx->OMSetRenderTargets(1, &oRtv, oDsv); ctx->RSSetViewports(1, &ovp); ctx->IASetPrimitiveTopology(otop); ctx->IASetInputLayout(oil);
    ctx->VSSetShader(ovs, nullptr, 0); ctx->PSSetShader(ops, nullptr, 0); ctx->GSSetShader(ogs, nullptr, 0);
    ctx->PSSetShaderResources(0, 1, &osrv); ctx->PSSetSamplers(0, 1, &osmp); ctx->PSSetConstantBuffers(0, 1, &ocb);
    ctx->OMSetBlendState(obs, obf, osm); ctx->OMSetDepthStencilState(ods, oref); ctx->RSSetState(ors);
    for (IUnknown* u : std::initializer_list<IUnknown*>{ oRtv, oDsv, oil, ovs, ops, ogs, osrv, osmp, ocb, obs, ods, ors }) if (u) u->Release();
    rtv->Release();
}
void GfxRelease() { if (g_srv) { g_srv->Release(); g_srv = nullptr; } if (g_copy) { g_copy->Release(); g_copy = nullptr; } g_cd = {}; }

// ---- build 17: GD3D11 renderer settings ---------------------------------------------------------------------
// GD3D11 (ddraw.dll) exports GDX_Get/SetXxx. Offsets inside its settings struct are found by probing: a setter is
// called with a marker value, the changed float is located, the old value is put back. Draw distances are read
// relative to GlobalWindStrength for the two struct layouts in use (checked by value ranges), else left out.
namespace {
using fnGet = void* (__cdecl*)(unsigned int&);
using fnSetF = void(__cdecl*)(float);
using fnSetB = void(__cdecl*)(bool);
struct Gdx { bool tried = false, ok = false, layout = false; char* base = nullptr; unsigned size = 0;
             int oWind = -1, oFog = -1, oSun = -1, oShadow = -1, oSection = -1, oIndoor = -1, oOutdoor = -1, oSkel = -1, oSmall = -1, oFx = -1;
             float origF[16]{}; int origSection = 0; bool saved = false; fnSetB scatter = nullptr; std::string version; } G;
int Probe(fnSetF set) {
    if (!set || !G.base) return -1;
    std::vector<char> before(G.base, G.base + G.size);
    const float mark = 12345.6789f;
    set(mark);
    int found = -1;
    for (unsigned o = 0; o + 4 <= G.size; o += 4) if (*(float*)(G.base + o) == mark) { found = (int)o; break; }
    if (found >= 0) memcpy(G.base + found, before.data() + found, 4); else memcpy(G.base, before.data(), G.size);
    return found;
}
bool InRange(int o, float lo, float hi) { if (o < 0 || o + 4 > (int)G.size) return false; float v = *(float*)(G.base + o); return v >= lo && v <= hi; }
bool InRangeI(int o, int lo, int hi) { if (o < 0 || o + 4 > (int)G.size) return false; int v = *(int*)(G.base + o); return v >= lo && v <= hi; }
void GdxInit() {
    if (G.tried) return;
    G.tried = true;
    HMODULE m = GetModuleHandleA("ddraw.dll");
    if (!m) return;
    auto get = (fnGet)GetProcAddress(m, "GDX_GetRendererSettings");
    if (!get) return;
    if (auto ver = (const char* (__cdecl*)())GetProcAddress(m, "GDX_GetVersionString")) { const char* v = ver(); if (v) G.version = v; }
    G.base = (char*)get(G.size);
    if (!G.base || G.size < 64 || G.size > 65536) return;
    G.oWind = Probe((fnSetF)GetProcAddress(m, "GDX_SetGlobalWindStrength"));
    G.oFog = Probe((fnSetF)GetProcAddress(m, "GDX_SetFogDensity"));
    G.oSun = Probe((fnSetF)GetProcAddress(m, "GDX_SetSunStrength"));
    G.oShadow = Probe((fnSetF)GetProcAddress(m, "GDX_SetShadowStrength"));
    G.scatter = (fnSetB)GetProcAddress(m, "GDX_SetAtmosphericScattering");
    G.ok = G.oWind >= 0;
    if (G.ok) {
        struct L { int sec, ind, out, skel, small, fx; } lay[2] = { { -60, -56, -52, -48, -44, -40 }, { -52, -48, -44, -40, -36, -32 } };
        for (auto& l : lay) {
            int w = G.oWind;
            if (InRangeI(w + l.sec, 1, 128) && InRange(w + l.ind, 100.f, 2e6f) && InRange(w + l.out, 100.f, 2e6f) && InRange(w + l.skel, 100.f, 2e6f) &&
                InRange(w + l.small, 100.f, 2e6f) && InRange(w + l.fx, 100.f, 2e6f)) {
                G.oSection = w + l.sec; G.oIndoor = w + l.ind; G.oOutdoor = w + l.out; G.oSkel = w + l.skel; G.oSmall = w + l.small; G.oFx = w + l.fx; G.layout = true; break;
            }
        }
    }
    Log("GD3D11: %s, ustawienia %u B, wiatr@%d mgla@%d slonce@%d cienie@%d zasiegi=%s", G.version.c_str(), G.size, G.oWind, G.oFog, G.oSun, G.oShadow, G.layout ? "tak" : "nie");
}
float& F(int o) { return *(float*)(G.base + o); }
}
bool GdxAvailable() { GdxInit(); return G.ok; }
bool GdxDistances() { GdxInit(); return G.layout; }
std::string GdxInfo() { GdxInit(); return !G.base ? "nie znaleziono GD3D11 (ddraw.dll)" : !G.ok ? "GD3D11 bez dostępu do ustawień" : (G.version.empty() ? std::string("GD3D11") : G.version) + (G.layout ? "" : " - zasięgi rysowania niedostępne w tej wersji"); }
// called every second from the game thread: on = apply rows, off = put the original values back once
void GdxTick() {
    using namespace gx;
    GdxInit();
    if (!G.ok) return;
    const int offs[9] = { G.oOutdoor, G.oSmall, G.oSkel, G.oFx, G.oIndoor, G.oFog, G.oSun, G.oShadow, G.oWind };
    if (On("gdx")) {
        if (!G.saved) {
            for (int i = 0; i < 9; i++) if (offs[i] >= 0) G.origF[i] = F(offs[i]);
            if (G.oSection >= 0) G.origSection = *(int*)(G.base + G.oSection);
            G.saved = true;
            if (Row* r = T.Get("gdxinit")) if (r->val == 0) {   // first use: start from the game's own values
                auto set = [](const char* id, float v) { if (Row* q = T.Get(id)) q->val = std::max(q->vmin, std::min(q->vmax, v)); };
                if (G.oOutdoor >= 0) set("gdxdraw", G.origF[0] / 100.f);
                if (G.oSmall >= 0) set("gdxsmall", G.origF[1] / 100.f);
                if (G.oSkel >= 0) set("gdxnpc", G.origF[2] / 100.f);
                if (G.oFx >= 0) set("gdxfx", G.origF[3] / 100.f);
                if (G.oSection >= 0) set("gdxsect", (float)G.origSection);
                r->val = 1; T.Save();
            }
        }
        if (G.oOutdoor >= 0) F(G.oOutdoor) = Val("gdxdraw") * 100.f;
        if (G.oIndoor >= 0) F(G.oIndoor) = std::max(G.origF[4], Val("gdxdraw") * 50.f);
        if (G.oSmall >= 0) F(G.oSmall) = Val("gdxsmall") * 100.f;
        if (G.oSkel >= 0) F(G.oSkel) = Val("gdxnpc") * 100.f;
        if (G.oFx >= 0) F(G.oFx) = Val("gdxfx") * 100.f;
        if (G.oSection >= 0) *(int*)(G.base + G.oSection) = (int)Val("gdxsect");
        if (G.oFog >= 0) F(G.oFog) = G.origF[5] * Val("gdxfog") / 100.f;
        if (G.oSun >= 0) F(G.oSun) = G.origF[6] * Val("gdxsun") / 100.f;
        if (G.oShadow >= 0) F(G.oShadow) = G.origF[7] * Val("gdxshadow") / 100.f;
        if (G.oWind >= 0) F(G.oWind) = G.origF[8] * Val("gdxwind") / 100.f;
        static int lastSc = -1; int sc = On("gdxscatter") ? 1 : 0;
        if (G.scatter && sc != lastSc) { G.scatter(sc != 0); lastSc = sc; }
    } else if (G.saved) {
        for (int i = 0; i < 9; i++) if (offs[i] >= 0) F(offs[i]) = G.origF[i];
        if (G.oSection >= 0) *(int*)(G.base + G.oSection) = G.origSection;
        G.saved = false;
    }
}
