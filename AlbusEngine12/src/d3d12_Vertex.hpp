#pragma once

#include "d3d12_Vector3f.hpp"

namespace shi62::d3d12 {

struct Vertex {
  inline Vertex(const Vector3f& pos, const Vector3f& normal)
      : mPos{ pos }
      , mNormal{ normal } {}
  Vector3f mPos;
  Vector3f mNormal;
};

} // namespace shi62::d3d12