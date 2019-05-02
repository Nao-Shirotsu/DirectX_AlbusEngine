#include <windows.h>
#include <mmsystem.h>
#include <crtdbg.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <xnamath.h>

#include "winapi_Window.hpp"

#define SAFE_RELEASE(x) \
  {                     \
    if (x) {            \
      (x)->Release();   \
      (x) = NULL;       \
    }                   \
  } // 解放マクロ

//機能レベルの配列
D3D_FEATURE_LEVEL g_pFeatureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
UINT g_FeatureLevels = 3;                   // 配列の要素数
D3D_FEATURE_LEVEL g_FeatureLevelsSupported; // デバイス作成時に返される機能レベル

// インターフェイス
ID3D11Device* g_pD3DDevice = NULL;               // デバイス
ID3D11DeviceContext* g_pImmediateContext = NULL; // デバイス・コンテキスト
IDXGISwapChain* g_pSwapChain = NULL;             // スワップ・チェイン

ID3D11RenderTargetView* g_pRenderTargetView = NULL; // 描画ターゲット・ビュー
D3D11_VIEWPORT g_ViewPort[1];                       // ビューポート

ID3D11Texture2D* g_pDepthStencil = NULL;            // 深度/ステンシル
ID3D11DepthStencilView* g_pDepthStencilView = NULL; // 深度/ステンシル・ビュー

ID3D11VertexShader* g_pVertexShader = NULL;            // 頂点シェーダ
ID3D11GeometryShader* g_pGeometryShader = NULL;        // ジオメトリ・シェーダ
ID3D11PixelShader* g_pPixelShader[2] = { NULL, NULL }; // ピクセル・シェーダ
ID3D11Buffer* g_pCBuffer = NULL;                       // 定数バッファ

ID3D11BlendState* g_pBlendState = NULL;               // ブレンド・ステート・オブジェクト
ID3D11RasterizerState* g_pRasterizerState = NULL;     // ラスタライザ・ステート・オブジェクト
ID3D11DepthStencilState* g_pDepthStencilState = NULL; // 深度/ステンシル・ステート・オブジェクト

ID3D11Resource* g_pTexture = NULL;              // テクスチャ
ID3D11ShaderResourceView* g_pTextureSRV = NULL;         // シェーダ リソース ビュー

ID3D11SamplerState* g_pTextureSamplerWrap = NULL;       // サンプラー
ID3D11SamplerState* g_pTextureSamplerMirror = NULL;     // サンプラー
ID3D11SamplerState* g_pTextureSamplerClamp = NULL;      // サンプラー
ID3D11SamplerState* g_pTextureSamplerMirrorOnce = NULL; // サンプラー
ID3D11SamplerState* g_pTextureSamplerBorder = NULL;     // サンプラー

// シェーダのコンパイル オプション
#if defined(DEBUG) || defined(_DEBUG)
UINT g_flagCompile = D3D10_SHADER_DEBUG | D3D10_SHADER_SKIP_OPTIMIZATION
                     | D3D10_SHADER_ENABLE_STRICTNESS | D3D10_SHADER_PACK_MATRIX_COLUMN_MAJOR;
#else
UINT g_flagCompile = D3D10_SHADER_ENABLE_STRICTNESS | D3D10_SHADER_PACK_MATRIX_COLUMN_MAJOR;
#endif

//選択中のピクセル・シェーダ
int g_numPixelShader = 0;

// 深度バッファのモード
bool g_bDepthMode = true;

// スタンバイモード
bool g_bStandbyMode = false;

// 描画ターゲットをクリアする値(RGBA)
float g_ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };

float g_fEye = 5.0f;                     // 視点までの距離
XMFLOAT3 g_vLightPos(3.0f, 3.0f, -3.0f); // 光源座標(ワールド座標系)

// 定数バッファのデータ定義
struct cbCBuffer {
  XMFLOAT4X4 World;      // ワールド変換行列
  XMFLOAT4X4 View;       // ビュー変換行列
  XMFLOAT4X4 Projection; // 透視変換行列
  XMFLOAT3 Light;        // 光源座標(ワールド座標系)
  FLOAT dummy;           // ダミー
};

// 定数バッファのデータ
struct cbCBuffer g_cbCBuffer;

// InitDirect3Dで使用するため前方宣言
HRESULT InitBackBuffer();

void IndicateMessageBox(LPCWSTR errMsg) {
  MessageBox(nullptr, errMsg, L"Error", MB_OK);
}

