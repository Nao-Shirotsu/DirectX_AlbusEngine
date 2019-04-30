#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11async.h>
#include <xnamath.h>

#include "d3d11_Core.hpp"

namespace shi62::d3d11 {

auto CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut) -> HRESULT {
  HRESULT resultHandle = S_OK;

  DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3D10_SHADER_PACK_MATRIX_COLUMN_MAJOR;
#if defined(DEBUG) || defined(_DEBUG)
  dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

  resultHandle = D3DX11CompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel,
                                       dwShaderFlags, 0, nullptr, ppBlobOut, nullptr, nullptr);
  return resultHandle;
}

Core::Core(const HWND windowHandle)
    : terminationRequested(false)
    , mWindowHandle(windowHandle)
    , mD3dDevice(nullptr)
    , mImmediateContext(nullptr)
    , mSwapChain(nullptr)
    , mRenderTargetView(nullptr)
    , mDepthStencilBuffer(nullptr)
    , mDepthStencilView(nullptr)
    , mVertexLayout(nullptr)
    , mVertexPos(nullptr)
    , mVertexColor(nullptr)
    , mVertexIndex(nullptr)
    , mVertexShader(nullptr)
    , mGeometryShader(nullptr)
    , mPixelShader(nullptr)
    , mConstantBuffer{ nullptr, nullptr, nullptr }
    , mLightPos(3.0f, 3.0f, -2.0f)
    , mBlendState(nullptr)
    , mRasterizerState(nullptr)
    , mDepthStencilState(nullptr) {
  RECT rect;
  GetClientRect(mWindowHandle, &rect);
  UINT width = rect.right - rect.left;
  UINT height = rect.bottom - rect.top;

  HRESULT resultHandle = CreateDevice(width, height);
  if (FAILED(resultHandle)) {
    HandleError(L"Driver initialization failed");
    return;
  }

  CreateVertexPosBuffer();
  CreateVertexColorBuffer();
  CreateVertexIndexBuffer();

  resultHandle = CreateRenderTargetView();
  if (FAILED(resultHandle)) {
    HandleError(L"RenderTargetView creation failed");
    return;
  }

  SetViewport(width, height);

  // Compile the vertex shader
  ID3DBlob* blobVS = nullptr;
  resultHandle = CompileShaderFromFile(L"Vertex_Pixel.fx", "VS", "vs_4_0", &blobVS);
  if (FAILED(resultHandle)) {
    HandleError(L"VertexShader compiling failed");
    return;
  }

  // Create the vertex shader
  resultHandle = mD3dDevice->CreateVertexShader(blobVS->GetBufferPointer(), blobVS->GetBufferSize(), nullptr, &mVertexShader);
  if (FAILED(resultHandle)) {
    HandleError(L"VertexShader creation failed");
    return;
  }

  resultHandle = DefineInputLayout(blobVS);
  if (FAILED(resultHandle)) {
    HandleError(L"InputLayout initialization failed");
    return;
  }

  // Set the input layout
  mImmediateContext->IASetInputLayout(mVertexLayout);

  // Compile the geometry shader
  ID3DBlob* blobGS = nullptr;
  resultHandle = CompileShaderFromFile(L"Vertex_Pixel.fx", "GS", "gs_4_0", &blobGS);
  if (FAILED(resultHandle)) {
    HandleError(L"GeometryShader compiling failed");
    return;
  }

  // Create the geometry shader
  resultHandle = mD3dDevice->CreateGeometryShader(blobGS->GetBufferPointer(), blobGS->GetBufferSize(), nullptr, &mGeometryShader);
  blobGS->Release();
  if (FAILED(resultHandle)) {
    HandleError(L"GeometryShader creation failed");
    return;
  }

  // Compile the pixel shader
  ID3DBlob* blobPS = nullptr;
  resultHandle = CompileShaderFromFile(L"Vertex_Pixel.fx", "PS", "ps_4_0", &blobPS);
  if (FAILED(resultHandle)) {
    HandleError(L"PixelShader compiling failed");
    return;
  }

  // Create the pixel shader
  resultHandle = mD3dDevice->CreatePixelShader(blobPS->GetBufferPointer(), blobPS->GetBufferSize(), nullptr, &mPixelShader);
  blobPS->Release();
  if (FAILED(resultHandle)) {
    HandleError(L"PixelShader creation failed");
    return;
  }

  // 定数バッファの定義
  D3D11_BUFFER_DESC constantBufferDesc;
  constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;             // 動的(ダイナミック)使用法
  constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;  // 定数バッファ
  constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // CPUから書き込む
  constantBufferDesc.MiscFlags = 0;
  constantBufferDesc.StructureByteStride = 0;

  // 定数バッファ①の作成
  constantBufferDesc.ByteWidth = sizeof(XMFLOAT4X4); // バッファ・サイズ
  resultHandle = mD3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &mConstantBuffer[0]);
  if (FAILED(resultHandle)) {
    HandleError(L"Constant buffer creation failed");
    return;
  }

  // 定数バッファ②の作成
  constantBufferDesc.ByteWidth = sizeof(CBChangesEveryFrame); // バッファ・サイズ
  resultHandle = mD3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &mConstantBuffer[1]);
  if (FAILED(resultHandle)) {
    HandleError(L"Constant buffer creation failed");
    return;
  }

  // 定数バッファ③の作成
  constantBufferDesc.ByteWidth = sizeof(XMFLOAT4X4); // バッファ・サイズ
  resultHandle = mD3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &mConstantBuffer[2]);
  if (FAILED(resultHandle)) {
    HandleError(L"Constant buffer creation failed");
    return;
  }

  resultHandle = CreateBlendState();
  if (FAILED(resultHandle)) {
    HandleError(L"BlendStateObject creation failed");
    return;
  }

  // ラスタライザ・ステート・オブジェクトの作成
  resultHandle = CreateRasterizerState();
  if (FAILED(resultHandle)) {
    HandleError(L"RasterizerStateObject creation failed");
    return;
  }

  // 深度/ステンシル・ステート・オブジェクトの作成
  resultHandle = CreateDepthStencil();
  if (FAILED(resultHandle)) {
    HandleError(L"DepthStencilStateObject creation failed");
    return;
  }

  // バック バッファの初期化
  resultHandle = InitBackBuffer();
  if (FAILED(resultHandle)) {
    HandleError(L"BackBuffer initialization failed");
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
  float clearColor[4] = { 0.0f, 0.125f, 0.25f, 0.75f }; // red,green,blue,alpha
  mImmediateContext->ClearRenderTargetView(mRenderTargetView, clearColor);

  // 深度/ステンシルのクリア
  mImmediateContext->ClearDepthStencilView(
      mDepthStencilView, // クリアする深度/ステンシル・ビュー
      D3D11_CLEAR_DEPTH, // 深度値だけをクリアする
      1.0f,              // 深度バッファをクリアする値
      0);                // ステンシル・バッファをクリアする値(この場合、無関係)

  // ***************************************
  // 立方体の描画

  // 定数バッファ②を更新
  // ビュー変換行列
  XMVECTORF32 cameraPos = { 0.0f, 5.0f, -5.0f, 1.0f };  // 視点(カメラの位置)
  XMVECTORF32 focusPos = { 0.0f, 0.0f, 0.0f, 1.0f }; // 注視点
  XMVECTORF32 cameraDirUp = { 0.0f, 1.0f, 0.0f, 1.0f };   // カメラの上方向
  XMMATRIX mat = XMMatrixLookAtLH(cameraPos, focusPos, cameraDirUp);
  XMStoreFloat4x4(&mCBChangesEveryFrame.View, XMMatrixTranspose(mat));
  // 点光源座標
  XMVECTOR lightSourcePos = XMVector3TransformCoord(XMLoadFloat3(&mLightPos), mat);
  XMStoreFloat3(&mCBChangesEveryFrame.Light, lightSourcePos);
  // 定数バッファ②のマップ取得
  D3D11_MAPPED_SUBRESOURCE mappedResource;
  auto resultHandle = mImmediateContext->Map(
      mConstantBuffer[1],             // マップするリソース
      0,                       // サブリソースのインデックス番号
      D3D11_MAP_WRITE_DISCARD, // 書き込みアクセス
      0,                       //
      &mappedResource);        // データの書き込み先ポインタ
  if (FAILED(resultHandle)) {
    HandleError(L"ConstantBuffer mapping failed");
    return;
  }
  // データ書き込み
  CopyMemory(mappedResource.pData, &mCBChangesEveryFrame, sizeof(CBChangesEveryFrame));
  // マップ解除
  mImmediateContext->Unmap(mConstantBuffer[1], 0);

  // 定数バッファ③を更新
  // ワールド変換行列
  XMMATRIX matY, matX;
  FLOAT rotationDiff = static_cast<FLOAT>(XM_PI * (timeGetTime() % 3000)) / 1500.0f;
  matY = XMMatrixRotationY(rotationDiff);
  rotationDiff = static_cast<FLOAT>(XM_PI * (timeGetTime() % 3000)) / 1500.0f;
  matX = XMMatrixRotationX(rotationDiff);
  XMStoreFloat4x4(&mCBChangesEveryObject, XMMatrixTranspose(matY * matX));
  // 定数バッファ③のマップ取得
  resultHandle = mImmediateContext->Map(
      mConstantBuffer[2],             // マップするリソース
      0,                       // サブリソースのインデックス番号
      D3D11_MAP_WRITE_DISCARD, // 書き込みアクセス
      0,                       //
      &mappedResource);        // データの書き込み先ポインタ
  if (FAILED(resultHandle)) {
    HandleError(L"ConstantBuffer mapping failed");
    return;
  }
  // データ書き込み
  CopyMemory(mappedResource.pData, &mCBChangesEveryObject, sizeof(XMFLOAT4X4));
  // マップ解除
  mImmediateContext->Unmap(mConstantBuffer[2], 0);

  // ***************************************
  // IAに頂点バッファを設定
  UINT strides[2] = { sizeof(XMFLOAT3), sizeof(XMFLOAT3) };
  UINT offsets[2] = { 0, 0 };
  mImmediateContext->IASetVertexBuffers(0, 2, &mVertexPos, strides, offsets);
  // IAにインデックス・バッファを設定
  mImmediateContext->IASetIndexBuffer(mVertexIndex, DXGI_FORMAT_R32_UINT, 0);
  // IAに入力レイアウト・オブジェクトを設定
  mImmediateContext->IASetInputLayout(mVertexLayout);
  // IAにプリミティブの種類を設定
  mImmediateContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // VSに頂点シェーダを設定
  mImmediateContext->VSSetShader(mVertexShader, nullptr, 0);
  // VSに定数バッファを設定
  mImmediateContext->VSSetConstantBuffers(0, 3, mConstantBuffer);

  // GSにジオメトリ・シェーダを設定
  mImmediateContext->GSSetShader(mGeometryShader, nullptr, 0);
  // GSに定数バッファを設定
  mImmediateContext->GSSetConstantBuffers(0, 3, mConstantBuffer);

  // RSにビューポートを設定
  mImmediateContext->RSSetViewports(1, &mViewport);
  // RSにラスタライザ・ステート・オブジェクトを設定
  mImmediateContext->RSSetState(mRasterizerState);

  // PSにピクセル・シェーダを設定
  mImmediateContext->PSSetShader(mPixelShader, nullptr, 0);
  // PSに定数バッファを設定
  mImmediateContext->PSSetConstantBuffers(0, 3, mConstantBuffer);

  // OMに描画ターゲット ビューと深度/ステンシル・ビューを設定
  mImmediateContext->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);
  // OMにブレンド・ステート・オブジェクトを設定
  FLOAT BlendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
  mImmediateContext->OMSetBlendState(mBlendState, BlendFactor, 0xffffffff);
  // OMに深度/ステンシル・ステート・オブジェクトを設定
  mImmediateContext->OMSetDepthStencilState(mDepthStencilState, 0);

  // ***************************************
  // 頂点バッファのデータをインデックス・バッファを使って描画する
  mImmediateContext->DrawIndexed(
      36, // 描画するインデックス数(頂点数)
      0,  // インデックス・バッファの最初のインデックスから描画開始
      0); // 頂点バッファ内の最初の頂点データから使用開始

  // ***************************************
  // バック バッファの表示
  resultHandle = mSwapChain->Present(0,  // 画面を直ぐに更新する
                                     0); // 画面を実際に更新する
  Sleep(1);
}

