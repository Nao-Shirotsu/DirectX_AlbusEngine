#include <crtdbg.h>

#include "winapi_Window.hpp"

shi62::winapi::Window::Window( HINSTANCE instanceHandle, LPCWSTR windowClassName, LPCWSTR windowTitle, const int width, const int height ):
    mInstanceHandle( instanceHandle ),
    mWindowClassName( windowClassName ),
    mWindowTitle( windowTitle ),
    mMessage(),
    mMessageState( TRUE ),
    mDriverType( D3D_DRIVER_TYPE_NULL ),
    mFeatureLevel( D3D_FEATURE_LEVEL_11_0 ),
    mD3dDevice( nullptr ),
    mImmediateContext( nullptr ),
    mSwapChain( nullptr ),
    mRenderTargetView( nullptr ){
    WNDCLASS windowClass;
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = []( HWND windowHandle,
                                  UINT message,
                                  WPARAM wParam,
                                  LPARAM lParam ) -> LRESULT CALLBACK{
        switch( message ){
        case WM_PAINT:
            PAINTSTRUCT paintStruct;
            HDC hdc;
            hdc = BeginPaint( windowHandle, &paintStruct );
            EndPaint( windowHandle, &paintStruct );
            break;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;

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

    RECT windowRect = { 0, 0, width, height };
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

    if( auto res = FAILED( InitDevice() ) ){
        CleanupDevice();
        _ASSERT_EXPR( res != 0, TEXT( "Device Initialization Error" ) );
    }

    mMessageState = PeekMessage( &mMessage, nullptr, 0, 0, PM_REMOVE );
}

shi62::winapi::Window::~Window(){
    UnregisterClass( mWindowClassName, mInstanceHandle );
    CleanupDevice();
    mWindowHandle = nullptr;
}

auto shi62::winapi::Window::Update() -> void{
    if( mMessageState != 0 ){
        TranslateMessage( &mMessage );
        DispatchMessage( &mMessage );
    }
    else{
        Render();
    }
    mMessageState = PeekMessage( &mMessage, nullptr, 0, 0, PM_REMOVE );
}

auto shi62::winapi::Window::TerminationRequested() -> bool const{
    return mMessage.message == WM_QUIT || mMessageState == -1; // エラー発生時
}

auto shi62::winapi::Window::InitDevice() -> HRESULT{
    HRESULT hr = S_OK;

    RECT rect;
    GetClientRect( mWindowHandle, &rect );
    UINT width = rect.right - rect.left;
    UINT height = rect.bottom - rect.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = mWindowHandle;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        mDriverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain( NULL, mDriverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
                                            D3D11_SDK_VERSION, &sd, &mSwapChain, &mD3dDevice, &mFeatureLevel, &mImmediateContext );
        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = NULL;
    hr = mSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* ) & pBackBuffer );
    if( FAILED( hr ) )
        return hr;

    hr = mD3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &mRenderTargetView );
    pBackBuffer->Release();
    if( FAILED( hr ) )
        return hr;

    mImmediateContext->OMSetRenderTargets( 1, &mRenderTargetView, NULL );

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = ( FLOAT ) width;
    vp.Height = ( FLOAT ) height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    mImmediateContext->RSSetViewports( 1, &vp );

    return S_OK;
}

auto shi62::winapi::Window::CleanupDevice() -> void{
    if( mImmediateContext ){ mImmediateContext->ClearState(); }
    if( mRenderTargetView ){ mRenderTargetView->Release(); }
    if( mSwapChain ){ mSwapChain->Release(); }
    if( mImmediateContext ){ mImmediateContext->Release(); }
    if( mD3dDevice ){ mD3dDevice->Release(); }
}

auto shi62::winapi::Window::Render() -> void{
    // Just clear the backbuffer

    // ==========test=========== 色変えたいだけ
    static float unit = 0.01f;
    static int count = 0;
    float diff = count * unit;
    count++;
    if( count > 98 ){ count = 0; }
    // ==========test===========

    float ClearColor[4] = { 0.25f, 0.875f, 0.875f }; //red,green,blue,alpha
    mImmediateContext->ClearRenderTargetView( mRenderTargetView, ClearColor );
    mSwapChain->Present( 0, 0 );
}

auto shi62::winapi::ProcessKeydownMessage( HWND windowHandle, WPARAM wParam ) -> void{
    switch( wParam ){
    case VK_ESCAPE:
        PostMessage( windowHandle, WM_CLOSE, 0, 0 );
        break;
    }
}
