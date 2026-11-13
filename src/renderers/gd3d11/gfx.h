#pragma once
#include <d3d11.h>
#include <dxgi.h>
bool GfxActive();
void GfxApply(ID3D11Device* dev, ID3D11DeviceContext* ctx, IDXGISwapChain* sc);   // before the overlay
void GfxRelease();                                                                // on resize
#include <string>
bool GdxAvailable();          // build 17: GD3D11 settings reachable
bool GdxDistances();          // draw distances found in its settings
std::string GdxInfo();
void GdxTick();               // game thread, ~1 s