auto Core::Draw() -> void const {
  // Render a triangle
  mImmediateContext->VSSetShader(mVertexShader, nullptr, 0);
  mImmediateContext->PSSetShader(mPixelShader, nullptr, 0);
  mImmediateContext->Draw(3, 0);

  // Present the information rendered to the back buffer to the front buffer (the screen)
  mSwapChain->Present(0, 0);
}

#define SAFE_RELEASE(x) \
  {                     \
    if (x) {            \
      (x)->Release();   \
      (x) = nullptr;    \
    }                   \
  } // 解放マクロ

auto Core::CleanupDevice() -> void {
  SAFE_RELEASE(mDepthStencilState);
  SAFE_RELEASE(mBlendState);
  SAFE_RELEASE(mRasterizerState);

  SAFE_RELEASE(mConstantBuffer[2]);
  SAFE_RELEASE(mConstantBuffer[1]);
  SAFE_RELEASE(mConstantBuffer[0]);

  SAFE_RELEASE(mVertexLayout);

  SAFE_RELEASE(mPixelShader);
  SAFE_RELEASE(mGeometryShader);
  SAFE_RELEASE(mVertexShader);

  SAFE_RELEASE(mVertexIndex);
  SAFE_RELEASE(mVertexPos);

  SAFE_RELEASE(mVertexColor);

  SAFE_RELEASE(mDepthStencilView);
  SAFE_RELEASE(mDepthStencilBuffer);

  SAFE_RELEASE(mRenderTargetView);
  SAFE_RELEASE(mSwapChain);

  SAFE_RELEASE(mImmediateContext);
  SAFE_RELEASE(mD3dDevice);
}