/*-------------------------------------------
	Direct3D初期化
--------------------------------------------*/
HRESULT InitDirect3D(HWND windowHandle) {
  // ウインドウのクライアント エリア
  RECT rc;
  GetClientRect(windowHandle, &rc);
  UINT width = rc.right - rc.left;
  UINT height = rc.bottom - rc.top;

  // デバイスとスワップ チェインの作成
  DXGI_SWAP_CHAIN_DESC sd;
  ZeroMemory(&sd, sizeof(sd));                                           // 構造体の値を初期化
  sd.BufferCount = 3;                                                    // バック バッファ数
  sd.BufferDesc.Width = width;                                           // バック バッファの幅
  sd.BufferDesc.Height = height;                                         // バック バッファの高さ
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;                     // フォーマット
  sd.BufferDesc.RefreshRate.Numerator = 60;                              // リフレッシュ レート(分子)
  sd.BufferDesc.RefreshRate.Denominator = 1;                             // リフレッシュ レート(分母)
  sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE; // スキャンライン
  sd.BufferDesc.Scaling = DXGI_MODE_SCALING_CENTERED;                    // スケーリング
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;                      // バック バッファの使用法
  sd.OutputWindow = windowHandle;                                           // 関連付けるウインドウ
  sd.SampleDesc.Count = 1;                                               // マルチ サンプルの数
  sd.SampleDesc.Quality = 0;                                             // マルチ サンプルのクオリティ
  sd.Windowed = TRUE;                                                    // ウインドウ モード
  sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;                     // モード自動切り替え

#if defined(DEBUG) || defined(_DEBUG)
  UINT createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
#else
  UINT createDeviceFlags = 0;
#endif

  // ハードウェア・デバイスを作成
  HRESULT hr = D3D11CreateDeviceAndSwapChain(
      NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags,
      g_pFeatureLevels, g_FeatureLevels, D3D11_SDK_VERSION, &sd,
      &g_pSwapChain, &g_pD3DDevice, &g_FeatureLevelsSupported,
      &g_pImmediateContext);
  if (FAILED(hr)) {
    // WARPデバイスを作成
    hr = D3D11CreateDeviceAndSwapChain(
        NULL, D3D_DRIVER_TYPE_WARP, NULL, createDeviceFlags,
        g_pFeatureLevels, g_FeatureLevels, D3D11_SDK_VERSION, &sd,
        &g_pSwapChain, &g_pD3DDevice, &g_FeatureLevelsSupported,
        &g_pImmediateContext);
    if (FAILED(hr)) {
      // リファレンス・デバイスを作成
      hr = D3D11CreateDeviceAndSwapChain(
          NULL, D3D_DRIVER_TYPE_REFERENCE, NULL, createDeviceFlags,
          g_pFeatureLevels, g_FeatureLevels, D3D11_SDK_VERSION, &sd,
          &g_pSwapChain, &g_pD3DDevice, &g_FeatureLevelsSupported,
          &g_pImmediateContext);
      if (FAILED(hr)) {
        return E_FAIL;
      }
    }
  }

  // **********************************************************
  // 頂点シェーダのコードをコンパイル
  ID3DBlob* pBlobVS = NULL;
  hr = D3DX11CompileFromFile(
      L"shader.fx", // ファイル名
      NULL,                          // マクロ定義(なし)
      NULL,                          // インクルード・ファイル定義(なし)
      "VS",                          // 「VS関数」がシェーダから実行される
      "vs_4_0",                      // 頂点シェーダ
      g_flagCompile,                 // コンパイル・オプション
      0,                             // エフェクトのコンパイル・オプション(なし)
      NULL,                          // 直ぐにコンパイルしてから関数を抜ける。
      &pBlobVS,                      // コンパイルされたバイト・コード
      NULL,                          // エラーメッセージは不要
      NULL);                         // 戻り値
  if (FAILED(hr)) {
    IndicateMessageBox(L"Vertex compile");
    return hr;
  }

  // 頂点シェーダの作成
  hr = g_pD3DDevice->CreateVertexShader(
      pBlobVS->GetBufferPointer(), // バイト・コードへのポインタ
      pBlobVS->GetBufferSize(),    // バイト・コードの長さ
      NULL,
      &g_pVertexShader); // 頂点シェーダを受け取る変数
  SAFE_RELEASE(pBlobVS); // バイト・コードを解放
  if (FAILED(hr)) {
    IndicateMessageBox(L"Vertex create");
    return hr;
  }

  // **********************************************************
  // ジオメトリ・シェーダのコードをコンパイル
  ID3DBlob* pBlobGS = NULL;
  hr = D3DX11CompileFromFile(
      L"shader.fx", // ファイル名
      NULL,                          // マクロ定義(なし)
      NULL,                          // インクルード・ファイル定義(なし)
      "GS",                          // 「GS関数」がシェーダから実行される
      "gs_4_0",                      // ジオメトリ・シェーダ
      g_flagCompile,                 // コンパイル・オプション
      0,                             // エフェクトのコンパイル・オプション(なし)
      NULL,                          // 直ぐにコンパイルしてから関数を抜ける。
      &pBlobGS,                      // コンパイルされたバイト・コード
      NULL,                          // エラーメッセージは不要
      NULL);                         // 戻り値
  if (FAILED(hr)) {
    IndicateMessageBox(L"Geometry compile");
    return hr;
  }

  // ジオメトリ・シェーダの作成
  hr = g_pD3DDevice->CreateGeometryShader(
      pBlobGS->GetBufferPointer(), // バイト・コードへのポインタ
      pBlobGS->GetBufferSize(),    // バイト・コードの長さ
      NULL,
      &g_pGeometryShader); // ジオメトリ・シェーダを受け取る変数
  SAFE_RELEASE(pBlobGS);   // バイト・コードを解放
  if (FAILED(hr)) {
    IndicateMessageBox(L"Geometry create");
    return hr;
  }

  // **********************************************************
  // ピクセル・シェーダのコードをコンパイル
  LPCSTR shader_name[2] = { "PS_Load", "PS_Sample" };
  for (int i = 0; i < _countof(shader_name); ++i) {
    ID3DBlob* pBlobPS = NULL;
    hr = D3DX11CompileFromFile(
        L"shader.fx", // ファイル名
        NULL,                          // マクロ定義(なし)
        NULL,                          // インクルード・ファイル定義(なし)
        shader_name[i],                // 「shader_name[i]関数」がシェーダから実行される
        "ps_4_0",                      // ピクセル・シェーダ
        g_flagCompile,                 // コンパイル・オプション
        0,                             // エフェクトのコンパイル・オプション(なし)
        NULL,                          // 直ぐにコンパイルしてから関数を抜ける。
        &pBlobPS,                      // コンパイルされたバイト・コード
        NULL,                          // エラーメッセージは不要
        NULL);                         // 戻り値
    if (FAILED(hr)) {
      IndicateMessageBox(L"Pixel compile");
      return hr;
    }

    // ピクセル・シェーダの作成
    hr = g_pD3DDevice->CreatePixelShader(
        pBlobPS->GetBufferPointer(), // バイト・コードへのポインタ
        pBlobPS->GetBufferSize(),    // バイト・コードの長さ
        NULL,
        &g_pPixelShader[i]); // ピクセル・シェーダを受け取る変数
    SAFE_RELEASE(pBlobPS);   // バイト・コードを解放
    if (FAILED(hr)) {
      IndicateMessageBox(L"Pixel create");
      return hr;
    }
  }

  // **********************************************************
  // 定数バッファの定義
  D3D11_BUFFER_DESC cBufferDesc;
  cBufferDesc.Usage = D3D11_USAGE_DYNAMIC;             // 動的(ダイナミック)使用法
  cBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;  // 定数バッファ
  cBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // CPUから書き込む
  cBufferDesc.MiscFlags = 0;
  cBufferDesc.StructureByteStride = 0;

  // 定数バッファの作成
  cBufferDesc.ByteWidth = sizeof(cbCBuffer); // バッファ・サイズ
  hr = g_pD3DDevice->CreateBuffer(&cBufferDesc, NULL, &g_pCBuffer);
  if (FAILED(hr)) {
    IndicateMessageBox(L"CBuffer create");
    return hr;
  }

  // **********************************************************
  // ブレンド・ステート・オブジェクトの作成
  D3D11_BLEND_DESC BlendState;
  ZeroMemory(&BlendState, sizeof(D3D11_BLEND_DESC));
  BlendState.AlphaToCoverageEnable = FALSE;
  BlendState.IndependentBlendEnable = FALSE;
  BlendState.RenderTarget[0].BlendEnable = FALSE;
  BlendState.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
  hr = g_pD3DDevice->CreateBlendState(&BlendState, &g_pBlendState);
  if (FAILED(hr)) {
    IndicateMessageBox(L"BlendState create");
    return hr;
  }

  // ラスタライザ・ステート・オブジェクトの作成
  D3D11_RASTERIZER_DESC RSDesc;
  RSDesc.FillMode = D3D11_FILL_SOLID;   // 普通に描画する
  RSDesc.CullMode = D3D11_CULL_NONE;    // 両面を描画する
  RSDesc.FrontCounterClockwise = FALSE; // 時計回りが表面
  RSDesc.DepthBias = 0;
  RSDesc.DepthBiasClamp = 0;
  RSDesc.SlopeScaledDepthBias = 0;
  RSDesc.DepthClipEnable = TRUE;
  RSDesc.ScissorEnable = FALSE;
  RSDesc.MultisampleEnable = FALSE;
  RSDesc.AntialiasedLineEnable = FALSE;
  hr = g_pD3DDevice->CreateRasterizerState(&RSDesc, &g_pRasterizerState);
  if (FAILED(hr)) {
    IndicateMessageBox(L"RasterizerState create");
    return hr;
  }

  // **********************************************************
  // 深度/ステンシル・ステート・オブジェクトの作成
  D3D11_DEPTH_STENCIL_DESC DepthStencil;
  DepthStencil.DepthEnable = TRUE;                          // 深度テストあり
  DepthStencil.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; // 書き込む
  DepthStencil.DepthFunc = D3D11_COMPARISON_LESS;           // 手前の物体を描画
  DepthStencil.StencilEnable = FALSE;                       // ステンシル・テストなし
  DepthStencil.StencilReadMask = 0;                         // ステンシル読み込みマスク。
  DepthStencil.StencilWriteMask = 0;                        // ステンシル書き込みマスク。
      // 面が表を向いている場合のステンシル・テストの設定
  DepthStencil.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;      // 維持
  DepthStencil.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP; // 維持
  DepthStencil.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;      // 維持
  DepthStencil.FrontFace.StencilFunc = D3D11_COMPARISON_NEVER;       // 常に失敗
      // 面が裏を向いている場合のステンシル・テストの設定
  DepthStencil.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;      // 維持
  DepthStencil.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP; // 維持
  DepthStencil.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;      // 維持
  DepthStencil.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;      // 常に成功
  hr = g_pD3DDevice->CreateDepthStencilState(&DepthStencil,
                                             &g_pDepthStencilState);
  if (FAILED(hr)) {
    IndicateMessageBox(L"StencilState create");
    return hr;
  };

  // **********************************************************
  // 作成するテクスチャの設定
  D3DX11_IMAGE_LOAD_INFO imageLoadInfo;
  imageLoadInfo.Width = 256;                            // テクスチャの幅
  imageLoadInfo.Height = 256;                           // テクスチャの高さ
  imageLoadInfo.Depth = 0;                              // テクスチャの深さ
  imageLoadInfo.FirstMipLevel = 0;                      // 読み込む最初のミップマップ レベル
  imageLoadInfo.MipLevels = 8;                          // ミップマップ レベルの数
  imageLoadInfo.Usage = D3D11_USAGE_DEFAULT;            // デフォルト使用法
  imageLoadInfo.BindFlags = D3D11_BIND_SHADER_RESOURCE; // シェーダ リソース
  imageLoadInfo.CpuAccessFlags = 0;                     // CPUからアクセスしない
  imageLoadInfo.MiscFlags = 0;                          // オプション(特になし)
  imageLoadInfo.Format = DXGI_FORMAT_R8G8B8A8_UNORM;    // フォーマット
  imageLoadInfo.Filter = D3DX11_FILTER_LINEAR;          // 線形フィルタ
  imageLoadInfo.MipFilter = D3DX11_FILTER_LINEAR;       // 線形フィルタ
  imageLoadInfo.pSrcInfo = NULL;

  // テクスチャの読み込み
  D3DX11CreateTextureFromFile(
      g_pD3DDevice,              // リソースを作成するデバイス
      L"cbfront.png", // 画像ファイル名
      &imageLoadInfo,            // 作成するテクスチャの設定
      NULL,                      // 非同期で実行しない
      &g_pTexture,               // テクスチャを取得する変数
      &hr);                      // 戻り値
  if (FAILED(hr)) {
    IndicateMessageBox(L"Texture load");
    return hr;
  };

  // 2Dテクスチャにアクセスするシェーダ リソース ビューの設定
  D3D11_SHADER_RESOURCE_VIEW_DESC srDesc;
  srDesc.Format = imageLoadInfo.Format;               // フォーマット
  srDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
  srDesc.Texture2D.MostDetailedMip = 0;               // 最初のミップマップ レベル
  srDesc.Texture2D.MipLevels = -1;                    // すべてのミップマップ レベル

  // シェーダ リソース ビューの作成
  hr = g_pD3DDevice->CreateShaderResourceView(
      g_pTexture,      // アクセスするテクスチャ リソース
      &srDesc,         // シェーダ リソース ビューの設定
      &g_pTextureSRV); // 受け取る変数
  if (FAILED(hr)) {
    IndicateMessageBox(L"ShaderResourceView create");
    return hr;
  }

  // サンプラーの作成
  D3D11_SAMPLER_DESC descSampler;
  descSampler.Filter = D3D11_FILTER_ANISOTROPIC;
  descSampler.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
  descSampler.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
  descSampler.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
  descSampler.MipLODBias = 0.0f;
  descSampler.MaxAnisotropy = 2;
  descSampler.ComparisonFunc = D3D11_COMPARISON_NEVER;
  descSampler.BorderColor[0] = 0.0f;
  descSampler.BorderColor[1] = 0.0f;
  descSampler.BorderColor[2] = 0.0f;
  descSampler.BorderColor[3] = 0.0f;
  descSampler.MinLOD = -FLT_MAX;
  descSampler.MaxLOD = FLT_MAX;
  hr = g_pD3DDevice->CreateSamplerState(&descSampler, &g_pTextureSamplerWrap);
  if (FAILED(hr)) {
    IndicateMessageBox(L"SamplerState create");
    return hr;
  }
  descSampler.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
  descSampler.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
  hr = g_pD3DDevice->CreateSamplerState(&descSampler, &g_pTextureSamplerMirror);
  if (FAILED(hr)) {
    IndicateMessageBox(L"SamplerState create");
    return hr;
  }
  descSampler.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
  descSampler.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
  hr = g_pD3DDevice->CreateSamplerState(&descSampler, &g_pTextureSamplerClamp);
  if (FAILED(hr)) {
    IndicateMessageBox(L"SamplerState create");
    return hr;
  }
  descSampler.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
  descSampler.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
  hr = g_pD3DDevice->CreateSamplerState(&descSampler, &g_pTextureSamplerMirrorOnce);
  if (FAILED(hr)) {
    IndicateMessageBox(L"SamplerState create");
    return hr;
  }
  descSampler.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
  descSampler.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
  descSampler.BorderColor[0] = 1.0f;
  descSampler.BorderColor[1] = 0.0f;
  descSampler.BorderColor[2] = 0.0f;
  descSampler.BorderColor[3] = 1.0f;
  hr = g_pD3DDevice->CreateSamplerState(&descSampler, &g_pTextureSamplerBorder);
  if (FAILED(hr)) {
    IndicateMessageBox(L"SamplerState create");
    return hr;
  }

  // **********************************************************
  // バック バッファの初期化
  hr = InitBackBuffer();
  if (FAILED(hr)) {
    IndicateMessageBox(L"BackBuffer init");
    return hr;
  }

  return hr;
}

