#pragma once

#include <DirectXMath.h>

namespace shi62::d3d12 {

struct CBuffer {
  DirectX::XMMATRIX mTransWVP;
  DirectX::XMMATRIX mTransW;
  DirectX::XMFLOAT4 mLightDir;
  DirectX::XMFLOAT4 mDiffuseColor;
  inline CBuffer()
      : mTransWVP{ 0, 0, 0, 0,
                   0, 0, 0, 0,
                   0, 0, 0, 0,
                   0, 0, 0, 0 }
      , mTransW{ 0, 0, 0, 0,
                 0, 0, 0, 0,
                 0, 0, 0, 0,
                 0, 0, 0, 0 }
      , mLightDir{ 0, 0, 0, 0 }
      , mDiffuseColor{ 1, 0, 0, 0 } {};
};

} // namespace shi62::d3d12