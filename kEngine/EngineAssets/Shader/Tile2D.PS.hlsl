#include "Tile2D.hlsli"

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
};

ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    
    if (textureColor.a <= 0.5 || textureColor.a == 0.0)
        discard;
    
    output.color.rgb = gMaterial.color.rgb * textureColor.rgb;
    output.color.a = gMaterial.color.a * textureColor.a;

    if (output.color.a == 0.0)
        discard;
    
    return output;
}

