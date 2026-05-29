#include "SkyCube.hlsli"

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

struct Material
{
    float4 color;
    float4 color2;
    float4 color3;
    
    int enableLighting;

    float4x4 uvTransform;
    float shininess;
    float intensity;
    float heightScale;
    
    float reflectiveStrength;
};

// Resources
SamplerState gSampler : register(s0);
TextureCube<float4> gTexture : register(t0);
ConstantBuffer<Material> gMaterial : register(b0);


PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    output.color = textureColor * gMaterial.color;
    return output;
}