auto Core::HandleError(LPCWSTR errMsg) -> void {
  CleanupDevice();
  terminationRequested = true;
  MessageBox(nullptr, errMsg, L"Error", MB_OK);
}

} // namespace shi62::d3d11

auto shi62::d3d11::Core::CreateDevice(const UINT width, const UINT height) -> HRESULT {
  UINT createDeviceFlags = 0;
#if defined DEBUG || defined(_DEBUG)
  createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

  D3D_DRIVER_TYPE driverTypes[] = {
    D3D_DRIVER_TYPE_HARDWARE,
    D3D_DRIVER_TYPE_WARP,
    D3D_DRIVER_TYPE_REFERENCE,
    D3D_DRIVER_TYPE_NULL
  };
  UINT numDriverTypes = ARRAYSIZE(driverTypes);

  D3D_FEATURE_LEVEL featureLevels[] = {
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0,
  };
  UINT numFeatureLevels = ARRAYSIZE(featureLevels);

  DXGI_SWAP_CHAIN_DESC swapChainDesc;
  ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
  swapChainDesc.BufferCount = 3;
  swapChainDesc.BufferDesc.Width = width;
  swapChainDesc.BufferDesc.Height = height;
  swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
  swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
  swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
  swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_CENTERED;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.OutputWindow = mWindowHandle;
  swapChainDesc.SampleDesc.Count = 1;
  swapChainDesc.SampleDesc.Quality = 0;
  swapChainDesc.Windowed = TRUE;
  swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

  for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++) {
    auto driverType = driverTypes[driverTypeIndex];
    auto resultHandle = D3D11CreateDeviceAndSwapChain(nullptr, driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
                                                      D3D11_SDK_VERSION, &swapChainDesc, &mSwapChain, &mD3dDevice, &mFeatureLevel, &mImmediateContext);
    if (SUCCEEDED(resultHandle)) {
      return resultHandle;
    }
  }

  return E_FAIL;
}


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

  D3D11_BUFFER_DESC bufferDesc;
  ZeroMemory(&bufferDesc, sizeof(bufferDesc));
  bufferDesc.Usage = D3D11_USAGE_DEFAULT;
  bufferDesc.ByteWidth = sizeof(XMFLOAT3) * 8;
  bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

  D3D11_SUBRESOURCE_DATA InitData;
  ZeroMemory(&InitData, sizeof(InitData));
  InitData.pSysMem = verticesPos;
  auto resultHandle = mD3dDevice->CreateBuffer(&bufferDesc, &InitData, &mVertexPos);
  if (FAILED(resultHandle)) {
    HandleError(L"VertexBuffer initialization failed");
    return;
  }
}

