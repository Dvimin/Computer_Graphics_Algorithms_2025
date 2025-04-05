#include "d3d_utils.h"
#include "DDSTextureLoader.h"

using namespace DirectX;

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
ID3D11ShaderResourceView* textureView = nullptr;
ID3D11SamplerState* samplerState = nullptr;
ID3D11ShaderResourceView* skyboxTexture = nullptr;
ID3D11VertexShader* skyboxVertexShader = nullptr;
ID3D11PixelShader* skyboxPixelShader = nullptr;
ID3D11InputLayout* skyboxInputLayout = nullptr;
ID3D11Buffer* skyboxVertexBuffer = nullptr;
ID3D11Buffer* skyboxIndexBuffer = nullptr;
ID3D11DepthStencilView* depthStencilView = nullptr;
ID3D11Texture2D* depthStencilBuffer = nullptr;
ID3D11DepthStencilState* noDepthState = nullptr;
ID3D11RasterizerState* rasterState = nullptr;

UINT windowWidth = 800;
UINT windowHeight = 600;

float cameraPitch = 0.0f;
float cameraYaw = 0.0f;
bool cameraUpdated = false;

DirectX::XMVECTOR cameraEye = DirectX::XMVectorSet(0.0f, 1.0f, -5.0f, 0.0f);
DirectX::XMVECTOR cameraAt = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
DirectX::XMVECTOR cameraUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

