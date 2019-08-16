// AlbusEngine12.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include <windows.h>

#define WIN32_LEAN_AND_MEAN

// グローバル変数:
HINSTANCE hInst; // 現在のインターフェイス

// このコード モジュールに含まれる関数の宣言を転送します:
ATOM MyRegisterClass(HINSTANCE hInstance, LPCWSTR windowClassName);
BOOL InitInstance(HINSTANCE, int, LPCWSTR, LPCWSTR);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
void ProcessKeydownMessage(HWND windowHandle, WPARAM wParam);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  LPCWSTR winClass = L"dx12Handler";
  LPCWSTR winTitle = L"DirectX12 Test";

  MyRegisterClass(hInstance, winClass);

  // アプリケーション初期化の実行:
  if (!InitInstance(hInstance, nCmdShow, winClass, winTitle)) {
    return FALSE;
  }

  MSG msg;

  // メイン メッセージ ループ:
  while (GetMessage(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return ( int )msg.wParam;
}


//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
ATOM MyRegisterClass(HINSTANCE hInstance, LPCWSTR windowClassName) {
  WNDCLASSEXW wcex;
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = WndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = hInstance;
  wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wcex.lpszMenuName = nullptr;
  wcex.lpszClassName = windowClassName;
  wcex.hIconSm = nullptr;

  return RegisterClassExW(&wcex);
}

//
//   関数: InitInstance(HINSTANCE, int)
//
//   目的: インスタンス ハンドルを保存して、メイン ウィンドウを作成します
//
//   コメント:
//
//        この関数で、グローバル変数でインスタンス ハンドルを保存し、
//        メイン プログラム ウィンドウを作成および表示します。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, LPCWSTR windowClassName, LPCWSTR windowTitle) {
  hInst = hInstance; // グローバル変数にインスタンス ハンドルを格納する

  HWND hWnd = CreateWindowW(windowClassName, windowTitle, WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

  if (!hWnd) {
    return FALSE;
  }

  ShowWindow(hWnd, nCmdShow);
  UpdateWindow(hWnd);

  return TRUE;
}

//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的: メイン ウィンドウのメッセージを処理します。
//
//  WM_COMMAND  - アプリケーション メニューの処理
//  WM_PAINT    - メイン ウィンドウを描画する
//  WM_DESTROY  - 中止メッセージを表示して戻る
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hWnd, &ps);
      // TODO: HDC を使用する描画コードをここに追加してください...
      EndPaint(hWnd, &ps);
      break;
    }

    case WM_DESTROY: {
      PostQuitMessage(0);
      break;
    }

    case WM_KEYDOWN: {
      ProcessKeydownMessage(hWnd, wParam);
      break;
    }

    default: {
      break;
    }
  }
  return DefWindowProc(hWnd, message, wParam, lParam);
}

// バージョン情報ボックスのメッセージ ハンドラーです。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
  UNREFERENCED_PARAMETER(lParam);
  switch (message) {
    case WM_INITDIALOG: {
      return static_cast<INT_PTR>(TRUE);
    }

    case WM_COMMAND: {
      if (LOWORD(wParam) != IDOK && LOWORD(wParam) != IDCANCEL) {
        break;
      }
      EndDialog(hDlg, LOWORD(wParam));
      return static_cast<INT_PTR>(TRUE);
    }
  }
  return static_cast<INT_PTR>(FALSE);
}

void ProcessKeydownMessage(HWND windowHandle, WPARAM wParam) {
  switch (wParam) {
    case VK_ESCAPE: {
      PostMessage(windowHandle, WM_CLOSE, 0, 0);
      break;
    }
  }
}