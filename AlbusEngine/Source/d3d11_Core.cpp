#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11async.h>
#include <xnamath.h>

#include "d3d11_Core.hpp"

namespace shi62::d3d11 {

auto CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut) -> HRESULT {
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DX11CompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel,
                               dwShaderFlags, 0, nullptr, ppBlobOut, &pErrorBlob, nullptr);
    if (FAILED(hr)) {
        if (pErrorBlob != nullptr)
            OutputDebugStringA(( char* )pErrorBlob->GetBufferPointer());
        if (pErrorBlob)
            pErrorBlob->Release();
        return hr;
    }
    if (pErrorBlob)
        pErrorBlob->Release();

    return S_OK;
}

Core::Core(const HWND windowHandle)
    : terminationRequested(false)
    , mWindowHandle(windowHandle)
    , mDriverType(D3D_DRIVER_TYPE_NULL)
    , mFeatureLevel(D3D_FEATURE_LEVEL_11_0)
    , mD3dDevice(nullptr)
    , mImmediateContext(nullptr)
    , mSwapChain(nullptr)
    , mRenderTargetView(nullptr)
    , mVertexShader(nullptr)
    , mPixelShader(nullptr)
    , mVertexLayout(nullptr)
    , mVertexPos(nullptr) {
    HRESULT hr = S_OK;

    RECT rect;
    GetClientRect(mWindowHandle, &rect);
    UINT width = rect.right - rect.left;
    UINT height = rect.bottom - rect.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] = {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = mWindowHandle;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++) {
        mDriverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain(nullptr, mDriverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
                                           D3D11_SDK_VERSION, &sd, &mSwapChain, &mD3dDevice, &mFeatureLevel, &mImmediateContext);
        if (SUCCEEDED(hr))
            break;
    }
    if (FAILED(hr)) {
        HandleError(L"Driver initialization failed");
        return;
    }

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), ( LPVOID* )&pBackBuffer);
    if (FAILED(hr)) {
        HandleError(L"Swapchain initialization failed");
        return;
    }

    hr = mD3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &mRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr)) {
        HandleError(L"RenderTargetView creation failed");
        return;
    }

    mImmediateContext->OMSetRenderTargets(1, &mRenderTargetView, nullptr);

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = ( FLOAT )width;
    vp.Height = ( FLOAT )height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    mImmediateContext->RSSetViewports(1, &vp);

    // Compile the vertex shader
    ID3DBlob* pVSBlob = nullptr;
    hr = CompileShaderFromFile(L"Vertex_Pixel.fx", "VS", "vs_4_0", &pVSBlob);
    if (FAILED(hr)) {
        HandleError(L"VertexShader compiling failed");
        return;
    }

    // Create the vertex shader
    hr = mD3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &mVertexShader);
    if (FAILED(hr)) {
        HandleError(L"VertexShader creation failed");
        return;
    }

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE(layout);

    // Create the input layout
    hr = mD3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
                                       pVSBlob->GetBufferSize(), &mVertexLayout);
    pVSBlob->Release();
    if (FAILED(hr)) {
        HandleError(L"InputLayout initialization failed");
        return;
    }

    // Set the input layout
    mImmediateContext->IASetInputLayout(mVertexLayout);

    // Compile the pixel shader
    ID3DBlob* pPSBlob = nullptr;
    hr = CompileShaderFromFile(L"Vertex_Pixel.fx", "PS", "ps_4_0", &pPSBlob);
    if (FAILED(hr)) {
        HandleError(L"PixelShader compiling failed");
        return;
    }

    // Create the pixel shader
    hr = mD3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &mPixelShader);
    pPSBlob->Release();
    if (FAILED(hr)) {
        HandleError(L"PixelShader creation failed");
        return;
    }

    // Set vertex buffer
    UINT stride = sizeof(XMFLOAT3);
    UINT offset = 0;
    mImmediateContext->IASetVertexBuffers(0, 1, &mVertexPos, &stride, &offset);

    // Set primitive topology
    mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

Core::~Core() {
    CleanupDevice();
}

auto Core::TerminationRequested() -> bool {
    return terminationRequested;
}

auto Core::Update() -> void {
    // Clear the back buffer
    float ClearColor[4] = { 0.0f, 0.125f, 0.25f, 0.75f }; // red,green,blue,alpha
    mImmediateContext->ClearRenderTargetView(mRenderTargetView, ClearColor);
}

auto Core::Draw() -> void const {
    // Render a triangle
    mImmediateContext->VSSetShader(mVertexShader, nullptr, 0);
    mImmediateContext->PSSetShader(mPixelShader, nullptr, 0);
    mImmediateContext->Draw(3, 0);

    // Present the information rendered to the back buffer to the front buffer (the screen)
    mSwapChain->Present(0, 0);
}

auto Core::CleanupDevice() -> void {
    if (mImmediateContext)
        mImmediateContext->ClearState();
    if (mVertexPos)
        mVertexPos->Release();
    if (mVertexLayout)
        mVertexLayout->Release();
    if (mVertexShader)
        mVertexShader->Release();
    if (mPixelShader)
        mPixelShader->Release();
    if (mRenderTargetView)
        mRenderTargetView->Release();
    if (mSwapChain)
        mSwapChain->Release();
    if (mImmediateContext)
        mImmediateContext->Release();
    if (mD3dDevice)
        mD3dDevice->Release();
}

auto Core::HandleError(LPCWSTR errMsg) -> void {
    CleanupDevice();
    terminationRequested = true;
    MessageBox(nullptr, errMsg, L"Error", MB_OK);
}

} // namespace shi62::d3d11

auto shi62::d3d11::Core::CreateVertexPosBuffer() -> void {
    XMFLOAT3 verticesPos[] = {
        XMFLOAT3(-1.0f, 1.0f, -1.0f),
        XMFLOAT3(1.0f, 1.0f, -1.0f),
        XMFLOAT3(1.0f, -1.0f, -1.0f),
        XMFLOAT3(-1.0f, -1.0f, -1.0f),
        XMFLOAT3(-1.0f, 1.0f, 1.0f),
        XMFLOAT3(1.0f, 1.0f, 1.0f),
        XMFLOAT3(1.0f, -1.0f, 1.0f),
        XMFLOAT3(-1.0f, -1.0f, 1.0f),
    };
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(XMFLOAT3) * 8;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = verticesPos;
    auto hr = mD3dDevice->CreateBuffer(&bd, &InitData, &mVertexPos);
    if (FAILED(hr)) {
        HandleError(L"VertexBuffer initialization failed");
        return;
    }
}

auto shi62::d3d11::Core::CreateVertexColorBuffer() -> void {
    XMFLOAT3 verticesColor[] = {
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(0.0f, 0.0f, 1.0f),
        XMFLOAT3(0.0f, 1.0f, 0.0f),
        XMFLOAT3(0.0f, 1.0f, 1.0f),
        XMFLOAT3(1.0f, 0.0f, 0.0f),
        XMFLOAT3(1.0f, 0.0f, 1.0f),
        XMFLOAT3(1.0f, 1.0f, 0.0f),
        XMFLOAT3(1.0f, 1.0f, 1.0f),
    };
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(XMFLOAT3) * 8;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = verticesColor;
    auto hr = mD3dDevice->CreateBuffer(&bd, &InitData, &mVertexColor);
    if (FAILED(hr)) {
        HandleError(L"VertexBuffer initialization failed");
        return;
    }
}
