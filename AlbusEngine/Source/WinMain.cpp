#include "winapi_Window.hpp"

int WINAPI wWinMain( HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPWSTR lpCmdLine,
                     int nCmdShow ){
    shi62::winapi::Window window( "win1", "your window", 400, 300 );
    while( !window.TerminationRequested() ){
        window.Update();
    }

    return 0;
}