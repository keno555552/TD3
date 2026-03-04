
// ==============================
// Constant Buffers
// ==============================

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

struct Camera
{
    float3 position;
};
ConstantBuffer<Camera> gCamera : register(b1);

// VS側の出力と一致させる
struct FlameVertexShaderOutput
{
    float4 position : SV_Position;
    float3 normal : NORMAL0;
    float3 worldPosition : POSITION0;
    float localY : TEXCOORD0;
};

// PS出力
struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(FlameVertexShaderOutput input)
{
    PixelShaderOutput output;

    // Unity:
    // float gradient = saturate(i.worldPos.y / 2.0);
    // ※Unity側の i.worldPos はローカル座標として使っていたので localY を使う
    float denom = max(gMaterial.heightScale, 0.0001f);
    float gradient = saturate((input.localY + 1.0f) / denom);

    //gradient = saturate((input.localY + offset) / gMaterial.heightScale);

    // Unity:
    // float3 flameColor = lerp(_BaseColor.rgb, _TipColor.rgb, gradient);
    float3 flameColor = lerp(gMaterial.color.rgb, gMaterial.color2.rgb, gradient);

    float3 viewDir = normalize(gCamera.position - input.worldPosition);
    
    float rim = pow(1.0 - saturate(dot(normalize(input.normal), viewDir)), denom * 1.0f);
    flameColor += rim * gMaterial.color3.rgb;

    // Unity:
    // return fixed4(flameColor * _GlowIntensity, 1.0);
    output.color = float4(flameColor * gMaterial.intensity, 1.0f);

    return output;
}
