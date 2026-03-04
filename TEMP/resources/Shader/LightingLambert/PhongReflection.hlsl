#ifndef _LM_PHONG_LIGHTING_
#define _LM_PHONG_LIGHTING_

float3 ApplyLighting_PhongReflection(float3 Normal, float3 LightDirection, float3 lightColor, float3 worldPos, float3 cameraPos, float shininess)
{
    Normal = normalize(Normal);
    LightDirection = normalize(LightDirection);
    
    // View direction
    float3 toEye = normalize(cameraPos - worldPos);
    
    // Diffuse (Half-Lambert)
    float NdotL = dot(Normal, LightDirection);
    float diffuse = pow(NdotL * 0.5f + 0.5f, 2.0f);
    
    // Specular (Phong)
    float3 reflectedLight = reflect(-LightDirection, Normal);
    float RdotE = max(dot(reflectedLight, toEye), 0.0f);
    float specularPow = pow(saturate(RdotE), shininess);
    
    // Combine
    return (diffuse + specularPow) * lightColor;
}

#endif
