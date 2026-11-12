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
