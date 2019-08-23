#include <Windows.h>

#include "d3d12_CBuffer.hpp"

namespace shi62::d3d12 {

CBuffer::CBuffer()
    : mTransMatrix{ 0, 0, 0, 0,
                    0, 0, 0, 0,
                    0, 0, 0, 0,
                    0, 0, 0, 0 } {
}

CBuffer::CBuffer(const DirectX::XMMATRIX& transMatrix)
    : mTransMatrix{ transMatrix } {
}

} // namespace shi62::d3d12