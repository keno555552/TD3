#include "Particle.hlsli"

/// ################### Readme ################### ///
/// このシェーダーは、PSとhlsliが付いてなく、普段のシェーダーと一緒に使う
/// 現在主に使ってるのは:
/// 1.hlsli: Particle.hlsli
/// 2.PS: Particle.PS.hlsl

/// ==================== Transformation Instance Buffer ==================== ///
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

/// ============================ Vertex Input ============================== ///

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float4 weight : WEIGHT0;
    int4 index : INDEX0;
};

/// ============================= Skeleton関連 ============================== ///

struct Well
{
    float4x4 skeletonSpaceMatrix;
    float4x4 skeletonSpaceMatrixInverseTranspose;
};

StructuredBuffer<Well> gMatrixPalette : register(t1);

/// ============================= Skinning関連 ============================== ///

struct Skinned
{
    float4 position;
    float3 normal;
};

Skinned Skinning(VertexShaderInput input)
{
    Skinned skinned;
    
    skinned.position  = float4(0, 0, 0, 0);
    skinned.position  = mul(input.position, gMatrixPalette[input.index.x].skeletonSpaceMatrix) * input.weight.x; // ボーンのX影響
    skinned.position += mul(input.position, gMatrixPalette[input.index.y].skeletonSpaceMatrix) * input.weight.y; // ボーンのY影響
    skinned.position += mul(input.position, gMatrixPalette[input.index.z].skeletonSpaceMatrix) * input.weight.z; // ボーンのZ影響
    skinned.position += mul(input.position, gMatrixPalette[input.index.w].skeletonSpaceMatrix) * input.weight.w; // ボーンのW影響
    
    skinned.normal  = float3(0, 0, 0);
    skinned.normal  = mul(input.normal, (float3x3) gMatrixPalette[input.index.x].skeletonSpaceMatrixInverseTranspose) * input.weight.x; // ボーンのX影響
    skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[input.index.y].skeletonSpaceMatrixInverseTranspose) * input.weight.y; // ボーンのY影響
    skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[input.index.z].skeletonSpaceMatrixInverseTranspose) * input.weight.z; // ボーンのZ影響
    skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[input.index.w].skeletonSpaceMatrixInverseTranspose) * input.weight.w; // ボーンのW影響
    skinned.normal = normalize(skinned.normal);
    
    return skinned;
}


VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    uint actualIndex = instanceId + instanceOffset;
    Skinned skinned = Skinning(input); // スキニング処理を適用
    
    TransformationMatrix transform = gTransformationMatrices[actualIndex];
    
    VertexShaderOutput output;
    output.position = mul(skinned.position, transform.WVP);
    output.normal = normalize(mul(skinned.normal, (float3x3) transform.worldInverseTranspose));
    output.texcoord = input.texcoord;
    output.worldPosition = mul(skinned.position, transform.world).xyz;
    return output;
}