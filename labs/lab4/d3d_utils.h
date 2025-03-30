#ifndef D3D_UTILS_H
#define D3D_UTILS_H

#include "main.h"
#include "shaders.h"

void InitD3D(HWND hWnd);
void ResizeD3D(UINT width, UINT height);
void RenderFrame();
void CleanupD3D();
void OnMouseMove(int x, int y, bool leftButtonDown);
#endif