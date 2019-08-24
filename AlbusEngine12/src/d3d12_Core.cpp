#include <D3Dcompiler.h>
#include <DirectXMath.h>

#include "Constants.hpp"
#include "d3d12_CBuffer.hpp"
#include "d3d12_Core.hpp"
#include "d3d12_Vector3f.hpp"

//ライブラリ読み込み
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace {

constexpr auto FRAME_COUNT = 3; //画面バッファ数
constexpr float SCREEN_CLEAR_COLOR[] = { 0.75f, 0.75f, 0.75f, 1.0f };

} // anonymous namespace

namespace shi62::d3d12 {

Core::Core(const HWND windowHandle)
    : mTermination{ false }
    , mRenderTargets{ FRAME_COUNT }
    , mCbvDataBegin{ nullptr } {
  //D3D12デバイスの作成
  D3D12CreateDevice(0, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDevice));
  //コマンドキューの作成
  D3D12_COMMAND_QUEUE_DESC queueDesc = {};
  queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue));
  //コマンドリストの作成
  mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator));
  mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&mCommandList));
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
  CreateDXGIFactory1(IID_PPV_ARGS(&factory));
  factory->CreateSwapChainForHwnd(mCommandQueue.Get(), windowHandle, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(mSwapChain.GetAddressOf()));
  mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
  //ディスクリプターヒープの作成（レンダーターゲットビュー用）
  D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
  rtvHeapDesc.NumDescriptors = FRAME_COUNT;
  rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap));
  mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  //フレームバッファとバックバッファそれぞれにレンダーターゲットビューを作成
  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
  for (UINT n = 0; n < FRAME_COUNT; n++) {
    mSwapChain->GetBuffer(n, IID_PPV_ARGS(&mRenderTargets[n]));
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
  D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
  mDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mRootSignature));
  //シェーダーの作成
  ID3DBlob* vertexShader;
  ID3DBlob* pixelShader;
  D3DCompileFromFile(L"shader/Simple.hlsl", nullptr, nullptr, "VS", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vertexShader, nullptr);
  D3DCompileFromFile(L"shader/Simple.hlsl", nullptr, nullptr, "PS", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelShader, nullptr);
  //頂点レイアウトの作成
  D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
  };
  //パイプラインステートオブジェクトの作成
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
  psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
  psoDesc.pRootSignature = mRootSignature.Get();
  psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
  psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);
  psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  psoDesc.DepthStencilState.DepthEnable = FALSE;
  psoDesc.DepthStencilState.StencilEnable = FALSE;
  psoDesc.SampleMask = UINT_MAX;
  psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  psoDesc.NumRenderTargets = 1;
  psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  psoDesc.SampleDesc.Count = 1;
  mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPipelineState));
  //バーテックスバッファの作成
  //頂点
  constexpr Vector3f triangleVertices[] = {
    { { 0.0f, 0.5f, 0.0f } },
    { { 0.5f, -0.5f, 0.0f } },
    { { -0.5f, -0.5f, 0.0f } }
  };

  //バーテックスバッファ
  const UINT vertexBufferSize = sizeof(triangleVertices);
  mDevice->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&mVertexBuffer));
  //バーテックスバッファに頂点データを詰め込む
  UINT8* pVertexDataBegin;
  CD3DX12_RANGE readRange(0, 0);
  mVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
  memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
  mVertexBuffer->Unmap(0, nullptr);
  //バーテックスバッファビューを初期化
  mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
  mVertexBufferView.StrideInBytes = sizeof(Vector3f);
  mVertexBufferView.SizeInBytes = vertexBufferSize;
  //フェンスを作成
  mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
  mFenceValue = 1;
  //コンスタントバッファの作成
  UINT CBSize = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
  //CBV（コンスタントバッファビュー）用のディスクリプターヒープ
  D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
  cbvHeapDesc.NumDescriptors = 1;
  cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  mDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap));

  mDevice->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(CBSize * 1),
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&mConstantBuffer));

  //CBV（コンスタントバッファビュー）の作成
  D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
  cbvDesc.BufferLocation = mConstantBuffer->GetGPUVirtualAddress();
  cbvDesc.SizeInBytes = CBSize;
  mDevice->CreateConstantBufferView(&cbvDesc, mCbvHeap->GetCPUDescriptorHandleForHeapStart());
  //ここでマップして開いておく（Renderの度にマップしなくていい）
  readRange = CD3DX12_RANGE(0, 0);
  mConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mCbvDataBegin));
}

void Core::UpdateCamera(const Camera& camera) {
  using namespace DirectX;

  //ワールドトランスフォーム
  static float rot = 0;
  const auto transWorld = XMMatrixRotationY(rot += 0.1f); //単純にyaw回転させる
  const auto transView = XMMatrixLookToRH(XMLoadFloat3(&camera.mPos), XMLoadFloat3(&camera.mForward), XMLoadFloat3(&camera.mUpward));
  const auto transProj = XMMatrixPerspectiveFovRH(3.14159f / 4.0f, ( FLOAT )WINDOW_WIDTH / ( FLOAT )WINDOW_HEIGHT, 0.1f, 1000.0f);

  //コンスタントバッファの内容を更新
  memcpy(mCbvDataBegin, &CBuffer{ XMMatrixTranspose(transWorld * transView * transProj) }, sizeof(CBuffer));
}


void Core::UpdateCommands() {
  //コマンドリストに書き込む前にはコマンドアロケーターをリセットする
  mCommandAllocator->Reset();
  //ここからコマンドリストにコマンドを書き込んでいく
  mCommandList->Reset(mCommandAllocator.Get(), mPipelineState.Get());
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
  //描画（実際にここで描画されるわけではない。コマンドを記録するだけ）
  mCommandList->DrawInstanced(3, 1, 0, 0);
  //バックバッファがこのあとPresentされることを伝える
  mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets[mFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
  //コマンドの書き込みはここで終わり、Closeする
  mCommandList->Close();
}

void Core::Render() {
  //コマンドリストの実行
  ID3D12CommandList* ppCommandLists[] = { mCommandList.Get() };
  mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
  //バックバッファをフロントに持ってきて画面に表示
  mSwapChain->Present(1, 0);
}

void Core::StallForGPU() {
  //現在のフレーム数を保持
  mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

  //GPUサイドが上のExecuteCommandListsで指示した仕事を”全て完了”したときにGPUサイドから発信するシグナル値をセット
  mCommandQueue->Signal(mFence.Get(), mFenceValue);

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