#pragma once

#include <vector>
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_4.h>
#include <wrl.h>

namespace shi62::d3d12 {

class Core {
public:
  Core(HWND windowHandle);

  void Render();

  [[nodiscard]] bool TerminationRequested() const;

private:
  template <class T>
  using ComPtr = Microsoft::WRL::ComPtr<T>;
  ComPtr<IDXGISwapChain3> mSwapChain;
  ComPtr<ID3D12Device> mDevice;
  std::vector<ComPtr<ID3D12Resource>> mRenderTargets;
  ComPtr<ID3D12CommandAllocator> mCommandAllocator;
  ComPtr<ID3D12CommandQueue> mCommandQueue;
  ComPtr<ID3D12DescriptorHeap> mRtvHeap;
  ComPtr<ID3D12PipelineState> mPipelineState;
  ComPtr<ID3D12GraphicsCommandList> mCommandList;
  UINT mRtvDescriptorSize;
  UINT mFrameIndex;
  ComPtr<ID3D12Fence> mFence;
  UINT64 mFenceValue;
  ComPtr<ID3D12RootSignature> mRootSignature;
  CD3DX12_VIEWPORT mViewport;
  CD3DX12_RECT mScissorRect;
  ComPtr<ID3D12Resource> mVertexBuffer;
  D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
};

} // namespace shi62::d3d12