#pragma once

#include <DirectXMath.h>

namespace shi62::d3d12 {

struct CBuffer {
  DirectX::XMMATRIX mTransMatrix;
  CBuffer();
  CBuffer(const DirectX::XMMATRIX& transMatrix);
};

} // namespace shi62::d3d12