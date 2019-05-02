// 定数バッファのデータ定義
cbuffer cbCBuffer : register(b0) { // 常にスロット「0」を使う
    matrix World;      // ワールド変換行列
    matrix View;       // ビュー変換行列
    matrix Projection; // 透視変換行列
    float3 Light;      // 光源座標(ビュー座標系)
};

Texture2D myTex2D;   // テクスチャ

// サンプラ
SamplerState smpWrap : register(s0);

// **************************************************
// 立方体の描画
// **************************************************

// ピクセル シェーダの入力データ定義
struct PS_INPUT {
    float4 Pos  : SV_POSITION; // 頂点座標(透視座標系)
    float3 PosView  : POSVIEW; // 頂点座標(ビュー座標系)
    float3 Norm : NORMAL;      // 法線ベクトル(ワールド座標系)
    float2 Tex  : TEXTURE;     // テクスチャ座標
};

// 頂点シェーダの関数
float4 VS(uint vID : SV_VertexID) : SV_POSITION {
    float4 pos;
    // 頂点座標(モデル座標系)の生成
    static const uint pID[36] = { 0,1,3, 1,2,3, 1,5,2, 5,6,2, 5,4,6, 4,7,6,
                            4,5,0, 5,1,0, 4,0,7, 0,3,7, 3,2,7, 2,6,7 };
    switch (pID[vID]) {
    case 0: pos = float4(-1.0,  1.0, -1.0, 1.0); break;
    case 1: pos = float4( 1.0,  1.0, -1.0, 1.0); break;
    case 2: pos = float4( 1.0, -1.0, -1.0, 1.0); break;
    case 3: pos = float4(-1.0, -1.0, -1.0, 1.0); break;
    case 4: pos = float4(-1.0,  1.0,  1.0, 1.0); break;
    case 5: pos = float4( 1.0,  1.0,  1.0, 1.0); break;
    case 6: pos = float4( 1.0, -1.0,  1.0, 1.0); break;
    case 7: pos = float4(-1.0, -1.0,  1.0, 1.0); break;
    }

    // 頂点座標　モデル座標系→ビュー座標系
    pos = mul(pos, World);
    pos = mul(pos, View);

    // 出力
    return pos;
}

// ジオメトリ シェーダの関数
[maxvertexcount(3)]
void GS(triangle float4 input[3] : SV_POSITION, uint pID : SV_PrimitiveID,
        inout TriangleStream<PS_INPUT> TriStream) {
    PS_INPUT output;

    // テクスチャ座標の計算
    static const float2 tID[6] = {{ 0.0,  0.0}, {1.0,  0.0}, {0.0, 1.0},
                                  { 1.0,  0.0}, {1.0,  1.0}, {0.0, 1.0}};
    uint tIdx = (pID & 0x01) ? 3 : 0;

    // 法線ベクトルの計算
    float3 faceEdge  = input[0].xyz / input[0].w;
    float3 faceEdgeA = (input[1].xyz / input[1].w) - faceEdge;
    float3 faceEdgeB = (input[2].xyz / input[2].w) - faceEdge;
    output.Norm = normalize(cross(faceEdgeA, faceEdgeB));

    // 各頂点の計算
    for (int i=0; i<3; ++i) {
        // 頂点座標　ビュー座標系
        output.PosView = input[i].xyz / input[i].w;
        // 頂点座標　ビュー座標系→射影座標系
        output.Pos = mul(input[i], Projection);
        // テクスチャ座標
        output.Tex = tID[tIdx + i];
        // 出力
        TriStream.Append(output);
    }
    TriStream.RestartStrip();
}

// ライティング計算
float lighting(PS_INPUT input)
{
    // 光源ベクトル
    float3 light = Light - input.PosView;
    // 距離
    float  leng = length(light);
    // 明るさ
    return 30 * dot(normalize(light), normalize(input.Norm)) / pow(leng, 2);
}

// ピクセル・シェーダの関数
float4 PS(PS_INPUT input) : SV_TARGET
{
    // ライティング計算
    float bright = lighting(input);

    // テクスチャ取得
    float4 texCol = myTex2D.Sample(smpWrap, input.Tex);         // テクセル読み込み

    // 色
    return saturate(bright * texCol);
}
