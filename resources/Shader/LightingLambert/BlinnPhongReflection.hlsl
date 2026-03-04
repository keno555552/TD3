#ifndef _LM_BlinnPHONG_LIGHTING_
#define _LM_BlinnPHONG_LIGHTING_

float3 ApplyLighting_BlinnPhongReflection(float3 Normal, float3 LightDirection, float3 lightColor, float3 worldPos, float3 cameraPos, float shininess)
{
    Normal = normalize(Normal);
    LightDirection = normalize(LightDirection);
    
    // View direction
    float3 toEye = normalize(cameraPos - worldPos);
    
    // Diffuse (Half-Lambert)
    float NdotL = dot(Normal, LightDirection);
    float diffuse = pow(NdotL * 0.5f + 0.5f, 2.0f);
    
    // Specular (Phong)
    float3 halfVector = normalize(LightDirection + toEye);
    float NdotH = max(dot(Normal, halfVector), 0.0f);
    float specularPow = pow(saturate(NdotH), shininess);
    
    // Combine
    return (diffuse + specularPow) * lightColor;
}

#endif
