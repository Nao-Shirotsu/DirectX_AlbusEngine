#pragma once

#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_4.h>

//�萔��`
namespace {

constexpr auto WINDOW_WIDTH = 800;  //�E�B���h�E��
constexpr auto WINDOW_HEIGHT = 450; //�E�B���h�E����
constexpr auto FRAME_COUNT = 3;     //��ʃo�b�t�@��

}

namespace shi62::d3d12 {

class Core {
public:
  Core(HWND windowHandle);

  void Render();

  [[nodiscard]] bool TerminationRequested() const;

private:
  IDXGISwapChain3* mSwapChain;
  ID3D12Device* mDevice;
  ID3D12Resource* mRenderTargets[FRAME_COUNT];
  ID3D12CommandAllocator* mCommandAllocator;
  ID3D12CommandQueue* mCommandQueue;
  ID3D12DescriptorHeap* mRtvHeap;
  ID3D12PipelineState* mPipelineState;
  ID3D12GraphicsCommandList* mCommandList;
  UINT mRtvDescriptorSize;
  UINT mFrameIndex;
  ID3D12Fence* mFence;
  UINT64 mFenceValue;
  ID3D12RootSignature* mRootSignature;
  CD3DX12_VIEWPORT mViewport;
  CD3DX12_RECT mScissorRect;
};

} // namespace shi62::d3d12