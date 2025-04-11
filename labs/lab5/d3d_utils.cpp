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
ID3D11DepthStencilState* reversedDepthState = nullptr;
ID3D11BlendState* transBlendState = nullptr;
ID3D11ShaderResourceView* transparentTextureView = nullptr;
ID3D11Buffer* quadVertexBuffer = nullptr;
ID3D11Buffer* quadIndexBuffer = nullptr;
ID3D11ShaderResourceView* transparentTextureView2 = nullptr;
ID3D11Buffer* quadVertexBuffer2 = nullptr;
ID3D11Buffer* quadIndexBuffer2 = nullptr;

UINT windowWidth = 800;
UINT windowHeight = 600;

const float MOUSE_SENSITIVITY = 0.005f;
float radius = 6.0f;
float initYaw = DirectX::XM_PI / 4.0f;
float initPitch = DirectX::XM_PIDIV4 / 2.0f; 

float eyeX = radius * sinf(initYaw) * cosf(initPitch);
float eyeY = radius * sinf(initPitch);
float eyeZ = -4.0f - radius * cosf(initYaw) * cosf(initPitch);

DirectX::XMVECTOR cameraEye = XMVectorSet(eyeX, eyeY, eyeZ, 0.0f);
DirectX::XMVECTOR cameraAt = XMVectorSet(0.0f, 0.0f, -4.0f, 0.0f);
DirectX::XMVECTOR cameraUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

float cameraYaw = initYaw;
float cameraPitch = initPitch;
bool cameraUpdated = true; 

ID3D11ShaderResourceView* CreateSolidColorTexture(float r, float g, float b, float a) {
    UINT color = ((UINT)(a * 255) << 24) |
        ((UINT)(r * 255) << 16) |
        ((UINT)(g * 255) << 8) |
        ((UINT)(b * 255));

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = 1;
    texDesc.Height = 1;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_IMMUTABLE;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = &color;
    initData.SysMemPitch = sizeof(UINT);

    ID3D11Texture2D* tex = nullptr;
    HRESULT hr = device->CreateTexture2D(&texDesc, &initData, &tex);
    if (FAILED(hr)) return nullptr;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    ID3D11ShaderResourceView* srv = nullptr;
    hr = device->CreateShaderResourceView(tex, &srvDesc, &srv);
    tex->Release();

    return SUCCEEDED(hr) ? srv : nullptr;
}

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
        descDepth.Format = DXGI_FORMAT_D32_FLOAT;
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
    descDepth.Format = DXGI_FORMAT_D32_FLOAT;
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
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    dsDesc.StencilEnable = FALSE;

    hr = device->CreateDepthStencilState(&dsDesc, &reversedDepthState);
    if (FAILED(hr)) MessageBox(nullptr, L"Failed to create reversedDepthState", L"Error", MB_OK);

    context->OMSetDepthStencilState(reversedDepthState, 0);

    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    dsDesc.StencilEnable = FALSE;

    hr = device->CreateDepthStencilState(&dsDesc, &noDepthState);
    if (FAILED(hr)) MessageBox(nullptr, L"Failed to create noDepthState", L"Error", MB_OK);

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = device->CreateBlendState(&blendDesc, &transBlendState);
    if (FAILED(hr)) MessageBox(nullptr, L"Failed to create blend state", L"Error", MB_OK);


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

    SimpleVertex quadVertices[] = {
    { { 1.5f,  2.0f, -2.0f }, { 0.0f, 0.0f } }, 
    { { 1.5f,  2.0f, -6.0f }, { 1.0f, 0.0f } }, 
    { { 1.5f, -2.0f, -6.0f }, { 1.0f, 1.0f } }, 
    { { 1.5f, -2.0f, -2.0f }, { 0.0f, 1.0f } }, 
    };

    D3D11_BUFFER_DESC quadVBDesc = {};
    quadVBDesc.Usage = D3D11_USAGE_DEFAULT;
    quadVBDesc.ByteWidth = sizeof(SimpleVertex) * 4;
    quadVBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA quadVBData = {};
    quadVBData.pSysMem = quadVertices;
    hr = device->CreateBuffer(&quadVBDesc, &quadVBData, &quadVertexBuffer);
    if (FAILED(hr)) MessageBox(nullptr, L"CreateBuffer (Quad Vertex) failed", L"Error", MB_OK);

    SimpleVertex quadVertices2[] = {
    { { 2.0f,  2.0f, -2.0f }, { 0.0f, 0.0f } }, 
    { { 2.0f,  2.0f, -6.0f }, { 1.0f, 0.0f } },
    { { 2.0f, -2.0f, -6.0f }, { 1.0f, 1.0f } },
    { { 2.0f, -2.0f, -2.0f }, { 0.0f, 1.0f } }, 
    };

    D3D11_BUFFER_DESC quadVBDesc2 = {};
    quadVBDesc2.Usage = D3D11_USAGE_DEFAULT;
    quadVBDesc2.ByteWidth = sizeof(quadVertices2);
    quadVBDesc2.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA quadVBData2 = { quadVertices2 };
    hr = device->CreateBuffer(&quadVBDesc2, &quadVBData2, &quadVertexBuffer2);
    if (FAILED(hr)) MessageBox(nullptr, L"CreateBuffer (Quad Vertex 2) failed", L"Error", MB_OK);

    WORD quadIndices2[] = { 0, 1, 2,  0, 2, 3 };

    D3D11_BUFFER_DESC quadIBDesc2 = {};
    quadIBDesc2.Usage = D3D11_USAGE_DEFAULT;
    quadIBDesc2.ByteWidth = sizeof(quadIndices2);
    quadIBDesc2.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA quadIBData2 = { quadIndices2 };
    hr = device->CreateBuffer(&quadIBDesc2, &quadIBData2, &quadIndexBuffer2);
    if (FAILED(hr)) MessageBox(nullptr, L"CreateBuffer (Quad Index 2) failed", L"Error", MB_OK);

    WORD quadIndices[] = { 0, 1, 2,  0, 2, 3 };

    D3D11_BUFFER_DESC quadIBDesc = {};
    quadIBDesc.Usage = D3D11_USAGE_DEFAULT;
    quadIBDesc.ByteWidth = sizeof(WORD) * 6;
    quadIBDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA quadIBData = {};
    quadIBData.pSysMem = quadIndices;
    hr = device->CreateBuffer(&quadIBDesc, &quadIBData, &quadIndexBuffer);
    if (FAILED(hr)) MessageBox(nullptr, L"CreateBuffer (Quad Index) failed", L"Error", MB_OK);

    bd.ByteWidth = sizeof(ConstantBufferViewProjection);
    hr = device->CreateBuffer(&bd, nullptr, &constantBufferViewProjection);
    if (FAILED(hr)) MessageBox(nullptr, L"CreateBuffer (ViewProjection) failed", L"Error", MB_OK);

    hr = DirectX::CreateDDSTextureFromFile(device, L"./labs/lab5/cat2.dds", nullptr, &textureView);
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

    hr = DirectX::CreateDDSTextureFromFile(device, L"./labs/lab5/sky2.dds", nullptr, &skyboxTexture);
    if (FAILED(hr)) MessageBox(nullptr, L"Failed to load cubemap", L"Error", MB_OK);

    transparentTextureView = CreateSolidColorTexture(1.0f, 0.0f, 0.0f, 0.5f);
    transparentTextureView2 = CreateSolidColorTexture(0.0f, 0.0f, 1.0f, 0.5f);

    pFactory->Release();
    if (pSelectedAdapter) pSelectedAdapter->Release();
}

