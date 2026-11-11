#pragma once
// Target portraits: a crop of the game frame around a saved character's head, taken
// automatically the first time the character is close and on screen with the menu closed.
#include <cstdint>
struct ID3D11Device; struct ID3D11DeviceContext; struct IDXGISwapChain;
struct Snapshot;
void PortraitCapture(ID3D11Device* dev, ID3D11DeviceContext* ctx, IDXGISwapChain* sc, const Snapshot& s, bool menuOpen);
void* PortraitFor(uintptr_t npc);     // ImTextureID (ID3D11ShaderResourceView*) or null
void PortraitClear();