auto shi62::d3d11::Core::CreateVertexColorBuffer() -> void {
  XMFLOAT3 verticesColor[] = {
    XMFLOAT3(1.0f, 1.0f, 1.0f),
    XMFLOAT3(1.0f, 1.0f, 1.0f),
    XMFLOAT3(1.0f, 1.0f, 1.0f),
    XMFLOAT3(1.0f, 1.0f, 1.0f),
    XMFLOAT3(1.0f, 1.0f, 1.0f),
    XMFLOAT3(1.0f, 1.0f, 1.0f),
    XMFLOAT3(1.0f, 1.0f, 1.0f),
    XMFLOAT3(1.0f, 1.0f, 1.0f),
  };

  D3D11_BUFFER_DESC bufferDesc;
  ZeroMemory(&bufferDesc, sizeof(bufferDesc));
  bufferDesc.Usage = D3D11_USAGE_DEFAULT;
  bufferDesc.ByteWidth = sizeof(XMFLOAT3) * 8;
  bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

  D3D11_SUBRESOURCE_DATA InitData;
  ZeroMemory(&InitData, sizeof(InitData));
  InitData.pSysMem = verticesColor;
  auto resultHandle = mD3dDevice->CreateBuffer(&bufferDesc, &InitData, &mVertexColor);
  if (FAILED(resultHandle)) {
    HandleError(L"VertexBuffer initialization failed");
    return;
  }
}


