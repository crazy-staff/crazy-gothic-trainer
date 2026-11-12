#pragma once
#include <d3d11.h>
#include <dxgi.h>
bool GfxActive();
void GfxApply(ID3D11Device* dev, ID3D11DeviceContext* ctx, IDXGISwapChain* sc);   // before the overlay
void GfxRelease();                                                                // on resize
