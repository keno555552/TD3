#include "FullScreenQuadVSO.hlsli"
#include "RenderCommand.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

RenderCommand gRenderCommand : register(b0);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    output.color = gTexture.Sample(gSampler, input.texcoord);
    
    /// まずはグレースケール化
    float value = dot(output.color.rgb, float3(0.2125f, 0.7154f, 0.0721f));
    
    float3 tintedGray = value * gRenderCommand.guardColor;
    
    output.color.rgb = lerp(output.color.rgb, tintedGray, gRenderCommand.guardAmount);
    
    return output;
}
