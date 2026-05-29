#include "object3d.hlsli"

//float4 main( float4 pos : POSITION ) : SV_POSITION
//{
//	return pos;
//}

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

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    //output.position = mul(input.position, gTransformationMatrices.WVP);
    output.texcoord = input.texcoord;
    //output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrices.world));
    return output;
}


