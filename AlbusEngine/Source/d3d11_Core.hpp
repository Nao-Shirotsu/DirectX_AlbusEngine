#pragma once
#include <d3d11.h>

#include "winapi_HwndHolder.hpp"

namespace shi62::d3d11 {

// D3D全般の処理をやっつけるクラス
// 流石に神クラスなので分割すべきではある...
class Core : public winapi::HwndHolder {
public:
    // 何もしない
    Core();

    // リソース解放
    ~Core();

    // コンストラクタとは別に初期化する
    auto Init() -> void;
    auto Update() -> void;
    auto Draw() -> void const;

private:
    auto InitDevice() -> HRESULT;
    auto CleanupDevice() -> void;
    auto Render() -> void;

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