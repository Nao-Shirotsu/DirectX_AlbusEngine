#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <crtdbg.h>

#include "Constants.hpp"
#include "d3d12_CBuffer.hpp"
#include "d3d12_Core.hpp"
#include "d3d12_Polygon.hpp"

//ライブラリ読み込み
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace {

constexpr auto FRAME_COUNT = 3; //画面バッファ数
constexpr float SCREEN_CLEAR_COLOR[] = { 0.75f, 0.75f, 0.75f, 1.0f };
constexpr int NUM_POLYGONS = 1;

} // anonymous namespace

namespace shi62::d3d12 {

void MYASSERT(HRESULT res, LPCWSTR errmsg) {
  _ASSERT_EXPR(SUCCEEDED(res), errmsg);
}

void Core::InitTrianglePos() {
  for (int i = 0; i < NUM_POLYGONS; ++i) {
    mTrianglePos[i] = { 1.5f * i, 2.0f, 0.0f };
  }
}

Core::Core(const HWND windowHandle)
    : mTermination{ false }
    , mRenderTargets{ FRAME_COUNT }
    , mCbvDataBegin{ nullptr }
    , mCBuffers{ NUM_POLYGONS }
    , mTrianglePos{ NUM_POLYGONS }
    , mVertices{ Vertex{ { 0.0f, 0.5f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
                 Vertex{ { 0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
                 Vertex{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, -1.0f } } } {
  HRESULT res;

  //D3D12デバイスの作成
  res = D3D12CreateDevice(0, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDevice));
  MYASSERT(res, L"CreateDevice");

  //コマンドキューの作成
  D3D12_COMMAND_QUEUE_DESC queueDesc = {};
  queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  res = mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue));
  MYASSERT(res, L"CreateCmdQ");

  //コマンドリストの作成
  res = mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator));
  MYASSERT(res, L"CreateCmdAlloc");
  res = mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&mCommandList));
  MYASSERT(res, L"CreateCmdList");
  mCommandList->Close();

  //スワップチェーンの作成
  DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
  swapChainDesc.BufferCount = FRAME_COUNT;
  swapChainDesc.Width = WINDOW_WIDTH;
  swapChainDesc.Height = WINDOW_HEIGHT;
  swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swapChainDesc.SampleDesc.Count = 1;
  IDXGIFactory4* factory;
  res = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
  MYASSERT(res, L"CreateDXGIFac");
  res = factory->CreateSwapChainForHwnd(mCommandQueue.Get(), windowHandle, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(mSwapChain.GetAddressOf()));
  MYASSERT(res, L"CreateSwap");
  mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

  //ディスクリプターヒープの作成（レンダーターゲットビュー用）
  D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
  rtvHeapDesc.NumDescriptors = FRAME_COUNT;
  rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  res = mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap));
  MYASSERT(res, L"CreateDscrRtvHeap");
  mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  //フレームバッファとバックバッファそれぞれにレンダーターゲットビューを作成
  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
  for (UINT n = 0; n < FRAME_COUNT; n++) {
    res = mSwapChain->GetBuffer(n, IID_PPV_ARGS(&mRenderTargets[n]));
    MYASSERT(res, L"Swap->GetBuff");
    mDevice->CreateRenderTargetView(mRenderTargets[n].Get(), nullptr, rtvHandle);
    rtvHandle.Offset(1, mRtvDescriptorSize);
  }

  //ルートシグネチャーを作成	CBV用にテーブル設定
  CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
  CD3DX12_ROOT_PARAMETER1 rootParameters[1];
  ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
  rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
  CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
  rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters,
                             0, nullptr,
                             D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
  ID3DBlob* signature;
  ID3DBlob* error;
  res = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
  MYASSERT(res, L"RootSign");
  res = mDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mRootSignature));
  MYASSERT(res, L"CreateRootSign");
  {
    //シェーダーの作成
    Microsoft::WRL::ComPtr<ID3DBlob> vertexShader;
    Microsoft::WRL::ComPtr<ID3DBlob> pixelShader;
    res = D3DCompileFromFile(L"shader/Simple.hlsl", nullptr, nullptr, "VS", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vertexShader, nullptr);
    MYASSERT(res, L"CreateShader");
    res = D3DCompileFromFile(L"shader/Simple.hlsl", nullptr, nullptr, "PS", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelShader, nullptr);
    MYASSERT(res, L"CreateShader");

    //頂点レイアウトの作成
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
      { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
      { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(Vertex::mPos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    //パイプラインステートオブジェクトの作成
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = mRootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    res = mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPipelineState));
    MYASSERT(res, L"CreatePipeState");
  }

  //バーテックスバッファの作成
  //頂点
  InitTrianglePos();
  
  //バーテックスバッファ
  const UINT vertexBufferSize = sizeof(Vertex) * mVertices.size();
  res = mDevice->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&mVertexBuffer));
  MYASSERT(res, L"CreateVBuffCommittedRsrc");

  //バーテックスバッファに頂点データを詰め込む
  UINT8* pVertexDataBegin;
  CD3DX12_RANGE readRange(0, 0);
  res = mVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
  MYASSERT(res, L"Map VBuff");
  memcpy(pVertexDataBegin, &mVertices[0], vertexBufferSize);
  mVertexBuffer->Unmap(0, nullptr);

  //バーテックスバッファビューを初期化
  mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
  mVertexBufferView.StrideInBytes = sizeof(Vertex);
  mVertexBufferView.SizeInBytes = vertexBufferSize;

  //フェンスを作成
  res = mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
  MYASSERT(res, L"CreateFence");
  mFenceValue = 1;

  //コンスタントバッファの作成
  UINT64 CBSize = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;

  //CBV（コンスタントバッファビュー）用のディスクリプターヒープ
  D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
  cbvHeapDesc.NumDescriptors = NUM_POLYGONS;
  cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  res = mDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap));
  MYASSERT(res, L"CreateDescrCbvHeap");
  res = mDevice->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(CBSize * NUM_POLYGONS),
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&mConstantBuffer));
  MYASSERT(res, L"CreateCbvComRsrc");

  //CBV（コンスタントバッファビュー）の作成
  for (UINT64 i = 0; i < NUM_POLYGONS; ++i) {
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = mConstantBuffer->GetGPUVirtualAddress() + i * CBSize;
    cbvDesc.SizeInBytes = CBSize;
    UINT stride = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE cbHandle = mCbvHeap->GetCPUDescriptorHandleForHeapStart();
    cbHandle.ptr += i * stride;
    mDevice->CreateConstantBufferView(&cbvDesc, cbHandle);
  }

  //ここでマップして開いておく（Renderの度にマップしなくていい）
  readRange = CD3DX12_RANGE(0, 0);
  res = mConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mCbvDataBegin));
  MYASSERT(res, L"Map CBuff");
}

