#pragma once

#include <DirectXMath.h>
#include <array>

namespace shi62::d3d12 {

struct Vector3f {
  constexpr Vector3f(std::array<float, 3> data)
      : mData{ data[0], data[1], data[2] } {}

  Vector3f& operator=(const Vector3f& other) {
    mData.x = other.mData.x;
    mData.y = other.mData.y;
    mData.z = other.mData.z;
  }

  DirectX::XMFLOAT3 mData;
};

} // namespace namespace shi62::d3d12