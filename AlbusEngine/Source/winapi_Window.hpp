#pragma once
#include <Windows.h>

namespace shi62::winapi{

// 画面に表示するウィンドウのクラス
class Window{
public:
    Window( const char* windowClassName, const char* windowTitle, const int width, const int height );
    ~Window();

    // ループ処理一回分を実行
    void Update();

    // ウィンドウをterminateするか否か
    bool TerminationRequested() const;

private:
    HINSTANCE mInstanceHandle;
    LPCSTR mWindowClassName;
    LPCSTR mWindowTitle; // Windowタイトルバーに表示する文字列
    HWND mWindowHandle;
    MSG mMessage;
    BOOL mMessageState; // メッセージ受信時に終了/エラーを検出する変数
};

// キーボードのいずれかのキー押下時のメッセージ処理関数
void ProcessKeydownMessage( HWND windowHandle, WPARAM wParam );

}