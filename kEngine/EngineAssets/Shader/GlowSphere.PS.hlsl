
cbuffer GlowParams : register(b1)
{
    float3 glowColor; // 光圈顏色
    float glowIntensity; // 強度
    float glowPower; // 邊緣銳度（2~5）
};

cbuffer CameraData : register(b2)
{
    float3 cameraPosition;
};

struct GlowVSOutput
{
    float4 position : SV_Position;
    float3 worldPosition : POSITION0;
    float3 normal : NORMAL0;
};

float4 main(GlowVSOutput input) : SV_Target
{
    float3 N = normalize(input.normal);
    float3 V = normalize(cameraPosition - input.worldPosition);

    // Fresnel / Rim Light 核心
    float rim = pow(1.0 - saturate(dot(N, V)), glowPower);

    float3 color = glowColor * rim * glowIntensity;

    return float4(color, rim); // alpha 用 rim 也很自然
}