#ifndef D3D_UTILS_H
#define D3D_UTILS_H

#include "main.h"
#include "shaders.h"

extern float cameraYaw;
extern float cameraPitch;
extern bool cameraUpdated;
extern DirectX::XMVECTOR cameraEye;
extern DirectX::XMVECTOR cameraAt;
extern DirectX::XMVECTOR cameraUp;

extern int lastMouseX;
extern int lastMouseY;

extern UINT windowWidth;
extern UINT windowHeight;

void InitD3D(HWND hWnd);
void ResizeD3D(UINT width, UINT height);
void RenderFrame();
void CleanupD3D();
void OnMouseMove(int x, int y, bool leftButtonDown);

ID3D11ShaderResourceView* CreateSolidColorTexture(float r, float g, float b, float a);

#endif