/*-------------------------------------------
	バック バッファの初期化(バック バッファを描画ターゲットに設定)
--------------------------------------------*/
HRESULT InitBackBuffer(void) {
  HRESULT hr;

  // スワップ・チェインから最初のバック・バッファを取得する
  ID3D11Texture2D* pBackBuffer; // バッファのアクセスに使うインターフェイス
  hr = g_pSwapChain->GetBuffer(
      0,                         // バック・バッファの番号
      __uuidof(ID3D11Texture2D), // バッファにアクセスするインターフェイス
      ( LPVOID* )&pBackBuffer);  // バッファを受け取る変数
  if (FAILED(hr)) {
    IndicateMessageBox(L"BackBuffer get");
    return hr;
  }

  // バック・バッファの情報
  D3D11_TEXTURE2D_DESC descBackBuffer;
  pBackBuffer->GetDesc(&descBackBuffer);

  // バック・バッファの描画ターゲット・ビューを作る
  hr = g_pD3DDevice->CreateRenderTargetView(
      pBackBuffer,           // ビューでアクセスするリソース
      NULL,                  // 描画ターゲット・ビューの定義
      &g_pRenderTargetView); // 描画ターゲット・ビューを受け取る変数
  SAFE_RELEASE(pBackBuffer); // 以降、バック・バッファは直接使わないので解放
  if (FAILED(hr)) {
    IndicateMessageBox(L"RenderTargetView create");
    return hr;
  }

  // 深度/ステンシル・テクスチャの作成
  D3D11_TEXTURE2D_DESC descDepth = descBackBuffer;
  //	descDepth.Width     = descBackBuffer.Width;   // 幅
  //	descDepth.Height    = descBackBuffer.Height;  // 高さ
  descDepth.MipLevels = 1;                        // ミップマップ・レベル数
  descDepth.ArraySize = 1;                        // 配列サイズ
  descDepth.Format = DXGI_FORMAT_D32_FLOAT;       // フォーマット(深度のみ)
                                                  //	descDepth.SampleDesc.Count   = 1;  // マルチサンプリングの設定
                                                  //	descDepth.SampleDesc.Quality = 0;  // マルチサンプリングの品質
  descDepth.Usage = D3D11_USAGE_DEFAULT;          // デフォルト使用法
  descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL; // 深度/ステンシルとして使用
  descDepth.CPUAccessFlags = 0;                   // CPUからはアクセスしない
  descDepth.MiscFlags = 0;                        // その他の設定なし
  hr = g_pD3DDevice->CreateTexture2D(
      &descDepth,        // 作成する2Dテクスチャの設定
      NULL,              //
      &g_pDepthStencil); // 作成したテクスチャを受け取る変数
  if (FAILED(hr)) {
    IndicateMessageBox(L"DepthStencilTexture create");
    return hr;
  }

  // 深度/ステンシル ビューの作成
  D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
  descDSV.Format = descDepth.Format; // ビューのフォーマット
  descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
  descDSV.Flags = 0;
  descDSV.Texture2D.MipSlice = 0;
  hr = g_pD3DDevice->CreateDepthStencilView(
      g_pDepthStencil,       // 深度/ステンシル・ビューを作るテクスチャ
      &descDSV,              // 深度/ステンシル・ビューの設定
      &g_pDepthStencilView); // 作成したビューを受け取る変数
  if (FAILED(hr)) {
    IndicateMessageBox(L"DepthStencilView create");
    return hr;
  }

  // ビューポートの設定
  g_ViewPort[0].TopLeftX = 0.0f;                         // ビューポート領域の左上X座標。
  g_ViewPort[0].TopLeftY = 0.0f;                         // ビューポート領域の左上Y座標。
  g_ViewPort[0].Width = ( FLOAT )descBackBuffer.Width;   // ビューポート領域の幅
  g_ViewPort[0].Height = ( FLOAT )descBackBuffer.Height; // ビューポート領域の高さ
  g_ViewPort[0].MinDepth = 0.0f;                         // ビューポート領域の深度値の最小値
  g_ViewPort[0].MaxDepth = 1.0f;                         // ビューポート領域の深度値の最大値

  // 定数バッファを更新
  // 射影変換行列(パースペクティブ(透視法)射影)
  XMMATRIX mat = XMMatrixPerspectiveFovLH(
      XMConvertToRadians(30.0f),                                      // 視野角30°
      ( float )descBackBuffer.Width / ( float )descBackBuffer.Height, // アスペクト比
      1.0f,                                                           // 前方投影面までの距離
      20.0f);                                                         // 後方投影面までの距離
  mat = XMMatrixTranspose(mat);
  XMStoreFloat4x4(&g_cbCBuffer.Projection, mat);

  return S_OK;
}