auto shi62::d3d11::Core::CreateVertexIndexBuffer() -> void {
  UINT verticesIndices[] = {
    0, 1, 3,
    1, 2, 3,
    1, 5, 2,
    5, 6, 2,
    5, 4, 6,
    4, 7, 6,
    4, 5, 0,
    5, 1, 0,
    4, 0, 7,
    0, 3, 7,
    3, 2, 7,
    2, 6, 7
  };

  D3D11_BUFFER_DESC bufferDesc;
  ZeroMemory(&bufferDesc, sizeof(bufferDesc));
  bufferDesc.Usage = D3D11_USAGE_DEFAULT;
  bufferDesc.ByteWidth = sizeof(XMFLOAT3) * 36;
  bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

  D3D11_SUBRESOURCE_DATA InitData;
  ZeroMemory(&InitData, sizeof(InitData));
  InitData.pSysMem = verticesIndices;
  auto resultHandle = mD3dDevice->CreateBuffer(&bufferDesc, &InitData, &mVertexIndex);
  if (FAILED(resultHandle)) {
    HandleError(L"VertexBuffer initialization failed");
    return;
  }
}

auto shi62::d3d11::Core::CreateRenderTargetView() -> HRESULT {
  ID3D11Texture2D* backBuffer = nullptr;
  auto resultHandle = mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), ( LPVOID* )&backBuffer);
  if (FAILED(resultHandle)) {
    return resultHandle;
  }

  resultHandle = mD3dDevice->CreateRenderTargetView(backBuffer, nullptr, &mRenderTargetView);
  backBuffer->Release();
  if (FAILED(resultHandle)) {
    return resultHandle;
  }

  mImmediateContext->OMSetRenderTargets(1, &mRenderTargetView, nullptr);

  return S_OK;
}

auto shi62::d3d11::Core::SetViewport(const UINT width, const UINT height) -> void {
  D3D11_VIEWPORT viewport;
  viewport.Width = ( FLOAT )width;
  viewport.Height = ( FLOAT )height;
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  mImmediateContext->RSSetViewports(1, &viewport);
}

