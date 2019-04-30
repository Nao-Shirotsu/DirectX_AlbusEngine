#include <crtdbg.h>

#include "d3d11_Core.hpp"
#include "winapi_Window.hpp"

namespace shi62::winapi {

Window::Window(HINSTANCE instanceHandle, LPCWSTR windowClassName, LPCWSTR windowTitle, const int width, const int height)
    : mInstanceHandle(instanceHandle)
    , mWindowClassName(windowClassName)
    , mWindowTitle(windowTitle)
    , mMessage({ 0 })
    , mMessageState(TRUE) {
  WNDCLASS windowClass;
  windowClass.style = CS_HREDRAW | CS_VREDRAW;
  windowClass.lpfnWndProc = [](HWND windowHandle,
                               UINT message,
                               WPARAM wParam,
                               LPARAM lParam) -> LRESULT CALLBACK {
    switch (message) {
    case WM_PAINT:
      PAINTSTRUCT paintStruct;
      HDC hdc;
      hdc = BeginPaint(windowHandle, &paintStruct);
      EndPaint(windowHandle, &paintStruct);
      break;

    case WM_DESTROY:
      PostQuitMessage(0);
      break;

    case WM_KEYDOWN:
      ProcessKeydownMessage(windowHandle, wParam);
      break;

    default:
      break;
    }

    return DefWindowProc(windowHandle, message, wParam, lParam);
  };
  windowClass.cbClsExtra = 0;
  windowClass.cbWndExtra = 0;
  windowClass.hInstance = mInstanceHandle;
  windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
  windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
  windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
  windowClass.lpszMenuName = nullptr;
  windowClass.lpszClassName = windowClassName;

  auto res = RegisterClass(&windowClass);

  // ウィンドウクラス登録失敗でASSERT
  _ASSERT_EXPR(res, TEXT("Class Registration Error"));

  RECT windowRect = { 0, 0, width, height };
  AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, TRUE);

  mWindowHandle = CreateWindow(mWindowClassName,
                               mWindowTitle,
                               WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT,
                               CW_USEDEFAULT,
                               windowRect.right - windowRect.left,
                               windowRect.bottom - windowRect.top,
                               nullptr,
                               nullptr,
                               mInstanceHandle,
                               nullptr);

  // ウィンドウ生成失敗でASSERT
  _ASSERT_EXPR(mWindowHandle, TEXT("Window Creation Error"));

  ShowWindow(mWindowHandle, SW_SHOWNORMAL);
  UpdateWindow(mWindowHandle);
}

Window::~Window() {
  UnregisterClass(mWindowClassName, mInstanceHandle);
  mWindowHandle = nullptr;
}

auto Window::Update() -> void {
  mMessageState = PeekMessage(&mMessage, nullptr, 0, 0, PM_REMOVE);
  if (!mMessageState) {
    return;
  }
  TranslateMessage(&mMessage);
  DispatchMessage(&mMessage);
}

auto Window::TerminationRequested() -> bool const {
  return mMessage.message == WM_QUIT || mMessageState == -1; // 終了時とエラー発生時
}

auto ProcessKeydownMessage(HWND windowHandle, WPARAM wParam) -> void {
  switch (wParam) {
  case VK_ESCAPE:
    PostMessage(windowHandle, WM_CLOSE, 0, 0);
    break;
  }
}

}

auto shi62::winapi::Window::GetWindowHandle() -> HWND const {
  return mWindowHandle;
}
