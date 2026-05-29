#include "SkyCube.hlsli"

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 world;
    float4x4 worldInverseTranspose;
};
StructuredBuffer<TransformationMatrix> gTransformationMatrices : register(t0);

cbuffer InstanceOffset : register(b1)
{
    uint instanceOffset;
}

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    
    uint actualIndex = instanceId + instanceOffset;
    TransformationMatrix transform = gTransformationMatrices[actualIndex];
    
    VertexShaderOutput o;
    
    o.position = mul(input.position, transform.WVP).xyww;
    o.texcoord = normalize(input.position.xyz);
    
    //float4 pos = mul(input.position, transform.WVP);
    //o.position = float4(pos.xy, pos.w, pos.w);
    //o.texcoord = normalize(input.position.xyz);
    
    return o;
}