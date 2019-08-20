#pragma once
#include <windows.h>

namespace shi62::winapi {

// 画面に表示するウィンドウのクラス
class Window {
public:
  Window(HINSTANCE instanceHandle, LPCWSTR windowClassName, LPCWSTR windowTitle, const int width, const int height);

  ~Window();

  // ループ処理一回分を実行
  void Update();

  // ウィンドウ終了処理が始まるときtrue
  [[nodiscard]] bool TerminationRequested() const;

  // mWindowHandleをぶん投げる
  [[nodiscard]] HWND GetWindowHandle() const;

private:
  HINSTANCE mInstanceHandle;
  LPCWSTR mWindowClassName;
  LPCWSTR mWindowTitle; // Windowタイトルバーに表示する文字列
  HWND mWindowHandle;
  MSG mMessage;
  BOOL mMessageState; // メッセージ受信時に終了/エラーを検出する変数
};

// キーボードのいずれかのキー押下時のメッセージ処理関数
void ProcessKeydownMessage(HWND windowHandle, WPARAM wParam);

} // namespace shi62:winapi