void ResizeD3D(UINT width, UINT height) {
    if (swapChain && width > 0 && height > 0) {
        if (renderTargetView) {
            renderTargetView->Release();
            renderTargetView = nullptr;
        }
        if (depthStencilView) {
            depthStencilView->Release();
            depthStencilView = nullptr;
        }
        if (depthStencilBuffer) {
            depthStencilBuffer->Release();
            depthStencilBuffer = nullptr;
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

        D3D11_TEXTURE2D_DESC descDepth = {};
        descDepth.Width = width;
        descDepth.Height = height;
        descDepth.MipLevels = 1;
        descDepth.ArraySize = 1;
        descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        descDepth.SampleDesc.Count = 1;
        descDepth.Usage = D3D11_USAGE_DEFAULT;
        descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        hr = device->CreateTexture2D(&descDepth, nullptr, &depthStencilBuffer);
        if (FAILED(hr)) MessageBox(nullptr, L"Failed to create depth texture", L"Error", MB_OK);

        if (depthStencilBuffer) {
            D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
            descDSV.Format = descDepth.Format;
            descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

            hr = device->CreateDepthStencilView(depthStencilBuffer, &descDSV, &depthStencilView);
            if (FAILED(hr)) {
                MessageBox(nullptr, L"Failed to create depth stencil view", L"Error", MB_OK);
            }
        }
        else {
            MessageBox(nullptr, L"Depth stencil buffer is null", L"Error", MB_OK);
        }


        context->OMSetRenderTargets(1, &renderTargetView, depthStencilView);

        D3D11_VIEWPORT vp = { 0.0f, 0.0f, (FLOAT)width, (FLOAT)height, 0.0f, 1.0f };
        context->RSSetViewports(1, &vp);

        windowWidth = width;
        windowHeight = height;
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



    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;  
    rasterDesc.FrontCounterClockwise = false;
    rasterDesc.DepthClipEnable = true;      
    rasterDesc.ScissorEnable = false;       
    rasterDesc.MultisampleEnable = false; 


    hr = device->CreateRasterizerState(&rasterDesc, &rasterState);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Failed to create rasterizer state", L"Error", MB_OK);
    }
    context->RSSetState(rasterState);
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

    D3D11_TEXTURE2D_DESC descDepth = {};
    descDepth.Width = 800;
    descDepth.Height = 600;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    hr = device->CreateTexture2D(&descDepth, nullptr, &depthStencilBuffer);
    if (FAILED(hr)) MessageBox(nullptr, L"Failed to create depth texture", L"Error", MB_OK);

    if (depthStencilBuffer) {
        D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
        descDSV.Format = descDepth.Format;
        descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        descDSV.Texture2D.MipSlice = 0;

        hr = device->CreateDepthStencilView(depthStencilBuffer, &descDSV, &depthStencilView);
        if (FAILED(hr)) {
            MessageBox(nullptr, L"Failed to create depth stencil view", L"Error", MB_OK);
        }
    }
    else {
        MessageBox(nullptr, L"Depth stencil buffer is null", L"Error", MB_OK);
    }

    context->OMSetRenderTargets(1, &renderTargetView, depthStencilView);

    D3D11_VIEWPORT vp = { 0.0f, 0.0f, 800.0f, 600.0f, 0.0f, 1.0f };
    context->RSSetViewports(1, &vp);

    windowWidth = 800;
    windowHeight = 600;

    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = FALSE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    dsDesc.StencilEnable = FALSE;
    hr = device->CreateDepthStencilState(&dsDesc, &noDepthState);
    if (FAILED(hr)) MessageBox(nullptr, L"Failed to create noDepthState", L"Error", MB_OK);

    // Шейдеры для куба
    ID3DBlob* pVSBlob = nullptr;
    hr = D3DCompile(vertexShaderCode, strlen(vertexShaderCode), "vertexShader", nullptr, nullptr, "main", "vs_5_0", 0, 0, &pVSBlob, nullptr);
    if (FAILED(hr)) MessageBox(nullptr, L"Vertex shader compilation failed", L"Error", MB_OK);

    hr = device->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &vertexShader);
    if (FAILED(hr)) MessageBox(nullptr, L"CreateVertexShader failed", L"Error", MB_OK);

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = device->CreateInputLayout(layout, 2, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &vertexLayout);
    if (FAILED(hr)) MessageBox(nullptr, L"CreateInputLayout failed", L"Error", MB_OK);
    pVSBlob->Release();

    ID3DBlob* pPSBlob = nullptr;
    hr = D3DCompile(pixelShaderCode, strlen(pixelShaderCode), "pixelShader", nullptr, nullptr, "main", "ps_5_0", 0, 0, &pPSBlob, nullptr);
    if (FAILED(hr)) MessageBox(nullptr, L"Pixel shader compilation failed", L"Error", MB_OK);

    hr = device->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &pixelShader);
    if (FAILED(hr)) MessageBox(nullptr, L"Create Pixel Shader failed", L"Error", MB_OK);
    pPSBlob->Release();

    // Шейдеры для skybox’а
    ID3DBlob* vsSkyboxBlob = nullptr;
    hr = D3DCompile(skyboxVertexShaderCode, strlen(skyboxVertexShaderCode), "skyboxVS", nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsSkyboxBlob, nullptr);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Skybox VS compile failed", L"Error", MB_OK);
        return;
    }
    hr = device->CreateVertexShader(vsSkyboxBlob->GetBufferPointer(), vsSkyboxBlob->GetBufferSize(), nullptr, &skyboxVertexShader);

    D3D11_INPUT_ELEMENT_DESC skyboxLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = device->CreateInputLayout(skyboxLayout, 1, vsSkyboxBlob->GetBufferPointer(), vsSkyboxBlob->GetBufferSize(), &skyboxInputLayout);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Create Input Layout failed (skybox)", L"Error", MB_OK);
        return;
    }
    vsSkyboxBlob->Release();

    ID3DBlob* psSkyboxBlob = nullptr;
    hr = D3DCompile(skyboxPixelShaderCode, strlen(skyboxPixelShaderCode), "skyboxPS", nullptr, nullptr, "main", "ps_5_0", 0, 0, &psSkyboxBlob, nullptr);
    hr = device->CreatePixelShader(psSkyboxBlob->GetBufferPointer(), psSkyboxBlob->GetBufferSize(), nullptr, &skyboxPixelShader);
    psSkyboxBlob->Release();

    SimpleVertex vertices[] = {
        { { -1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f } },
        { {  1.0f,  1.0f, -1.0f }, { 1.0f, 0.0f } },
        { {  1.0f,  1.0f,  1.0f }, { 1.0f, 1.0f } },
        { { -1.0f,  1.0f,  1.0f }, { 0.0f, 1.0f } },

        { { -1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f } },
        { {  1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f } },
        { {  1.0f, -1.0f,  1.0f }, { 1.0f, 1.0f } },
        { { -1.0f, -1.0f,  1.0f }, { 0.0f, 1.0f } },

        { { -1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f } },
        { {  1.0f,  1.0f, -1.0f }, { 1.0f, 0.0f } },
        { {  1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f } },
        { { -1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f } },

        { {  1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f } },
        { { -1.0f,  1.0f,  1.0f }, { 1.0f, 0.0f } },
        { { -1.0f, -1.0f,  1.0f }, { 1.0f, 1.0f } },
        { {  1.0f, -1.0f,  1.0f }, { 0.0f, 1.0f } },

        { { -1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f } },
        { { -1.0f,  1.0f, -1.0f }, { 1.0f, 0.0f } },
        { { -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f } },
        { { -1.0f, -1.0f,  1.0f }, { 0.0f, 1.0f } },

        { {  1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f } },
        { {  1.0f,  1.0f,  1.0f }, { 1.0f, 0.0f } },
        { {  1.0f, -1.0f,  1.0f }, { 1.0f, 1.0f } },
        { {  1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f } },
    };


    float skyboxVertices[] = {
        -10, -10, -10,  10, -10, -10,  10,  10, -10,  -10,  10, -10,
        -10, -10,  10,  10, -10,  10,  10,  10,  10,  -10,  10,  10
    };

    D3D11_BUFFER_DESC vbSkyDesc = {};
    vbSkyDesc.Usage = D3D11_USAGE_DEFAULT;
    vbSkyDesc.ByteWidth = sizeof(float) * 24;
    vbSkyDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbSkyData = {};
    vbSkyData.pSysMem = skyboxVertices;
    hr = device->CreateBuffer(&vbSkyDesc, &vbSkyData, &skyboxVertexBuffer);

    WORD skyboxIndices[] = {
        0, 1, 2, 0, 2, 3,     
        4, 6, 5, 4, 7, 6,     
        4, 5, 1, 4, 1, 0,     
        3, 2, 6, 3, 6, 7,     
        1, 5, 6, 1, 6, 2,     
        4, 0, 3, 4, 3, 7     
    };

    D3D11_BUFFER_DESC ibSkyDesc = {};
    ibSkyDesc.Usage = D3D11_USAGE_DEFAULT;
    ibSkyDesc.ByteWidth = sizeof(skyboxIndices);
    ibSkyDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA ibSkyData = {};
    ibSkyData.pSysMem = skyboxIndices;

    hr = device->CreateBuffer(&ibSkyDesc, &ibSkyData, &skyboxIndexBuffer);
    if (FAILED(hr)) MessageBox(nullptr, L"Failed to create skybox index buffer", L"Error", MB_OK);

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * 24; 
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;
    hr = device->CreateBuffer(&bd, &initData, &vertexBuffer);
    if (FAILED(hr)) MessageBox(nullptr, L"CreateBuffer (Vertex) failed", L"Error", MB_OK);

    WORD indices[] = {
        0, 2, 1,  0, 3, 2,
        4, 6, 5,  4, 7, 6,
        8, 9, 10,  8, 10, 11,
        12, 14, 13,  12, 15, 14,
        16, 17, 18,  16, 18, 19,
        20, 22, 21,  20, 23, 22,
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

    hr = DirectX::CreateDDSTextureFromFile(device, L"../../labs/lab4/bricks.dds", nullptr, &textureView);
    if (FAILED(hr)) MessageBox(nullptr, L"Failed to load texture", L"Error", MB_OK);

    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = device->CreateSamplerState(&sampDesc, &samplerState);
    if (FAILED(hr)) MessageBox(nullptr, L"Failed to create sampler", L"Error", MB_OK);

    hr = DirectX::CreateDDSTextureFromFile(device, L"../../labs/lab4/skybox.dds", nullptr, &skyboxTexture);
    if (FAILED(hr)) MessageBox(nullptr, L"Failed to load cubemap", L"Error", MB_OK);

    pFactory->Release();
    if (pSelectedAdapter) pSelectedAdapter->Release();
}