auto shi62::d3d11::Core::DefineInputLayout(ID3DBlob* blobVS) -> HRESULT {
  D3D11_INPUT_ELEMENT_DESC layout[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
  };
  UINT numElements = ARRAYSIZE(layout);

  // Create and set the input layout
  auto resultHandle = mD3dDevice->CreateInputLayout(layout, numElements, blobVS->GetBufferPointer(),
                                                    blobVS->GetBufferSize(), &mVertexLayout);
  blobVS->Release();
  return resultHandle;
}

auto shi62::d3d11::Core::CreateBlendState() -> HRESULT {
  D3D11_BLEND_DESC BlendState;
  ZeroMemory(&BlendState, sizeof(D3D11_BLEND_DESC));
  BlendState.AlphaToCoverageEnable = FALSE;
  BlendState.IndependentBlendEnable = FALSE;
  BlendState.RenderTarget[0].BlendEnable = FALSE;
  BlendState.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
  return mD3dDevice->CreateBlendState(&BlendState, &mBlendState);
}

auto shi62::d3d11::Core::CreateRasterizerState() -> HRESULT {
  D3D11_RASTERIZER_DESC rasterizerDesc;
  rasterizerDesc.FillMode = D3D11_FILL_SOLID;   // 普通に描画する
  rasterizerDesc.CullMode = D3D11_CULL_NONE;    // 両面を描画する
  rasterizerDesc.FrontCounterClockwise = FALSE; // 時計回りが表面
  rasterizerDesc.DepthBias = 0;
  rasterizerDesc.DepthBiasClamp = 0;
  rasterizerDesc.SlopeScaledDepthBias = 0;
  rasterizerDesc.DepthClipEnable = TRUE;
  rasterizerDesc.ScissorEnable = FALSE;
  rasterizerDesc.MultisampleEnable = FALSE;
  rasterizerDesc.AntialiasedLineEnable = FALSE;
  return mD3dDevice->CreateRasterizerState(&rasterizerDesc, &mRasterizerState);
}

auto shi62::d3d11::Core::CreateDepthStencil() -> HRESULT {
  D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
  depthStencilDesc.DepthEnable = TRUE;                          // 深度テストあり
  depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; // 書き込む
  depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;           // 手前の物体を描画
  depthStencilDesc.StencilEnable = FALSE;                       // ステンシル・テストなし
  depthStencilDesc.StencilReadMask = 0;                         // ステンシル読み込みマスク。
  depthStencilDesc.StencilWriteMask = 0;                        // ステンシル書き込みマスク。
  // 面が表を向いている場合のステンシル・テストの設定
  depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;      // 維持
  depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP; // 維持
  depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;      // 維持
  depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_NEVER;       // 常に失敗
  // 面が裏を向いている場合のステンシル・テストの設定
  depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;      // 維持
  depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP; // 維持
  depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;      // 維持
  depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;      // 常に成功
  return mD3dDevice->CreateDepthStencilState(&depthStencilDesc, &mDepthStencilState);
}

