#include <crtdbg.h>

#include "winapi_Window.hpp"

shi62::winapi::Window::Window( const char* windowClassName, const char* windowTitle, const int width, const int height ):
    mWindowClassName( TEXT( windowClassName ) ),
    mWindowTitle( TEXT( windowTitle ) ),
    mMessage(),
    mMessageState( TRUE ){
    WNDCLASS windowClass;
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = []( HWND windowHandle,
                                  UINT message,
                                  WPARAM wParam,
                                  LPARAM lParam ) -> LRESULT CALLBACK{
        switch( message ){
        case WM_DESTROY:
            PostQuitMessage( 0 );
            windowHandle = nullptr;
            return 0;

        case WM_KEYDOWN:
            ProcessKeydownMessage( windowHandle, wParam );
            break;

        default:
            break;
        }

        return DefWindowProc( windowHandle, message, wParam, lParam );
    };
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = mInstanceHandle;
    windowClass.hIcon = LoadIcon( nullptr, IDI_APPLICATION );
    windowClass.hCursor = LoadCursor( nullptr, IDC_ARROW );
    windowClass.hbrBackground = static_cast< HBRUSH >( GetStockObject( BLACK_BRUSH ) );
    windowClass.lpszMenuName = nullptr;
    windowClass.lpszClassName = windowClassName;

    // ウィンドウクラス登録失敗でASSERT
    _ASSERT_EXPR( RegisterClass( &windowClass ), TEXT( "Class Registration Error" ) );

    RECT windowRect;
    windowRect.top = 0;
    windowRect.left = 0;
    windowRect.right = width;
    windowRect.bottom = height;
    AdjustWindowRect( &windowRect, WS_OVERLAPPEDWINDOW, TRUE );

    mWindowHandle = CreateWindow( mWindowClassName,
                                  mWindowTitle,
                                  WS_OVERLAPPEDWINDOW,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  windowRect.right - windowRect.left,
                                  windowRect.bottom - windowRect.top,
                                  nullptr,
                                  nullptr,
                                  mInstanceHandle,
                                  nullptr );

    // ウィンドウ生成失敗でASSERT
    _ASSERT_EXPR( mWindowHandle, TEXT( "Window Creation Error" ) );

    ShowWindow( mWindowHandle, SW_SHOWNORMAL );
    UpdateWindow( mWindowHandle );

    mMessageState = GetMessage( &mMessage, mWindowHandle, 0, 0 );
}

shi62::winapi::Window::~Window(){
    UnregisterClass( mWindowClassName, mInstanceHandle );
}

void shi62::winapi::Window::Update(){
    TranslateMessage( &mMessage );
    DispatchMessage( &mMessage );
    mMessageState = GetMessage( &mMessage, mWindowHandle, 0, 0 );
}

bool shi62::winapi::Window::TerminationRequested() const{
    return !mMessageState || mMessageState == -1; // 終了時かエラー発生時
}

void shi62::winapi::ProcessKeydownMessage( HWND windowHandle, WPARAM wParam ){
    switch( wParam ){
    case VK_ESCAPE:
        PostMessage( windowHandle, WM_CLOSE, 0, 0 );
        break;
    }
}
