#include <crtdbg.h>

#include "winapi_Window.hpp"

namespace shi62::winapi {

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);

// キーボードのいずれかのキー押下時のメッセージ処理関数
void ProcessKeydownMessage(HWND windowHandle, WPARAM wParam);

Window::Window(HINSTANCE instanceHandle, LPCWSTR windowClassName, LPCWSTR windowTitle, const int width, const int height)
    : mInstanceHandle(instanceHandle)
    , mWindowClassName(windowClassName)
    , mWindowTitle(windowTitle)
    , mMessage({ 0 })
    , mMessageState(TRUE) {
  WNDCLASSEXW windowClass;
  windowClass.cbSize = sizeof(WNDCLASSEXW);
  windowClass.style = CS_HREDRAW | CS_VREDRAW;
  windowClass.lpfnWndProc = WindowProcedure;
  windowClass.cbClsExtra = 0;
  windowClass.cbWndExtra = 0;
  windowClass.hInstance = mInstanceHandle;
  windowClass.hIcon = LoadIcon(mInstanceHandle, IDI_APPLICATION);
  windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
  windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
  windowClass.lpszMenuName = nullptr;
  windowClass.lpszClassName = windowClassName;
  windowClass.hIconSm = nullptr;

  ATOM res = RegisterClassExW(&windowClass);

  // ウィンドウクラス登録失敗でASSERT
  _ASSERT_EXPR(res, TEXT("Class Registration Error"));

  RECT windowRect = { 0, 0, width, height };
  AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, TRUE);

  mWindowHandle = CreateWindowW(mWindowClassName,
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

void Window::Update() {
  mMessageState = PeekMessage(&mMessage, nullptr, 0, 0, PM_REMOVE);
  if (!mMessageState) {
    return;
  }
  TranslateMessage(&mMessage);
  DispatchMessage(&mMessage);
}

bool Window::TerminationRequested() const {
  return mMessage.message == WM_QUIT || mMessageState == -1; // 終了時とエラー発生時
}

HWND Window::GetWindowHandle() const {
  return mWindowHandle;
}

void ProcessKeydownMessage(HWND windowHandle, WPARAM wParam) {
  switch (wParam) {
    case VK_ESCAPE: {
      PostMessage(windowHandle, WM_CLOSE, 0, 0);
      break;
    }
  }
}

LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hWnd, &ps);
      // TODO: HDC を使用する描画コードをここに追加してください...
      EndPaint(hWnd, &ps);
      break;
    }

    case WM_DESTROY: {
      PostQuitMessage(0);
      break;
    }

    case WM_KEYDOWN: {
      ProcessKeydownMessage(hWnd, wParam);
      break;
    }

    default: {
      break;
    }
  }
  return DefWindowProc(hWnd, message, wParam, lParam);
}

} // namespace shi62::winapi