/*--------------------------------------------
	画面の描画処理
--------------------------------------------*/
HRESULT Render(void) {
  HRESULT hr;

  // 描画ターゲットのクリア
  g_pImmediateContext->ClearRenderTargetView(
      g_pRenderTargetView, // クリアする描画ターゲット
      g_ClearColor);       // クリアする値

  // 深度/ステンシルのクリア
  g_pImmediateContext->ClearDepthStencilView(
      g_pDepthStencilView, // クリアする深度/ステンシル・ビュー
      D3D11_CLEAR_DEPTH,   // 深度値だけをクリアする
      1.0f,                // 深度バッファをクリアする値
      0);                  // ステンシル・バッファをクリアする値(この場合、無関係)

  // ***************************************
  // 立方体の描画

  // 定数バッファを更新
  // ビュー変換行列
  XMVECTORF32 eyePosition = { 0.0f, g_fEye, -g_fEye, 1.0f }; // 視点(カメラの位置)
  XMVECTORF32 focusPosition = { 0.0f, 0.0f, 0.0f, 1.0f };    // 注視点
  XMVECTORF32 upDirection = { 0.0f, 1.0f, 0.0f, 1.0f };      // カメラの上方向
  XMMATRIX mat = XMMatrixLookAtLH(eyePosition, focusPosition, upDirection);
  XMStoreFloat4x4(&g_cbCBuffer.View, XMMatrixTranspose(mat));
  // 点光源座標
  XMVECTOR vec = XMVector3TransformCoord(XMLoadFloat3(&g_vLightPos), mat);
  XMStoreFloat3(&g_cbCBuffer.Light, vec);
  // ワールド変換行列
  XMMATRIX matY, matX;
  FLOAT rotate = (FLOAT)(XM_PI * (timeGetTime() % 3000)) / 1500.0f;
  matY = XMMatrixRotationY(rotate);
  XMStoreFloat4x4(&g_cbCBuffer.World, matY);
  // 定数バッファのマップ取得
  D3D11_MAPPED_SUBRESOURCE MappedResource;
  hr = g_pImmediateContext->Map(
      g_pCBuffer,              // マップするリソース
      0,                       // サブリソースのインデックス番号
      D3D11_MAP_WRITE_DISCARD, // 書き込みアクセス
      0,                       //
      &MappedResource);        // データの書き込み先ポインタ
  if (FAILED(hr)) {
    IndicateMessageBox(L"CBuffer update");
    return hr;
  }
  // データ書き込み
  CopyMemory(MappedResource.pData, &g_cbCBuffer, sizeof(cbCBuffer));
  // マップ解除
  g_pImmediateContext->Unmap(g_pCBuffer, 0);

  // ***************************************
  // IAに頂点バッファを設定
  // IAに入力レイアウト・オブジェクトを設定(頂点バッファなし)
  g_pImmediateContext->IASetInputLayout(NULL);
  // IAにプリミティブの種類を設定
  g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // VSに頂点シェーダを設定
  g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
  // VSに定数バッファを設定
  g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);

  // GSにジオメトリ・シェーダを設定
  g_pImmediateContext->GSSetShader(g_pGeometryShader, NULL, 0);
  // GSに定数バッファを設定
  g_pImmediateContext->GSSetConstantBuffers(0, 1, &g_pCBuffer);

  // RSにビューポートを設定
  g_pImmediateContext->RSSetViewports(1, g_ViewPort);
  // RSにラスタライザ・ステート・オブジェクトを設定
  g_pImmediateContext->RSSetState(g_pRasterizerState);

  // PSにピクセル・シェーダを設定
  g_pImmediateContext->PSSetShader(g_pPixelShader[(g_numPixelShader == 0) ? 0 : 1], NULL, 0);
  // PSに定数バッファを設定
  g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
  // PSにシェーダ・リソース・ビューを設定
  g_pImmediateContext->PSSetShaderResources(
      0,               // 設定する最初のスロット番号
      1,               // 設定するシェーダ・リソース・ビューの数
      &g_pTextureSRV); // 設定するシェーダ・リソース・ビューの配列
                       // PSにサンプラーを設定
  //g_pImmediateContext->PSSetSamplers(0, 1, &g_pTextureSamplerWrap);
  //g_pImmediateContext->PSSetSamplers(0, 1, &g_pTextureSamplerMirror);
  g_pImmediateContext->PSSetSamplers(0, 1, &g_pTextureSamplerClamp);
  //g_pImmediateContext->PSSetSamplers(0, 1, &g_pTextureSamplerMirrorOnce);
  //g_pImmediateContext->PSSetSamplers(0, 1, &g_pTextureSamplerBorder);

  // OMに描画ターゲット ビューと深度/ステンシル・ビューを設定
  g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_bDepthMode ? g_pDepthStencilView : NULL);
  // OMにブレンド・ステート・オブジェクトを設定
  FLOAT BlendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
  g_pImmediateContext->OMSetBlendState(g_pBlendState, BlendFactor, 0xffffffff);
  // OMに深度/ステンシル・ステート・オブジェクトを設定
  g_pImmediateContext->OMSetDepthStencilState(g_pDepthStencilState, 0);

  // ***************************************
  // 頂点バッファとインデックス・バッファを使わずに描画する
  g_pImmediateContext->Draw(
      36, // 描画する頂点数
      0); // 最初の頂点ID

  // ***************************************
  // バック バッファの表示
  hr = g_pSwapChain->Present(0,  // 画面を直ぐに更新する
                             0); // 画面を実際に更新する
  Sleep(5);
  return hr;
}

