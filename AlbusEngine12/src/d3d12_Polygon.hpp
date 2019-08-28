#pragma once

#include <array>
#include "d3d12_Vertex.hpp"

namespace shi62::d3d12 {

struct Polygon {
  Vector3f mPos;
  Vector3f mNormal;
  std::array<Vertex, 3> mVertices;
};

} // namespace shi62::d3d12