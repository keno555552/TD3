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
    float2 uv = input.texcoord;
    float3 color = gTexture.Sample(gSampler, uv).rgb;

    // 0. 中心を (0.5, 0.5) に移動してから、vignetteCenterを引く
    float2 cuv = uv - gRenderCommand.vignetteCenter + float2(0.5, 0.5);
    cuv = saturate(cuv); // 避免出界
    
    // 1. 距離衰減：中心は 1.0、邊緣は 0.0
    float2 correct = cuv * (1.0 - cuv); // 0 at edges, max at center
    float base = correct.x * correct.y; // 中心最大，四角最小

    // 2. 強度調整：乘以 vignetteRadius（越大 → vignette の効果が強まる）
    float v = saturate(base * gRenderCommand.vignetteRadius * 16.0);

    // 3. 柔和度
    v = pow(v, gRenderCommand.vignetteSoftness);

    // 4. 反転：v 越大 → vignette の影響が弱い
    float t = (1.0 - v) * gRenderCommand.vignetteIntensity;

    // 5. 色の補正：vignetteColor を乗算して、vignette の色を加える
    float3 tinted = lerp(color, gRenderCommand.vignetteColor.rgb, t * gRenderCommand.vignetteColor.a);

    output.color = float4(tinted, 1.0);
    return output;
}


//PixelShaderOutput main(VertexShaderOutput input)
//{
//    PixelShaderOutput output;
//    output.color = gTexture.Sample(gSampler, input.texcoord);
//    
//    float2 correct = input.texcoord * float2(1.0f - input.texcoord.yx);
//    
//    float vignette = correct.x * correct.y * 16.0f;
//    
//    vignette = saturate(pow(vignette, 0.8f));
//    
//    output.color.rgb *= vignette;
//    
//    return output;
//}
