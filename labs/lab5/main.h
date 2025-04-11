#ifndef MAIN_H
#define MAIN_H

#include <windowsx.h>
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "DirectXTK.lib")

extern ID3D11Device* device;
extern ID3D11DeviceContext* context;
extern IDXGISwapChain* swapChain;

extern ID3D11RenderTargetView* renderTargetView;
extern ID3D11Texture2D* depthStencilBuffer;
extern ID3D11DepthStencilView* depthStencilView;

extern ID3D11VertexShader* vertexShader;
extern ID3D11PixelShader* pixelShader;
extern ID3D11InputLayout* vertexLayout;
extern ID3D11VertexShader* skyboxVertexShader;
extern ID3D11PixelShader* skyboxPixelShader;
extern ID3D11InputLayout* skyboxInputLayout;

extern ID3D11Buffer* vertexBuffer;
extern ID3D11Buffer* indexBuffer;
extern ID3D11Buffer* constantBufferWorld;
extern ID3D11Buffer* constantBufferViewProjection;
extern ID3D11Buffer* skyboxVertexBuffer;
extern ID3D11Buffer* skyboxIndexBuffer;
extern ID3D11Buffer* quadVertexBuffer;
extern ID3D11Buffer* quadIndexBuffer;
extern ID3D11Buffer* quadVertexBuffer2;
extern ID3D11Buffer* quadIndexBuffer2;

extern ID3D11ShaderResourceView* textureView;
extern ID3D11ShaderResourceView* skyboxTexture;
extern ID3D11ShaderResourceView* transparentTextureView;
extern ID3D11ShaderResourceView* transparentTextureView2;
extern ID3D11SamplerState* samplerState;

extern ID3D11DepthStencilState* noDepthState;
extern ID3D11DepthStencilState* reversedDepthState;
extern ID3D11BlendState* transBlendState;
extern ID3D11RasterizerState* rasterState;

extern DirectX::XMVECTOR cameraEye;
extern DirectX::XMVECTOR cameraAt;
extern DirectX::XMVECTOR cameraUp;
extern float cameraPitch;
extern float cameraYaw;
extern bool cameraUpdated;

extern bool isMouseCaptured;
extern int lastMouseX, lastMouseY;
extern int windowCenterX, windowCenterY;
extern UINT windowWidth, windowHeight;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#endif
