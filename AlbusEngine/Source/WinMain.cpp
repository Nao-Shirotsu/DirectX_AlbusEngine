#include "d3d11_Core.hpp"
#include "winapi_Window.hpp"

int WINAPI wWinMain(HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPWSTR lpCmdLine,
                    int nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);
  UNREFERENCED_PARAMETER(nCmdShow);

  shi62::winapi::Window window(hInstance, L"win1", L"D3D11 自転立方体", 400, 300);
  shi62::d3d11::Core d3dCore(window.GetWindowHandle());

  while (!window.TerminationRequested() && !d3dCore.TerminationRequested()) {
    window.Update();
    d3dCore.Update();
    d3dCore.Draw();
  }

  return 0;
}