void OnMouseMove(int x, int y, bool leftButtonDown) {
    static int lastX = x;
    static int lastY = y;

    if (leftButtonDown) {
        float dx = (x - lastX) * 0.01f;
        float dy = (y - lastY) * 0.01f;

        cameraYaw += dx;
        cameraPitch += dy;

        if (cameraPitch > DirectX::XM_PIDIV2) cameraPitch = DirectX::XM_PIDIV2;
        if (cameraPitch < -DirectX::XM_PIDIV2) cameraPitch = -DirectX::XM_PIDIV2;

        cameraUpdated = true;
    }

    lastX = x;
    lastY = y;
}


void RenderFrame() {
    static ULONGLONG timeStart = 0;
    ULONGLONG timeCur = GetTickCount64();
    if (timeStart == 0) timeStart = timeCur;
    float t = (timeCur - timeStart) / 1000.0f;

    DirectX::XMMATRIX world = XMMatrixRotationY(t);



    if (cameraUpdated) {
        DirectX::XMMATRIX rot = DirectX::XMMatrixRotationRollPitchYaw(cameraPitch, cameraYaw, 0.0f);
        cameraEye = DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(0.0f, 1.0f, -5.0f, 0.0f), rot);
        cameraAt = DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rot);
        cameraUp = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rot);
        cameraUpdated = false;
    }

    DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(cameraEye, cameraAt, cameraUp);
    DirectX::XMMATRIX viewNoTrans = view;
    viewNoTrans.r[3] = XMVectorSet(0, 0, 0, 1);


    DirectX::XMMATRIX projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, (float)windowWidth / (float)windowHeight, 0.01f, 100.0f);

    FLOAT clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    context->ClearRenderTargetView(renderTargetView, clearColor);
    context->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTargetView, depthStencilView);


    ConstantBufferViewProjection cbSkyboxViewProj;
    cbSkyboxViewProj.mView = XMMatrixTranspose(viewNoTrans);
    cbSkyboxViewProj.mProjection = XMMatrixTranspose(projection);
    context->UpdateSubresource(constantBufferViewProjection, 0, nullptr, &cbSkyboxViewProj, 0, 0);

    UINT strideSky = sizeof(float) * 3;
    UINT offsetSky = 0;
    context->IASetInputLayout(skyboxInputLayout);
    context->IASetVertexBuffers(0, 1, &skyboxVertexBuffer, &strideSky, &offsetSky);
    context->IASetIndexBuffer(skyboxIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(skyboxVertexShader, nullptr, 0);
    context->VSSetConstantBuffers(0, 1, &constantBufferViewProjection);
    context->PSSetShader(skyboxPixelShader, nullptr, 0);
    context->PSSetShaderResources(0, 1, &skyboxTexture);
    context->PSSetSamplers(0, 1, &samplerState);
    context->OMSetDepthStencilState(noDepthState, 0); 
    context->DrawIndexed(36, 0, 0);

    ConstantBufferWorld cbWorld;
    cbWorld.mWorld = XMMatrixTranspose(world);
    context->UpdateSubresource(constantBufferWorld, 0, nullptr, &cbWorld, 0, 0);

    ConstantBufferViewProjection cbViewProj;
    cbViewProj.mView = XMMatrixTranspose(view);
    cbViewProj.mProjection = XMMatrixTranspose(projection);
    context->UpdateSubresource(constantBufferViewProjection, 0, nullptr, &cbViewProj, 0, 0);

    context->OMSetDepthStencilState(nullptr, 0); 
    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    context->IASetInputLayout(vertexLayout);
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vertexShader, nullptr, 0);
    context->VSSetConstantBuffers(0, 1, &constantBufferWorld);
    context->VSSetConstantBuffers(1, 1, &constantBufferViewProjection);
    context->PSSetShader(pixelShader, nullptr, 0);
    context->PSSetShaderResources(0, 1, &textureView);
    context->PSSetSamplers(0, 1, &samplerState);

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
    if (samplerState) samplerState->Release();
    if (textureView) textureView->Release();
    if (skyboxTexture) skyboxTexture->Release();
    if (skyboxIndexBuffer) skyboxIndexBuffer->Release();
    if (skyboxVertexBuffer) skyboxVertexBuffer->Release();
    if (skyboxInputLayout) skyboxInputLayout->Release();
    if (skyboxVertexShader) skyboxVertexShader->Release();
    if (skyboxPixelShader) skyboxPixelShader->Release();
    if (noDepthState) noDepthState->Release();
    if (depthStencilView) depthStencilView->Release();
    if (depthStencilBuffer) depthStencilBuffer->Release();
    if (rasterState) rasterState->Release();
}