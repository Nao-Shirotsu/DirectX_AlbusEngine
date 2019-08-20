#include <D3Dcompiler.h>

#include "d3d12_Core.hpp"
#include "Constants.hpp"
#include "d3d12_Vertex3d.hpp"

//���C�u�����ǂݍ���
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace {

constexpr auto FRAME_COUNT = 3; //��ʃo�b�t�@��
constexpr float SCREEN_CLEAR_COLOR[] = { 0.75f, 0.75f, 0.75f, 1.0f };

} // anonymous namespace

namespace shi62::d3d12 {

Core::Core(HWND windowHandle)
    : mRenderTargets{ FRAME_COUNT } {
  //D3D12�f�o�C�X�̍쐬
  D3D12CreateDevice(0, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDevice));

  //�R�}���h�L���[�̍쐬
  D3D12_COMMAND_QUEUE_DESC queueDesc = {};
  queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue));

  //�R�}���h���X�g�̍쐬
  mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator));
  mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), NULL, IID_PPV_ARGS(&mCommandList));
  mCommandList->Close();

  //�X���b�v�`�F�[���̍쐬
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
  factory->CreateSwapChainForHwnd(mCommandQueue.Get(), windowHandle, &swapChainDesc, NULL, NULL, (IDXGISwapChain1**)(mSwapChain.GetAddressOf()));
  mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

  //�f�B�X�N���v�^�[�q�[�v�̍쐬�i�����_�[�^�[�Q�b�g�r���[�p�j
  D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
  rtvHeapDesc.NumDescriptors = FRAME_COUNT;
  rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap));
  mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  //�t���[���o�b�t�@�ƃo�b�N�o�b�t�@���ꂼ��Ƀ����_�[�^�[�Q�b�g�r���[���쐬
  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
  for (UINT n = 0; n < FRAME_COUNT; n++) {
    mSwapChain->GetBuffer(n, IID_PPV_ARGS(&mRenderTargets[n]));
    mDevice->CreateRenderTargetView(mRenderTargets[n].Get(), NULL, rtvHandle);
    rtvHandle.Offset(1, mRtvDescriptorSize);
  }

  //���[�g�V�O�l�`���[���쐬
  CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
  rootSignatureDesc.Init(0, NULL, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
  ID3DBlob* signature;
  ID3DBlob* error;
  D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
  mDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mRootSignature));

  //�V�F�[�_�[�̍쐬
  ID3DBlob* vertexShader;
  ID3DBlob* pixelShader;
  D3DCompileFromFile(L"shader/Simple.hlsl", NULL, NULL, "VS", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vertexShader, nullptr);
  D3DCompileFromFile(L"shader/Simple.hlsl", NULL, NULL, "PS", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelShader, nullptr);
  
  //���_���C�A�E�g�̍쐬
  D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
  };

  //�p�C�v���C���X�e�[�g�I�u�W�F�N�g�̍쐬
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

  // ���_
  Vertex3d triangleVertices[] = {
    { { 0.0f, 0.5f, 0.0f } },
    { { 0.5f, -0.5f, 0.0f } },
    { { -0.5f, -0.5f, 0.0f } }
  };

  //�o�[�e�b�N�X�o�b�t�@
  const UINT vertexBufferSize = sizeof(triangleVertices);
  mDevice->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&mVertexBuffer));

  //�o�[�e�b�N�X�o�b�t�@�ɒ��_�f�[�^���l�ߍ���
  UINT8* pVertexDataBegin;
  CD3DX12_RANGE readRange(0, 0);
  mVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
  memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
  mVertexBuffer->Unmap(0, NULL);

  //�o�[�e�b�N�X�o�b�t�@�r���[��������
  mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
  mVertexBufferView.StrideInBytes = sizeof(Vertex3d);
  mVertexBufferView.SizeInBytes = vertexBufferSize;

  //�t�F���X���쐬
  mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
  mFenceValue = 1;
}

void Core::Render() {
  //�R�}���h���X�g�ɏ������ޑO�ɂ̓R�}���h�A���P�[�^�[�����Z�b�g����
  mCommandAllocator->Reset();

  // ------��������R�}���h���X�g�ɃR�}���h����������ł���------
  //�R�}���h���X�g�����Z�b�g����
  mCommandList->Reset(mCommandAllocator.Get(), mPipelineState.Get());

  //���[�g�V�O�l�`�����Z�b�g
  mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

  //�r���[�|�[�g���Z�b�g
  mViewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT));
  mScissorRect = CD3DX12_RECT(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
  mCommandList->RSSetViewports(1, &mViewport);
  mCommandList->RSSetScissorRects(1, &mScissorRect);

  //�o�b�N�o�b�t�@�������_�[�^�[�Q�b�g�Ƃ��Ďg�p���邱�Ƃ�`����
  mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets[mFrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), mFrameIndex, mRtvDescriptorSize);
  mCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, NULL);

  //��ʃN���A
  mCommandList->ClearRenderTargetView(rtvHandle, SCREEN_CLEAR_COLOR, 0, NULL);

  //�|���S���g�|���W�[�̎w��
  mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  //�o�[�e�b�N�X�o�b�t�@���Z�b�g
  mCommandList->IASetVertexBuffers(0, 1, &mVertexBufferView);

  //�`��i���ۂɂ����ŕ`�悳���킯�ł͂Ȃ��B�R�}���h���L�^���邾���j
  mCommandList->DrawInstanced(3, 1, 0, 0);

  //�o�b�N�o�b�t�@�����̂���Present����邱�Ƃ�`����
  mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets[mFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

  //�R�}���h�̏������݂͂����ŏI���AClose����
  mCommandList->Close();

  //�R�}���h���X�g�̎��s
  ID3D12CommandList* ppCommandLists[] = { mCommandList.Get() };
  mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

  //�o�b�N�o�b�t�@���t�����g�Ɏ����Ă��ĉ�ʂɕ\��
  mSwapChain->Present(1, 0);

  //���݉��Ԗڂ̃o�b�N�o�b�t�@���g���Ă��邩
  mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

  // -------��������-------
  //GPU�T�C�h�����ExecuteCommandLists�Ŏw�������d�����h�S�Ċ����h�����Ƃ���GPU�T�C�h���甭�M����V�O�i���l���Z�b�g
  mCommandQueue->Signal(mFence.Get(), mFenceValue);

  //��ŃZ�b�g�����V�O�i����GPU����A���Ă���܂ŃX�g�[���i���̍s�őҋ@�j
  while (mFence->GetCompletedValue() < mFenceValue);

  //App���ł͗B�ꂱ���Ńt�F���X�l���X�V����
  mFenceValue++;
}

bool Core::TerminationRequested() const{
  return false;
}

} // namespace shi62::d3d12