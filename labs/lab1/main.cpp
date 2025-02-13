 
#define UNICODE
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <assert.h>

#pragma comment(lib, "d3d11.lib")

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// Глобальные переменные DirectX
ID3D11Device* device = nullptr;
ID3D11DeviceContext* context = nullptr;
IDXGISwapChain* swapChain = nullptr;
ID3D11RenderTargetView* renderTargetView = nullptr;

// Функция изменения размера окна
void ResizeD3D(UINT width, UINT height) {
    if (swapChain) {
        // Освобождаем старый рендер-таргет
        if (renderTargetView) renderTargetView->Release();
        context->OMSetRenderTargets(0, nullptr, nullptr);

        HRESULT hr = swapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_UNKNOWN, 0);
        assert(SUCCEEDED(hr));

        // Получаем новый back buffer
        ID3D11Texture2D* backBuffer = nullptr;
        swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
        device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
        backBuffer->Release();

        // Устанавливаем новый рендер-таргет
        context->OMSetRenderTargets(1, &renderTargetView, nullptr);
    }
}

// Инициализация DirectX 11
void InitD3D(HWND hWnd) {
    IDXGIFactory* pFactory = nullptr;
    CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);

    IDXGIAdapter* pSelectedAdapter = nullptr;
    IDXGIAdapter* pAdapter = nullptr;
    UINT adapterIdx = 0;
    while (pFactory->EnumAdapters(adapterIdx, &pAdapter) != DXGI_ERROR_NOT_FOUND) {
        DXGI_ADAPTER_DESC desc;
        pAdapter->GetDesc(&desc);
        if (wcscmp(desc.Description, L"Microsoft Basic Render Driver") != 0) {
            pSelectedAdapter = pAdapter;
            break;
        }
        pAdapter->Release();
        adapterIdx++;
    }
    assert(pSelectedAdapter != nullptr);

    // Настройка флагов
    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel;
    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };

    HRESULT result = D3D11CreateDevice(
        pSelectedAdapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr,
        createDeviceFlags, levels, 1, D3D11_SDK_VERSION,
        &device, &featureLevel, &context
    );
    assert(SUCCEEDED(result));

    // Создание Swap Chain
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Width = 800;
    swapChainDesc.BufferDesc.Height = 600;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    result = pFactory->CreateSwapChain(device, &swapChainDesc, &swapChain);
    assert(SUCCEEDED(result));

    // Запрашиваем back buffer и создаём render target
    ID3D11Texture2D* backBuffer = nullptr;
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
    backBuffer->Release();

    // Устанавливаем рендер-таргет
    context->OMSetRenderTargets(1, &renderTargetView, nullptr);
}

// Отрисовка кадра
void RenderFrame() {
    FLOAT clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    context->ClearRenderTargetView(renderTargetView, clearColor);
    swapChain->Present(1, 0);
}

// Очистка ресурсов DirectX
void CleanupD3D() {
    if (renderTargetView) renderTargetView->Release();
    if (swapChain) swapChain->Release();
    if (context) context->Release();
    if (device) device->Release();
}

// Главная функция приложения
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
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

// Обработчик событий окна
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
