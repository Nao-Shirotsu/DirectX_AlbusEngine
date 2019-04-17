float4 VertexShade( float4 pos : POSITION ) : SV_Position
{
    return pos;
}

float4 PixelShade() : SV_Target
{
    return float4( 0.75f, 0.0f, 0.75f, 1.0f );
}