auto shi62::d3d11::Core::InitBackBuffer() -> HRESULT {
  HRESULT resultHandle;

  // スワップ・チェインから最初のバック・バッファを取得する
  ID3D11Texture2D* backBuffer; // バッファのアクセスに使うインターフェイス
  resultHandle = mSwapChain->GetBuffer(
      0,                         // バック・バッファの番号
      __uuidof(ID3D11Texture2D), // バッファにアクセスするインターフェイス
      ( LPVOID* )&backBuffer);  // バッファを受け取る変数
  if (FAILED(resultHandle)) {
    HandleError(L"BackBuffer getting failed");
    return E_FAIL;
  }

  // バック・バッファの情報
  D3D11_TEXTURE2D_DESC descBackBuffer;
  backBuffer->GetDesc(&descBackBuffer);

  // バック・バッファの描画ターゲット・ビューを作る
  resultHandle = mD3dDevice->CreateRenderTargetView(
      backBuffer,         // ビューでアクセスするリソース
      nullptr,             // 描画ターゲット・ビューの定義
      &mRenderTargetView); // 描画ターゲット・ビューを受け取る変数
  backBuffer->Release();
  if (FAILED(resultHandle)) {
    HandleError(L"BackBuffer targetview creation failed");
    return E_FAIL;
  }

  // 深度/ステンシル・テクスチャの作成
  D3D11_TEXTURE2D_DESC descDepth = descBackBuffer;
  //    descDepth.Width     = descBackBuffer.Width;   // 幅
  //    descDepth.Height    = descBackBuffer.Height;  // 高さ
  descDepth.MipLevels = 1;                        // ミップマップ・レベル数
  descDepth.ArraySize = 1;                        // 配列サイズ
  descDepth.Format = DXGI_FORMAT_D32_FLOAT;       // フォーマット(深度のみ)
                                                  //    descDepth.SampleDesc.Count   = 1;  // マルチサンプリングの設定
                                                  //    descDepth.SampleDesc.Quality = 0;  // マルチサンプリングの品質
  descDepth.Usage = D3D11_USAGE_DEFAULT;          // デフォルト使用法
  descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL; // 深度/ステンシルとして使用
  descDepth.CPUAccessFlags = 0;                   // CPUからはアクセスしない
  descDepth.MiscFlags = 0;                        // その他の設定なし
  resultHandle = mD3dDevice->CreateTexture2D(
      &descDepth,            // 作成する2Dテクスチャの設定
      nullptr,               //
      &mDepthStencilBuffer); // 作成したテクスチャを受け取る変数
  if (FAILED(resultHandle)) {
    HandleError(L"DepthStencilTexture creation failed");
    return E_FAIL;
  }

  // 深度/ステンシル ビューの作成
  D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
  descDSV.Format = descDepth.Format; // ビューのフォーマット
  descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
  descDSV.Flags = 0;
  descDSV.Texture2D.MipSlice = 0;
  resultHandle = mD3dDevice->CreateDepthStencilView(
      mDepthStencilBuffer, // 深度/ステンシル・ビューを作るテクスチャ
      &descDSV,            // 深度/ステンシル・ビューの設定
      &mDepthStencilView); // 作成したビューを受け取る変数
  if (FAILED(resultHandle)) {
    HandleError(L"DepthStencilView creation failed");
    return E_FAIL;
  }

  // ビューポートの設定
  mViewport.TopLeftX = 0.0f;                         // ビューポート領域の左上X座標。
  mViewport.TopLeftY = 0.0f;                         // ビューポート領域の左上Y座標。
  mViewport.Width = ( FLOAT )descBackBuffer.Width;   // ビューポート領域の幅
  mViewport.Height = ( FLOAT )descBackBuffer.Height; // ビューポート領域の高さ
  mViewport.MinDepth = 0.0f;                         // ビューポート領域の深度値の最小値
  mViewport.MaxDepth = 1.0f;                         // ビューポート領域の深度値の最大値

  // 定数バッファ①を更新
  // 射影変換行列(パースペクティブ(透視法)射影)
  XMMATRIX mat = XMMatrixPerspectiveFovLH(
      XMConvertToRadians(30.0f),                                      // 視野角30°
      ( float )descBackBuffer.Width / ( float )descBackBuffer.Height, // アスペクト比
      1.0f,                                                           // 前方投影面までの距離
      20.0f);                                                         // 後方投影面までの距離
  mat = XMMatrixTranspose(mat);
  XMStoreFloat4x4(&mCBNeverChanges, mat);
  // 定数バッファ①のマップ取得
  D3D11_MAPPED_SUBRESOURCE mappedResource;
  resultHandle = mImmediateContext->Map(
      mConstantBuffer[0],             // マップするリソース
      0,                       // サブリソースのインデックス番号
      D3D11_MAP_WRITE_DISCARD, // 書き込みアクセス
      0,                       //
      &mappedResource);        // データの書き込み先ポインタ
  if (FAILED(resultHandle)) {
    HandleError(L"ConstantBuffer update failed");
    return E_FAIL;
  }

  // データ書き込み
  CopyMemory(mappedResource.pData, &mCBNeverChanges, sizeof(XMFLOAT4X4));
  // マップ解除
  mImmediateContext->Unmap(mConstantBuffer[0], 0);

  return S_OK;
}