/*-------------------------------------------
	Direct3Dの終了処理
--------------------------------------------*/
bool CleanupDirect3D(void) {
  // デバイス・ステートのクリア
  if (g_pImmediateContext)
    g_pImmediateContext->ClearState();

  // スワップ チェインをウインドウ モードにする
  if (g_pSwapChain)
    g_pSwapChain->SetFullscreenState(FALSE, NULL);

  // 取得したインターフェイスの開放
  SAFE_RELEASE(g_pTextureSamplerWrap);
  SAFE_RELEASE(g_pTextureSamplerMirror);
  SAFE_RELEASE(g_pTextureSamplerClamp);
  SAFE_RELEASE(g_pTextureSamplerMirrorOnce);
  SAFE_RELEASE(g_pTextureSamplerBorder);
  SAFE_RELEASE(g_pTextureSRV);

  SAFE_RELEASE(g_pDepthStencilState);
  SAFE_RELEASE(g_pBlendState);
  SAFE_RELEASE(g_pRasterizerState);

  SAFE_RELEASE(g_pCBuffer);

  SAFE_RELEASE(g_pPixelShader[1]);
  SAFE_RELEASE(g_pPixelShader[0]);
  SAFE_RELEASE(g_pGeometryShader);
  SAFE_RELEASE(g_pVertexShader);

  SAFE_RELEASE(g_pDepthStencilView);
  SAFE_RELEASE(g_pDepthStencil);

  SAFE_RELEASE(g_pRenderTargetView);
  SAFE_RELEASE(g_pSwapChain);

  SAFE_RELEASE(g_pImmediateContext);
  SAFE_RELEASE(g_pD3DDevice);

  return true;
}

