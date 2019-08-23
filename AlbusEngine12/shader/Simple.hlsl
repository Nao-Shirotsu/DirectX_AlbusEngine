cbuffer CBuffer : register(b0) {
  float4x4 g_mWVP;
};

float4 VS(float4 Pos : POSITION) : SV_POSITION {
  float4 pos = mul(Pos, g_mWVP);
  return pos;
}

float4 PS(float4 Pos : SV_POSITION) : SV_Target {
  return float4(0, 1, 0, 1);
}
