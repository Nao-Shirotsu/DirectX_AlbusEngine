#pragma once
#include <windows.h>

namespace shi62::winapi{

// winapi::Windowが持つウィンドウハンドルを他クラスが保持するためのインターフェース
// このクラスを継承するクラスのみがwinapi::Window::mWindowHandleをコピー代入できる
class HwndHolder {
public:    
    // 何もしない
    HwndHolder();

    // mWindowHandleをセット
    auto SetHwnd(HWND windowHandle) -> void;

protected:
    HWND mWindowHandle;
};

} // namespace shi62::winapi