void OnMouseMove(int x, int y, bool leftButtonDown) {
    if (leftButtonDown) {
        float dx = (x - lastMouseX) * MOUSE_SENSITIVITY;
        float dy = (y - lastMouseY) * MOUSE_SENSITIVITY;

        cameraYaw += dx;
        cameraPitch += dy;

        if (cameraPitch > DirectX::XM_PIDIV2) cameraPitch = DirectX::XM_PIDIV2;
        if (cameraPitch < -DirectX::XM_PIDIV2) cameraPitch = -DirectX::XM_PIDIV2;

        cameraUpdated = true;
    }

    lastMouseX = x;
    lastMouseY = y;
}

void RenderFrame() {
    static ULONGLONG timeStart = 0;
    ULONGLONG timeCur = GetTickCount64();
    if (timeStart == 0) timeStart = timeCur;
    float t = (timeCur - timeStart) / 1000.0f;

    if (cameraUpdated) {
        cameraAt = DirectX::XMVectorSet(0.0f, 0.0f, -4.0f, 0.0f);
        float radius = 6.0f;
        float eyeX = radius * sinf(cameraYaw) * cosf(cameraPitch);
        float eyeY = radius * sinf(cameraPitch);
        float eyeZ = -4.0f - radius * cosf(cameraYaw) * cosf(cameraPitch);
        cameraEye = DirectX::XMVectorSet(eyeX, eyeY, eyeZ, 0.0f);
        cameraUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        cameraUpdated = false;
    }

    DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(cameraEye, cameraAt, cameraUp);
    DirectX::XMMATRIX viewNoTrans = view;
    viewNoTrans.r[3] = XMVectorSet(0, 0, 0, 1);
    DirectX::XMMATRIX projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, (float)windowWidth / windowHeight, 0.1f, 100.0f);


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

    ConstantBufferViewProjection cbViewProj;
    cbViewProj.mView = XMMatrixTranspose(view);
    cbViewProj.mProjection = XMMatrixTranspose(projection);
    context->UpdateSubresource(constantBufferViewProjection, 0, nullptr, &cbViewProj, 0, 0);

    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    context->IASetInputLayout(vertexLayout);
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vertexShader, nullptr, 0);
    context->VSSetConstantBuffers(1, 1, &constantBufferViewProjection);
    context->PSSetShader(pixelShader, nullptr, 0);
    context->PSSetShaderResources(0, 1, &textureView);
    context->PSSetSamplers(0, 1, &samplerState);
    context->OMSetDepthStencilState(reversedDepthState, 0);

    DirectX::XMMATRIX world = XMMatrixRotationY(t) * XMMatrixTranslation(0.0f, 0.0f, -4.0f);
    ConstantBufferWorld cbWorld;
    cbWorld.mWorld = XMMatrixTranspose(world);
    context->UpdateSubresource(constantBufferWorld, 0, nullptr, &cbWorld, 0, 0);
    context->VSSetConstantBuffers(0, 1, &constantBufferWorld);
    context->DrawIndexed(36, 0, 0);

    DirectX::XMMATRIX world2 = XMMatrixTranslation(4.0f, 0.0f, -4.0f);
    cbWorld.mWorld = XMMatrixTranspose(world2);
    context->UpdateSubresource(constantBufferWorld, 0, nullptr, &cbWorld, 0, 0);
    context->DrawIndexed(36, 0, 0);


    XMVECTOR redQuadPos = XMVectorSet(1.5f, 0.0f, -4.0f, 1.0f);  
    XMVECTOR blueQuadPos = XMVectorSet(2.0f, 0.0f, -4.0f, 1.0f); 

    float distToRed = XMVectorGetX(XMVector3Length(XMVectorSubtract(redQuadPos, cameraEye)));
    float distToBlue = XMVectorGetX(XMVector3Length(XMVectorSubtract(blueQuadPos, cameraEye)));

    if (distToRed > distToBlue) {
        context->IASetVertexBuffers(0, 1, &quadVertexBuffer, &stride, &offset);
        context->IASetIndexBuffer(quadIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
        DirectX::XMMATRIX world3 = XMMatrixIdentity();
        cbWorld.mWorld = XMMatrixTranspose(world3);
        context->UpdateSubresource(constantBufferWorld, 0, nullptr, &cbWorld, 0, 0);
        context->OMSetBlendState(transBlendState, nullptr, 0xFFFFFFFF);
        context->OMSetDepthStencilState(noDepthState, 0);
        context->PSSetShaderResources(0, 1, &transparentTextureView);
        context->DrawIndexed(6, 0, 0);

        context->IASetVertexBuffers(0, 1, &quadVertexBuffer2, &stride, &offset);
        context->IASetIndexBuffer(quadIndexBuffer2, DXGI_FORMAT_R16_UINT, 0);
        DirectX::XMMATRIX world4 = XMMatrixIdentity();
        cbWorld.mWorld = XMMatrixTranspose(world4);
        context->UpdateSubresource(constantBufferWorld, 0, nullptr, &cbWorld, 0, 0);
        context->OMSetBlendState(transBlendState, nullptr, 0xFFFFFFFF);
        context->OMSetDepthStencilState(noDepthState, 0);
        context->PSSetShaderResources(0, 1, &transparentTextureView2);
        context->DrawIndexed(6, 0, 0);
    }
    else {
        context->IASetVertexBuffers(0, 1, &quadVertexBuffer2, &stride, &offset);
        context->IASetIndexBuffer(quadIndexBuffer2, DXGI_FORMAT_R16_UINT, 0);
        DirectX::XMMATRIX world4 = XMMatrixIdentity();
        cbWorld.mWorld = XMMatrixTranspose(world4);
        context->UpdateSubresource(constantBufferWorld, 0, nullptr, &cbWorld, 0, 0);
        context->OMSetBlendState(transBlendState, nullptr, 0xFFFFFFFF);
        context->OMSetDepthStencilState(noDepthState, 0);
        context->PSSetShaderResources(0, 1, &transparentTextureView2);
        context->DrawIndexed(6, 0, 0);

        context->IASetVertexBuffers(0, 1, &quadVertexBuffer, &stride, &offset);
        context->IASetIndexBuffer(quadIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
        DirectX::XMMATRIX world3 = XMMatrixIdentity();
        cbWorld.mWorld = XMMatrixTranspose(world3);
        context->UpdateSubresource(constantBufferWorld, 0, nullptr, &cbWorld, 0, 0);
        context->OMSetBlendState(transBlendState, nullptr, 0xFFFFFFFF);
        context->OMSetDepthStencilState(noDepthState, 0);
        context->PSSetShaderResources(0, 1, &transparentTextureView);
        context->DrawIndexed(6, 0, 0);
    }
    context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(reversedDepthState, 0);

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
    if (reversedDepthState) reversedDepthState->Release();
    if (noDepthState) noDepthState->Release();
    if (depthStencilView) depthStencilView->Release();
    if (depthStencilBuffer) depthStencilBuffer->Release();
    if (rasterState) rasterState->Release();
    if (transBlendState) transBlendState->Release();
    if (transparentTextureView) transparentTextureView->Release();
    if (quadVertexBuffer) quadVertexBuffer->Release();
    if (quadIndexBuffer) quadIndexBuffer->Release();
    if (transparentTextureView2) transparentTextureView2->Release();
    if (quadVertexBuffer2) quadVertexBuffer2->Release();
    if (quadIndexBuffer2) quadIndexBuffer2->Release();
}