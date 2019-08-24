//ヘッダー読み込み
#include "d3d12_Core.hpp"
#include "winapi_Window.hpp"
#include "Constants.hpp"

INT WINAPI WinMain(_In_ HINSTANCE hInst,
                   _In_opt_ HINSTANCE hPrevInst,
                   _In_ LPSTR szStr,
                   _In_ INT iCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInst);
  UNREFERENCED_PARAMETER(szStr);
  UNREFERENCED_PARAMETER(iCmdShow);

  auto windowInstace = shi62::winapi::Window{ hInst, L"d3d12 init", L"DirectX12 Test", WINDOW_WIDTH, WINDOW_HEIGHT };
  auto d3d12Core = shi62::d3d12::Core(windowInstace.GetWindowHandle());

  while (!(windowInstace.TerminationRequested() || d3d12Core.TerminationRequested())) {
    windowInstace.Update();
    d3d12Core.UpdateCamera({}, {}, {});
    d3d12Core.UpdateCommands();
    d3d12Core.Render();
    d3d12Core.StallForGPU();
  }
  return 0;
}