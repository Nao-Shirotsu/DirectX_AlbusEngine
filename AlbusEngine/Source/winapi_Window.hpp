#pragma once
#include <windows.h>
#include <d3d11.h>

namespace shi62::winapi{

// 画面に表示するウィンドウのクラス
class Window{
public:
    Window( HINSTANCE instanceHandle, LPCWSTR windowClassName, LPCWSTR windowTitle, const int width, const int height );
    ~Window();

    // ループ処理一回分を実行
    auto Update() -> void;

    // ウィンドウ終了処理が始まるときtrue
    auto TerminationRequested() -> bool const;

private:
    //====D3D11 samples tutorial1のコピペ====
    auto InitDevice() -> HRESULT;
    auto CleanupDevice() -> void; 
    auto Render() -> void;
    
    D3D_DRIVER_TYPE         mDriverType;
    D3D_FEATURE_LEVEL       mFeatureLevel;
    ID3D11Device*           mD3dDevice;
    ID3D11DeviceContext*    mImmediateContext;
    IDXGISwapChain*         mSwapChain;
    ID3D11RenderTargetView* mRenderTargetView;
    //======================================

    HINSTANCE mInstanceHandle;
    LPCWSTR   mWindowClassName;
    LPCWSTR   mWindowTitle; // Windowタイトルバーに表示する文字列
    HWND      mWindowHandle;
    MSG       mMessage;
    BOOL      mMessageState; // メッセージ受信時に終了/エラーを検出する変数
};

// キーボードのいずれかのキー押下時のメッセージ処理関数
auto ProcessKeydownMessage( HWND windowHandle, WPARAM wParam ) -> void;

}