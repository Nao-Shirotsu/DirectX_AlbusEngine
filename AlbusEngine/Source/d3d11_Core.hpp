#pragma once
#pragma once
#include <d3d11.h>

namespace shi62::d3d11 {

// D3D全般の処理をやっつけるクラス
// 流石に神クラスなので分割すべきではある...
class Core {
public:
    // 色々初期化
    Core(const HWND windowHandle);

    // リソース解放
    ~Core();

    [[nodiscard]]
    auto TerminationRequested() -> bool;

    auto Update() -> void;

    auto Draw() -> void const;

private:
    auto CleanupDevice() -> void;

    bool terminationRequested;
    HWND mWindowHandle;
    D3D_DRIVER_TYPE mDriverType;
    D3D_FEATURE_LEVEL mFeatureLevel;
    ID3D11Device* mD3dDevice;
    ID3D11DeviceContext* mImmediateContext;
    IDXGISwapChain* mSwapChain;
    ID3D11RenderTargetView* mRenderTargetView;
    ID3D11VertexShader* mVertexShader;
    ID3D11PixelShader* mPixelShader;
    ID3D11InputLayout* mVertexLayout;
    ID3D11Buffer* mVertexBuffer;
};

} // namespace shi62::d3d11