#include "FullScreenQuadVSO.hlsli"

static const int kNumVertex = 3;
static const float4 kPosition[kNumVertex] =
{
    float4(-1.0f,  1.0f, 0.0f, 1.0f),
    float4( 3.0f,  1.0f, 0.0f, 1.0f),
    float4(-1.0f, -3.0f, 0.0f, 1.0f)
};
static const float2 kTexcoord[kNumVertex] =
{
    { 0.0f, 0.0f },
    { 2.0f, 0.0f },
    { 0.0f, 2.0f }
};
VertexShaderOutput main(uint vertexId : SV_VertexID)
{
    VertexShaderOutput output;
    output.position = kPosition[vertexId];
    output.texcoord = kTexcoord[vertexId];
    return output;
}