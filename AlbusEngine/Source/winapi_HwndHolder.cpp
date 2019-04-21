#include "winapi_HwndHolder.hpp"

namespace shi62::winapi {

HwndHolder::HwndHolder() = default;

auto HwndHolder::SetHwnd(HWND windowHandle) -> void {
    mWindowHandle = windowHandle;
}

}
