#include "d3d_utils.h"

ID3D11Device* device = nullptr;
ID3D11DeviceContext* context = nullptr;
IDXGISwapChain* swapChain = nullptr;
ID3D11RenderTargetView* renderTargetView = nullptr;
ID3D11VertexShader* vertexShader = nullptr;
ID3D11PixelShader* pixelShader = nullptr;
ID3D11InputLayout* vertexLayout = nullptr;
ID3D11Buffer* vertexBuffer = nullptr;
ID3D11Buffer* indexBuffer = nullptr;
ID3D11Buffer* constantBufferWorld = nullptr;
ID3D11Buffer* constantBufferViewProjection = nullptr;

float cameraPitch = 0.0f;
float cameraYaw = 0.0f;
bool cameraUpdated = false;

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
        { DirectX::XMFLOAT3(-1.0f, 1.0f, -1.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
        { DirectX::XMFLOAT3(1.0f, 1.0f, -1.0f), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
        { DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f), DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
        { DirectX::XMFLOAT3(-1.0f, 1.0f, 1.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
        { DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
        { DirectX::XMFLOAT3(1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },
        { DirectX::XMFLOAT3(1.0f, -1.0f, 1.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { DirectX::XMFLOAT3(-1.0f, -1.0f, 1.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) },
    };

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * 8;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;
    hr = device->CreateBuffer(&bd, &initData, &vertexBuffer);
    if (FAILED(hr)) MessageBox(nullptr, L"CreateBuffer (Vertex) failed", L"Error", MB_OK);

    WORD indices[] = {
        3, 1, 0,
        2, 1, 3,
        0, 5, 4,
        1, 5, 0,
        3, 4, 7,
        0, 4, 3,
        1, 6, 5,
        2, 6, 1,
        2, 7, 6,
        3, 7, 2,
        6, 4, 5,
        7, 4, 6,
    };

    bd.ByteWidth = sizeof(WORD) * 36;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    initData.pSysMem = indices;
    hr = device->CreateBuffer(&bd, &initData, &indexBuffer);
    if (FAILED(hr)) MessageBox(nullptr, L"CreateBuffer (Index) failed", L"Error", MB_OK);

    bd.ByteWidth = sizeof(ConstantBufferWorld);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    hr = device->CreateBuffer(&bd, nullptr, &constantBufferWorld);
    if (FAILED(hr)) MessageBox(nullptr, L"CreateBuffer (World) failed", L"Error", MB_OK);

    bd.ByteWidth = sizeof(ConstantBufferViewProjection);
    hr = device->CreateBuffer(&bd, nullptr, &constantBufferViewProjection);
    if (FAILED(hr)) MessageBox(nullptr, L"CreateBuffer (ViewProjection) failed", L"Error", MB_OK);

    pFactory->Release();
    if (pSelectedAdapter) pSelectedAdapter->Release();
}

void RenderFrame() {
    static ULONGLONG timeStart = 0;
    ULONGLONG timeCur = GetTickCount64();
    if (timeStart == 0) timeStart = timeCur;
    float t = (timeCur - timeStart) / 1000.0f;

    DirectX::XMMATRIX world = DirectX::XMMatrixRotationY(t);

    static DirectX::XMVECTOR eye = DirectX::XMVectorSet(0.0f, 1.0f, -5.0f, 0.0f);
    static DirectX::XMVECTOR at = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    static DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    if (cameraUpdated) {
        DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(cameraPitch, cameraYaw, 0.0f);
        eye = DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(0.0f, 1.0f, -5.0f, 0.0f), rotationMatrix);
        at = DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotationMatrix);
        up = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotationMatrix);
        cameraUpdated = false;
    }

    DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(eye, at, up);
    DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2, 800.0f / 600.0f, 0.01f, 100.0f);

    ConstantBufferWorld cbWorld;
    cbWorld.mWorld = DirectX::XMMatrixTranspose(world);
    context->UpdateSubresource(constantBufferWorld, 0, nullptr, &cbWorld, 0, 0);

    ConstantBufferViewProjection cbViewProj;
    cbViewProj.mView = DirectX::XMMatrixTranspose(view);
    cbViewProj.mProjection = DirectX::XMMatrixTranspose(projection);
    context->UpdateSubresource(constantBufferViewProjection, 0, nullptr, &cbViewProj, 0, 0);

    FLOAT clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    context->ClearRenderTargetView(renderTargetView, clearColor);

    context->OMSetRenderTargets(1, &renderTargetView, nullptr);

    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
    context->IASetInputLayout(vertexLayout);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vertexShader, nullptr, 0);
    context->VSSetConstantBuffers(0, 1, &constantBufferWorld);
    context->VSSetConstantBuffers(1, 1, &constantBufferViewProjection);
    context->PSSetShader(pixelShader, nullptr, 0);

    context->DrawIndexed(36, 0, 0);

    swapChain->Present(0, 0);
}

void CleanupD3D() {
    if (constantBufferViewProjection) constantBufferViewProjection->Release();
    if (constantBufferWorld) constantBufferWorld->Release();
    if (indexBuffer) indexBuffer->Release();
    if (vertexBuffer) vertexBuffer->Release();
    if (vertexLayout) vertexLayout->Release();
    if (vertexShader) vertexShader->Release();
    if (pixelShader) pixelShader->Release();
    if (renderTargetView) renderTargetView->Release();
    if (swapChain) swapChain->Release();
    if (context) context->Release();
    if (device) device->Release();
}