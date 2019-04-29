cbuffer cbNeverChanges : register(b0) {
    matrix Projection;
};

cbuffer cbChangesEveryFrame : register(b1) {
    matrix View;
    float3 Light;
};

cbuffer cbChangesEveryObject : register(b2) {
    matrix World;
};

struct VS_INPUT {
    float3 Pos : POSITION;
    float3 Col : COLOR;
};

struct GS_INPUT {
    float4 Pos : SV_POSITION;
    float4 Col : COLOR;
};

struct PS_INPUT {
    float4 Pos : SV_POSITION;
    float3 PosView : POSVIEW;
    float3 Norm : NORMAL;
    float4 Col : COLOR;
};

GS_INPUT VS(VS_INPUT input) {
    GS_INPUT output;
    float4 pos4 = float4(input.Pos, 1.0);
    output.Pos = mul(pos4, World);
    output.Pos = mul(output.Pos, View);
    output.Col = float4(input.Col, 1.0);
    return output;
}

[maxvertexcount(3)] void GS(triangle GS_INPUT input[3],
                            inout TriangleStream<PS_INPUT> TriStream) {
    PS_INPUT output;
    float3 faceEdge = input[0].Pos.xyz / input[0].Pos.w;
    float3 faceEdgeA = (input[1].Pos.xyz / input[1].Pos.w) - faceEdge;
    float3 faceEdgeB = (input[2].Pos.xyz / input[2].Pos.w) - faceEdge;
    output.Norm = normalize(cross(faceEdgeA, faceEdgeB));
    for (int i = 0; i < 3; ++i) {
        output.PosView = input[i].Pos.xyz / input[i].Pos.w;
        output.Pos = mul(input[i].Pos, Projection);
        output.Col = input[i].Col;
        TriStream.Append(output);
    }
    TriStream.RestartStrip();
}

float4 PS(PS_INPUT input)
    : SV_TARGET {
    float3 light = Light - input.PosView;
    float leng = length(light);
    float bright = 30 * dot(normalize(light), normalize(input.Norm)) / pow(leng, 2);
    return saturate(bright * input.Col);
}