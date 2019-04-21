#include "winapi_Window.hpp"
#include "d3d11_Core.hpp"

int WINAPI wWinMain(HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPWSTR lpCmdLine,
                    int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    shi62::winapi::Window window(hInstance, L"win1", L"D3D11 自転立方体", 400, 300);
    shi62::d3d11::Core d3dCore;
    window.InjectHwnd(d3dCore);
    d3dCore.Init();

    while (!window.TerminationRequested()) {
        window.Update();
        d3dCore.Update();
        d3dCore.Draw();
    }

    return 0;
}