void Core::UpdateCamera(const Camera& camera) {
  using namespace DirectX;
  
  //ワールドトランスフォーム
  static float rot = 0;
  const auto transView = XMMatrixLookToRH(XMLoadFloat3(&camera.mPos), XMLoadFloat3(&camera.mForward), XMLoadFloat3(&camera.mUpward));
  const auto transProj = XMMatrixPerspectiveFovRH(3.14159f / 4.0f, static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT), 0.1f, 1000.0f);
  const auto Rot = XMMatrixRotationY(rot);
  rot += 0.0625f;

  for (UINT64 i = 0; i < NUM_POLYGONS; ++i) {
    //コンスタントバッファの内容を更新
    mCBuffers[i].mTransW = Rot * XMMatrixTranslation(mTrianglePos[i].x, mTrianglePos[i].y, mTrianglePos[i].z);
    mCBuffers[i].mTransWVP = XMMatrixTranspose(mCBuffers[i].mTransW * transView * transProj);
    mCBuffers[i].mLightDir = { 0, 0, 1, 0 };
    mCBuffers[i].mDiffuseColor = { 0, 1, 0, 1 };

    UINT8* dstPtr = mCbvDataBegin + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT * i;
    std::memcpy(dstPtr, &mCBuffers[i], sizeof(CBuffer));
  }
  //std::memcpy(mCbvDataBegin, &mCBuffers[0], sizeof(CBuffer) * NUM_POLYGONS);
}


void Core::UpdateCommands() {
  HRESULT res;

  //コマンドリストに書き込む前にはコマンドアロケーターをリセットする
  res = mCommandAllocator->Reset();
  MYASSERT(res, L"CmdAlloc Reset");

  //ここからコマンドリストにコマンドを書き込んでいく
  res = mCommandList->Reset(mCommandAllocator.Get(), mPipelineState.Get());
  MYASSERT(res, L"CmdList Reset");

  //ルートシグネチャをセット
  mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

  //コンスタントバッファをセット
  ID3D12DescriptorHeap* ppHeaps[] = { mCbvHeap.Get() };
  mCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
  mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());

  //ビューポートをセット
  mViewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT));
  mScissorRect = CD3DX12_RECT(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
  mCommandList->RSSetViewports(1, &mViewport);
  mCommandList->RSSetScissorRects(1, &mScissorRect);

  //バックバッファをレンダーターゲットとして使用することを伝える
  mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets[mFrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), mFrameIndex, mRtvDescriptorSize);
  mCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

  //画面クリア
  mCommandList->ClearRenderTargetView(rtvHandle, SCREEN_CLEAR_COLOR, 0, nullptr);

  //ポリゴントポロジーの指定
  mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  //バーテックスバッファをセット
  mCommandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
  int size = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  for (UINT64 i = 0; i < NUM_POLYGONS; ++i) {
    auto cbvSrvUavDescHeap = mCbvHeap->GetGPUDescriptorHandleForHeapStart();
    cbvSrvUavDescHeap.ptr += i * size;
    mCommandList->SetGraphicsRootDescriptorTable(0, cbvSrvUavDescHeap);
    mCommandList->DrawInstanced(3, 1, 0, 0);
  }

  //バックバッファがこのあとPresentされることを伝える
  mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets[mFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

  //コマンドの書き込みはここで終わり、Closeする
  res = mCommandList->Close();
  MYASSERT(res, L"CmdList Close");
}

void Core::Render() {
  HRESULT res;

  //コマンドリストの実行
  ID3D12CommandList* ppCommandLists[] = { mCommandList.Get() };
  mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

  //バックバッファをフロントに持ってきて画面に表示
  res = mSwapChain->Present(1, 0);
  MYASSERT(res, L"Swap Present");
}

void Core::StallForGPU() {
  HRESULT res;

  //現在のフレーム数を保持
  mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

  //GPUサイドが上のExecuteCommandListsで指示した仕事を”全て完了”したときにGPUサイドから発信するシグナル値をセット
  res = mCommandQueue->Signal(mFence.Get(), mFenceValue);
  MYASSERT(res, L"CmdQ Signal");

  //上でセットしたシグナルがGPUから帰ってくるまでストール（この行で待機）
  while (mFence->GetCompletedValue() < mFenceValue)
    ;

  //App側では唯一ここでフェンス値を更新する
  mFenceValue++;
}

bool Core::TerminationRequested() const {
  return mTermination;
}

} // namespace shi62::d3d12