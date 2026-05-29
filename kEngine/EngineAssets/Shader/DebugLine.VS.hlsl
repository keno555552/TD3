#include "DebugLine.hlsli"

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 world;
    float4x4 worldInverseTranspose;
};
StructuredBuffer<TransformationMatrix> gTransformationMatrices : register(t0);

struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
};

VertexShaderOutput main(VSInput input, uint instanceID : SV_InstanceID)
{
    VertexShaderOutput o;
    
    TransformationMatrix m = gTransformationMatrices[instanceID];
    
    o.position = mul(float4(input.position, 1.0f), m.WVP);

    o.color = input.color;
    return o;
}