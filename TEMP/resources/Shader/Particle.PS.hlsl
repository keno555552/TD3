#include "Particle.hlsli"
#if defined(LIGHT_MODEL_LAMBERT)
#include "./LightingLambert/Lambert.hlsl"
#endif

#if defined(LIGHT_MODEL_HALF)
#include "./LightingLambert/HalfLambert.hlsl"
#endif

#if defined(LIGHT_MODEL_PHONG)
#include "./LightingLambert/PhongReflection.hlsl"
#endif

#if defined(LIGHT_MODEL_BLINN_PHONG)
#include "./LightingLambert/BlinnPhongReflection.hlsl"
#endif

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

struct LightGPU
{
    float3 direction;
    float angle;

    float3 position;
    float range;

    float3 color;
    float intensity;

    int type;
    int extra0;
    int extra1;
    int extra2;
};


struct Camera
{
    float3 position;
};


// Constant buffers
ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<Camera> gCamera : register(b1);
cbuffer LightCountCB : register(b2)
{
    uint gLightCount;
};

// Resources
Texture2D<float4> gTexture : register(t0);
StructuredBuffer<LightGPU> gLights : register(t1);

// Resources
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
    
    if (gMaterial.enableLighting)
    {
        float3 result = 0;
        float totalWeight = 0.0f;

        float3 N = normalize(input.normal);

        uint lightCount = gLightCount;
        
        // ⭐ 多光源迴圈
        [loop]
        for (uint i = 0; i < lightCount; i++)
        {
            LightGPU L = gLights[i];
            
            float3 lightDir = 0;
            float3 lightColor = 0;
            bool validLight = true;
            
            // DirectionalLight
            if (L.type == 0)
            {
                lightDir = normalize(-L.direction);
                lightColor = L.color * L.intensity;
            }
            
            // Point Light
            else if (L.type == 1)
            {
                float3 toLight = L.position - input.worldPosition;
                float dist = length(toLight);
                lightDir = normalize(toLight);

                // 距離衰減
                float atten = saturate(1.0f - dist / L.range);

                lightColor = L.color * L.intensity * atten;
            }
            
            // Spot Light
            else if (L.type == 2)
            {
                float3 toLight = L.position - input.worldPosition;
                float dist = length(toLight);
                lightDir = normalize(toLight);

                // 距離衰減
                float atten = saturate(1.0f - dist / L.range);

            // Spot cone（方向衰減）
                float cosTheta = dot(lightDir, normalize(-L.direction));
                float spotFactor = saturate(cosTheta / cos(L.angle));

                lightColor = L.color * L.intensity * atten * spotFactor;
            }
            else
            {
                validLight = false;
            }

            if (!validLight)
                continue;

            
            
#if defined(LIGHT_MODEL_LAMBERT)
                result += ApplyLighting_Lambert(N, lightDir, lightColor);
                totalWeight += 1.0f;
#endif
        
#if defined(LIGHT_MODEL_HALF)
                result += ApplyLighting_HalfLambert(N, lightDir, lightColor);
                totalWeight += 1.0f;
#endif
       
#if defined (LIGHT_MODEL_PHONG)

                result += ApplyLighting_PhongReflection(N,
                                                        lightDir,
                                                        lightColor,
                                                        input.worldPosition,
                                                        gCamera.position,
                                                        gMaterial.shininess);
                totalWeight += 1.0f;
#endif 
       
#if defined (LIGHT_MODEL_BLINN_PHONG)

                result += ApplyLighting_BlinnPhongReflection(N,
                                                            lightDir,
                                                            lightColor,
                                                            input.worldPosition,
                                                            gCamera.position,
                                                            gMaterial.shininess);
                totalWeight += 1.0f;
#endif 
       
            
        }
        //result += gMaterial.color.rgb * textureColor.rgb;
        //totalWeight += 1.0f;
        
        float3 lightFactor = result / max(totalWeight, 1.0f);
        
        //lightFactor = (gMaterial.color.rgb + lightFactor) * 0.5f;
        //float3 finalColor = textureColor.rgb * lightFactor * gDirectionalLight.intensity;
        
        float3 finalColor = gMaterial.color.rgb * textureColor.rgb * lightFactor;
       
        output.color.rgb = finalColor;
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    {
        output.color.rgb = gMaterial.color.rgb * textureColor.rgb;
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    //output.color.rgb = normalize(input.normal) * 0.5f + 0.5f;
    //output.color.a = 1.0f;
    return output;

}

