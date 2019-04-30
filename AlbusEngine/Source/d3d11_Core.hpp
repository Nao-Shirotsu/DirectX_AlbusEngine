#pragma once
#include <d3d11.h>
#include <xnamath.h>

namespace shi62::d3d11 {

struct CBChangesEveryFrame {
    XMFLOAT4X4 View; // ビュー変換行列
    XMFLOAT3 Light;  // 光源座標(ワールド座標系)
    FLOAT dummy;     // ダミー
};

// D3D全般の処理をやっつけるクラス
// 流石に神クラスなので分割すべきではある...
class Core {
public:
    // 色々初期化
    Core(const HWND windowHandle);

    // リソース解放
    ~Core();

    [[nodiscard]] auto TerminationRequested() -> bool;

    auto Update() -> void;

    auto Draw() -> void const;

private:
    auto CleanupDevice() -> void;
    auto CreateVertexPosBuffer() -> void;
    auto CreateVertexColorBuffer() -> void;
    auto CreateVertexIndexBuffer() -> void;
    auto InitBackBuffer() -> HRESULT;

    // エラー処理関数 (命名が難しいので役割過多だよ、あとで色々変えとけ)
    auto HandleError(LPCWSTR errMsg) -> void;

    bool terminationRequested;
    HWND mWindowHandle;
    D3D_FEATURE_LEVEL mFeatureLevel;
    ID3D11Device* mD3dDevice;
    ID3D11DeviceContext* mImmediateContext;
    IDXGISwapChain* mSwapChain;

    ID3D11RenderTargetView* mRenderTargetView;
    D3D11_VIEWPORT mViewport;

    ID3D11Texture2D* mDepthStencilBuffer;
    ID3D11DepthStencilView* mDepthStencilView;

    ID3D11InputLayout* mVertexLayout;
    ID3D11Buffer* mVertexPos;
    ID3D11Buffer* mVertexColor;
    ID3D11Buffer* mVertexIndex;

    ID3D11VertexShader* mVertexShader;
    ID3D11GeometryShader* 
        mGeometryShader;
    ID3D11PixelShader* mPixelShader;

    ID3D11Buffer* mCBuffer[3]; // 定数バッファ
    XMFLOAT4X4 mCBNeverChanges;
    XMFLOAT4X4 mCBChangesEveryObject;
    CBChangesEveryFrame mCBChangesEveryFrame;
    XMFLOAT3 mLightPos;

    ID3D11BlendState* mBlendState;
    ID3D11RasterizerState* mRasterizerState;
    ID3D11DepthStencilState* mDepthStencilState;
};

} // namespace shi62::d3d11