/*--------------------------------------------
	デバイスの消失処理
--------------------------------------------*/
HRESULT IsDeviceRemoved(HWND windowHandle) {
  HRESULT hr;

  // デバイスの消失確認
  hr = g_pD3DDevice->GetDeviceRemovedReason();
  switch (hr) {
  case S_OK:
    break; // 正常

  case DXGI_ERROR_DEVICE_HUNG:
  case DXGI_ERROR_DEVICE_RESET:
    return E_FAIL;
    CleanupDirect3D();   // Direct3Dの解放(アプリケーション定義)
    hr = InitDirect3D(windowHandle); // Direct3Dの初期化(アプリケーション定義)
    if (FAILED(hr)) {
      IndicateMessageBox(L"Device reinit");
      return hr;
    }
    break;

  case DXGI_ERROR_DEVICE_REMOVED:
  case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
  case DXGI_ERROR_INVALID_CALL:
  default:
    return E_FAIL;
    return hr; // どうしようもないので、アプリケーションを終了。
  };

  return S_OK; // 正常
}

/*--------------------------------------------
	アイドル時の処理
--------------------------------------------*/
bool AppIdle(HWND windowHandle) {
  if (!g_pD3DDevice)
    return false;

  HRESULT hr;
  // デバイスの消失処理
  hr = IsDeviceRemoved(windowHandle);
  if (FAILED(hr)) {
    IndicateMessageBox(L"Device critical error");
    return hr;
  }

  // スタンバイ モード
  if (g_bStandbyMode) {
    hr = g_pSwapChain->Present(0, DXGI_PRESENT_TEST);
    if (hr != S_OK) {
      Sleep(100); // 0.1秒待つ
      return true;
    }
    g_bStandbyMode = false; // スタンバイ モードを解除する
  }

  // 画面の更新
  hr = Render();
  if (hr == DXGI_STATUS_OCCLUDED) {
    g_bStandbyMode = true; // スタンバイ モードに入る
  }

  return true;
}

int WINAPI wWinMain(HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPWSTR lpCmdLine,
                    int nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);
  UNREFERENCED_PARAMETER(nCmdShow);

  // デバッグ ヒープ マネージャによるメモリ割り当ての追跡方法を設定
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

  // XNA Mathライブラリのサポート チェック
  if (XMVerifyCPUSupport() != TRUE) {
    return 0;
  }

  shi62::winapi::Window window(hInstance, L"win1", L"D3D11 青本 Sample12", 400, 300);

  if (auto res = InitDirect3D(window.GetWindowHandle()); FAILED(res)) {
    CleanupDirect3D();
    return 0;
  }

  while (!window.TerminationRequested()) {
    window.Update();
    AppIdle(window.GetWindowHandle());
  }

  CleanupDirect3D();
  return 0;
}