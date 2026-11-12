#pragma once
// Character portraits / item icons rendered by the game itself. The game thread parks the camera in
// front of the subject (frame A), then hides the subject (frame B); Present cuts the difference out
// -> transparent background, faded at the bottom. Results are cached as files by key (instance id).
#include <cstdint>
#include <string>
#include <vector>
struct ID3D11Device; struct ID3D11DeviceContext; struct IDXGISwapChain;
// odd = capture this frame (1 A, 3 B, 5 A2, 7 B2), even = game thread prepares the next shot, 8 = done
extern volatile int g_photoStage;
extern char g_photoKey[128];
extern volatile int g_photoItem;      // 1 = item (no bottom fade)
void PortraitPresent(ID3D11Device* dev, ID3D11DeviceContext* ctx, IDXGISwapChain* sc);
void* PortraitFor(const std::string& key);   // ImTextureID or null (loads the cached file on first use)
bool PortraitHas(const std::string& key);    // in memory or on disk
void PortraitClear();                        // drop textures (files stay)
std::vector<std::string> PortraitKeys();     // every cached file (file name = key with odd chars as '_')
void PortraitDelete(const std::string& key); // file + texture
void ShotRequest();                          // build 15: screenshot of the next game frame
