#pragma once

#include <DirectXMath.h>

namespace shi62::d3d12 {

struct Camera {
  constexpr Camera(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& forward, const DirectX::XMFLOAT3& upward)
      : mPos{ pos }
      , mForward{ forward }
      , mUpward{upward} {}

  DirectX::XMFLOAT3 mPos;
  DirectX::XMFLOAT3 mForward;
  DirectX::XMFLOAT3 mUpward;
};

} // namespace shi62::d3d12