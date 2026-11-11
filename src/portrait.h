#pragma once
// Character portraits rendered by the game itself: for two frames the camera is moved in
// front of the character's face (frame A with the character, frame B with it hidden); the
// difference of both frames is the character -> transparent background, faded at the bottom.
#include <cstdint>
struct ID3D11Device; struct ID3D11DeviceContext; struct IDXGISwapChain;
// shared state between the game thread (camera/visibility) and Present (capture)
// 0 idle, 1 frame A rendered -> capture, 2 A captured, 3 frame B rendered -> capture, 4 done
extern volatile int g_photoStage;
extern volatile uintptr_t g_photoNpc;
void PortraitPresent(ID3D11Device* dev, ID3D11DeviceContext* ctx, IDXGISwapChain* sc);
void* PortraitFor(uintptr_t npc);     // ImTextureID (ID3D11ShaderResourceView*) or null
void PortraitClear();
