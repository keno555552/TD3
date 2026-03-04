#include "Tile2D.hlsli"

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 world;
    float4x4 worldInversTranspose;
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

VertexShaderOutput main(VertexShaderInput input, uint instanceId:SV_InstanceID)
{
    uint actualIndex = instanceId + instanceOffset;
    TransformationMatrix transform = gTransformationMatrices[actualIndex];
    
    VertexShaderOutput output;
    output.position = mul(input.position, transform.WVP);
    output.texcoord = input.texcoord;
    return output;
}


//VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
//{
//    VertexShaderOutput output;
//    output.position = float4(0.0f, 0.0f, -0.5f, 1.0f); // 螢幕正中央
//    output.texcoord = float2(0.0f, 0.0f);
//    return output;
//}

