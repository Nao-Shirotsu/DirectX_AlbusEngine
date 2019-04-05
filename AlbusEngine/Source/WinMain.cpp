#include "winapi_Window.hpp"

int WINAPI wWinMain( HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPWSTR lpCmdLine,
                     int nCmdShow ){

    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );
    UNREFERENCED_PARAMETER( nCmdShow );

    shi62::winapi::Window window( hInstance, L"win1", L"D3D11 samples Tutorial 1", 400, 300 );
    while( !window.TerminationRequested() ){
        window.Update();
    }

    return 0;
}