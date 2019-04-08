#include <crtdbg.h>
#include <d3dcompiler.h>
#include <d3dx11async.h>
#include <xnamath.h>

#include "winapi_Window.hpp"

namespace shi62::winapi{

Window::Window( HINSTANCE instanceHandle, LPCWSTR windowClassName, LPCWSTR windowTitle, const int width, const int height ):
    mInstanceHandle( instanceHandle ),
    mWindowClassName( windowClassName ),
    mWindowTitle( windowTitle ),
    mMessage( { 0 } ),
    mMessageState( TRUE ),
    mDriverType( D3D_DRIVER_TYPE_NULL ),
    mFeatureLevel( D3D_FEATURE_LEVEL_11_0 ),
    mD3dDevice( nullptr ),
    mImmediateContext( nullptr ),
    mSwapChain( nullptr ),
    mRenderTargetView( nullptr ),
    mVertexShader( nullptr ),
    mPixelShader( nullptr ),
    mVertexLayout( nullptr ),
    mVertexBuffer( nullptr ){
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
}

Window::~Window(){
    UnregisterClass( mWindowClassName, mInstanceHandle );
    CleanupDevice();
    mWindowHandle = nullptr;
}

auto Window::Update() -> void{
    mMessageState = PeekMessage( &mMessage, nullptr, 0, 0, PM_REMOVE );
    if( mMessageState != 0 ){
        TranslateMessage( &mMessage );
        DispatchMessage( &mMessage );
    }
    else{
        Render();
    }
}

auto Window::TerminationRequested() -> bool const{
    return mMessage.message == WM_QUIT || mMessageState == -1; // 終了時とエラー発生時
}

auto CompileShaderFromFile( const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut ) -> HRESULT{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DX11CompileFromFile( szFileName, NULL, NULL, szEntryPoint, szShaderModel,
                                dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
    if( FAILED( hr ) )
    {
        if( pErrorBlob != NULL )
            OutputDebugStringA( ( char* ) pErrorBlob->GetBufferPointer() );
        if( pErrorBlob ) pErrorBlob->Release();
        return hr;
    }
    if( pErrorBlob ) pErrorBlob->Release();

    return S_OK;
}

auto Window::InitDevice() -> HRESULT{
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

    // Compile the vertex shader
    ID3DBlob* pVSBlob = NULL;
    hr = CompileShaderFromFile( L"Tutorial02.fx", "VS", "vs_4_0", &pVSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( NULL,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

    // Create the vertex shader
    hr = mD3dDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &mVertexShader );
    if( FAILED( hr ) )
    {
        pVSBlob->Release();
        return hr;
    }

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE( layout );

    // Create the input layout
    hr = mD3dDevice->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
                                          pVSBlob->GetBufferSize(), &mVertexLayout );
    pVSBlob->Release();
    if( FAILED( hr ) )
        return hr;

    // Set the input layout
    mImmediateContext->IASetInputLayout( mVertexLayout );

    // Compile the pixel shader
    ID3DBlob* pPSBlob = NULL;
    hr = CompileShaderFromFile( L"Tutorial02.fx", "PS", "ps_4_0", &pPSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( NULL,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

    // Create the pixel shader
    hr = mD3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &mPixelShader );
    pPSBlob->Release();
    if( FAILED( hr ) )
        return hr;

    // Create vertex buffer
    XMFLOAT3 vertices[] =
    {
        XMFLOAT3( 0.0f, 0.5f, 0.5f ),
        XMFLOAT3( 0.5f, -0.5f, 0.5f ),
        XMFLOAT3( -0.5f, -0.5f, 0.5f ),
    };
    D3D11_BUFFER_DESC bd;
    ZeroMemory( &bd, sizeof( bd ) );
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof( XMFLOAT3 ) * 3;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory( &InitData, sizeof( InitData ) );
    InitData.pSysMem = vertices;
    hr = mD3dDevice->CreateBuffer( &bd, &InitData, &mVertexBuffer );
    if( FAILED( hr ) )
        return hr;

    // Set vertex buffer
    UINT stride = sizeof( XMFLOAT3 );
    UINT offset = 0;
    mImmediateContext->IASetVertexBuffers( 0, 1, &mVertexBuffer, &stride, &offset );

    // Set primitive topology
    mImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    return S_OK;
}

auto Window::CleanupDevice() -> void{
    if( mImmediateContext ) mImmediateContext->ClearState();
    if( mVertexBuffer ) mVertexBuffer->Release();
    if( mVertexLayout ) mVertexLayout->Release();
    if( mVertexShader ) mVertexShader->Release();
    if( mPixelShader ) mPixelShader->Release();
    if( mRenderTargetView ) mRenderTargetView->Release();
    if( mSwapChain ) mSwapChain->Release();
    if( mImmediateContext ) mImmediateContext->Release();
    if( mD3dDevice ) mD3dDevice->Release();
}

auto Window::Render() -> void{
    // Clear the back buffer 
    float ClearColor[4] = { 0.0f, 0.125f, 0.25f, 0.75f }; // red,green,blue,alpha
    mImmediateContext->ClearRenderTargetView( mRenderTargetView, ClearColor );

    // Render a triangle
    mImmediateContext->VSSetShader( mVertexShader, NULL, 0 );
    mImmediateContext->PSSetShader( mPixelShader, NULL, 0 );
    mImmediateContext->Draw( 3, 0 );

    // Present the information rendered to the back buffer to the front buffer (the screen)
    mSwapChain->Present( 0, 0 );
}

auto ProcessKeydownMessage( HWND windowHandle, WPARAM wParam ) -> void{
    switch( wParam ){
    case VK_ESCAPE:
        PostMessage( windowHandle, WM_CLOSE, 0, 0 );
        break;
    }
}

}