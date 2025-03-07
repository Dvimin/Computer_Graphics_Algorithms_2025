#define UNICODE
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <assert.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

ID3D11Device* device = nullptr;
ID3D11DeviceContext* context = nullptr;
IDXGISwapChain* swapChain = nullptr;
ID3D11RenderTargetView* renderTargetView = nullptr;
ID3D11VertexShader* vertexShader = nullptr;
ID3D11PixelShader* pixelShader = nullptr;
ID3D11InputLayout* vertexLayout = nullptr;
ID3D11Buffer* vertexBuffer = nullptr;

const char* vertexShaderCode = R"(
struct VS_INPUT
{
    float4 Pos : POSITION;
    float4 Color : COLOR;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    output.Pos = input.Pos;
    output.Color = input.Color;
    return output;
}
)";

const char* pixelShaderCode = R"(
struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};

float4 main(PS_INPUT input) : SV_Target
{
    return input.Color;
}
)";

struct SimpleVertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT4 Color;
};

void ResizeD3D(UINT width, UINT height) {
    if (swapChain && width > 0 && height > 0) { 
        if (renderTargetView) {
            renderTargetView->Release();
            renderTargetView = nullptr;
        }
        context->OMSetRenderTargets(0, nullptr, nullptr);

        HRESULT hr = swapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_UNKNOWN, 0);
        if (FAILED(hr)) MessageBox(nullptr, L"ResizeBuffers failed", L"Error", MB_OK);

        ID3D11Texture2D* backBuffer = nullptr;
        hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
        if (SUCCEEDED(hr)) {
            hr = device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
            backBuffer->Release();
        }
        if (FAILED(hr)) MessageBox(nullptr, L"CreateRenderTargetView failed", L"Error", MB_OK);

        context->OMSetRenderTargets(1, &renderTargetView, nullptr);

        D3D11_VIEWPORT vp = { 0.0f, 0.0f, (FLOAT)width, (FLOAT)height, 0.0f, 1.0f };
        context->RSSetViewports(1, &vp);
    }
}

void InitD3D(HWND hWnd) {
    IDXGIFactory* pFactory = nullptr;
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);
    if (FAILED(hr)) MessageBox(nullptr, L"CreateDXGIFactory failed", L"Error", MB_OK);

    IDXGIAdapter* pSelectedAdapter = nullptr;
    UINT adapterIdx = 0;
    while (pFactory->EnumAdapters(adapterIdx, &pSelectedAdapter) != DXGI_ERROR_NOT_FOUND) {
        DXGI_ADAPTER_DESC desc;
        pSelectedAdapter->GetDesc(&desc);
        if (wcscmp(desc.Description, L"Microsoft Basic Render Driver") != 0) {
            break;
        }
        pSelectedAdapter->Release();
        adapterIdx++;
    }
    if (!pSelectedAdapter) MessageBox(nullptr, L"No suitable adapter found", L"Error", MB_OK);

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel;
    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
    hr = D3D11CreateDevice(pSelectedAdapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr,
        createDeviceFlags, levels, 1, D3D11_SDK_VERSION,
        &device, &featureLevel, &context);
    if (FAILED(hr)) MessageBox(nullptr, L"D3D11CreateDevice failed", L"Error", MB_OK);

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Width = 800;
    swapChainDesc.BufferDesc.Height = 600;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    if (!device) {
        MessageBox(nullptr, L"Device creation failed!", L"Error", MB_OK);
        return;
    }
    hr = pFactory->CreateSwapChain(device, &swapChainDesc, &swapChain);
    if (FAILED(hr)) MessageBox(nullptr, L"CreateSwapChain failed", L"Error", MB_OK);

    ID3D11Texture2D* backBuffer = nullptr;
    hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (SUCCEEDED(hr)) {
        hr = device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
        backBuffer->Release();
    }
    if (FAILED(hr)) MessageBox(nullptr, L"CreateRenderTargetView failed", L"Error", MB_OK);

    context->OMSetRenderTargets(1, &renderTargetView, nullptr);

    D3D11_VIEWPORT vp = { 0.0f, 0.0f, 800.0f, 600.0f, 0.0f, 1.0f };
    context->RSSetViewports(1, &vp);

    ID3DBlob* pVSBlob = nullptr;
    hr = D3DCompile(vertexShaderCode, strlen(vertexShaderCode), "vertexShader", nullptr, nullptr, "main", "vs_5_0", 0, 0, &pVSBlob, nullptr);
    if (FAILED(hr)) MessageBox(nullptr, L"Vertex shader compilation failed", L"Error", MB_OK);

    hr = device->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &vertexShader);
    if (FAILED(hr)) MessageBox(nullptr, L"CreateVertexShader failed", L"Error", MB_OK);

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    hr = device->CreateInputLayout(layout, 2, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &vertexLayout);
    if (FAILED(hr)) MessageBox(nullptr, L"CreateInputLayout failed", L"Error", MB_OK);
    pVSBlob->Release();

    ID3DBlob* pPSBlob = nullptr;
    hr = D3DCompile(pixelShaderCode, strlen(pixelShaderCode), "pixelShader", nullptr, nullptr, "main", "ps_5_0", 0, 0, &pPSBlob, nullptr);
    if (FAILED(hr)) MessageBox(nullptr, L"Pixel shader compilation failed", L"Error", MB_OK);

    hr = device->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &pixelShader);
    if (FAILED(hr)) MessageBox(nullptr, L"CreatePixelShader failed", L"Error", MB_OK);
    pPSBlob->Release();

    SimpleVertex vertices[] = {
        { DirectX::XMFLOAT3(0.0f, 0.5f, 0.5f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
        { DirectX::XMFLOAT3(0.5f, -0.5f, 0.5f), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
        { DirectX::XMFLOAT3(-0.5f, -0.5f, 0.5f), DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }
    };

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * 3;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;
    hr = device->CreateBuffer(&bd, &initData, &vertexBuffer);
    if (FAILED(hr)) MessageBox(nullptr, L"CreateBuffer failed", L"Error", MB_OK);

    pFactory->Release();
    if (pSelectedAdapter) pSelectedAdapter->Release();
}

void RenderFrame() {
    FLOAT clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    context->ClearRenderTargetView(renderTargetView, clearColor);

    context->OMSetRenderTargets(1, &renderTargetView, nullptr);

    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context->IASetInputLayout(vertexLayout);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vertexShader, nullptr, 0);
    context->PSSetShader(pixelShader, nullptr, 0);
    context->Draw(3, 0);

    swapChain->Present(1, 0);
}

void CleanupD3D() {
    if (vertexBuffer) vertexBuffer->Release();
    if (vertexLayout) vertexLayout->Release();
    if (vertexShader) vertexShader->Release();
    if (pixelShader) pixelShader->Release();
    if (renderTargetView) renderTargetView->Release();
    if (swapChain) swapChain->Release();
    if (context) context->Release();
    if (device) device->Release();
}

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"DirectXWindowClass";
    RegisterClass(&wc);

    RECT rc = { 0, 0, 800, 600 };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hWnd = CreateWindowEx(0, wc.lpszClassName, L"DirectX 11 Window",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    InitD3D(hWnd);

    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            RenderFrame();
        }
    }

    CleanupD3D();
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_SIZE:
        ResizeD3D(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}