#include "winapi_Window.hpp"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);
  UNREFERENCED_PARAMETER(nCmdShow);

  shi62::winapi::Window windowInstance{ hInstance, L"dx12 window handler", L"DirectX12 Test", 800, 600 };
  while (!windowInstance.TerminationRequested()) {
    windowInstance.Update();
  }

  return 0;
}