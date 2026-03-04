#ifndef _LM_HALFLAMBERT_LIGHTING_
#define _LM_HALFLAMBERT_LIGHTING_

float3 ApplyLighting_HalfLambert(float3 N, float3 L, float3 lightColor)
{
    float NdotL = dot(N, L);
    float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);

    return cos * lightColor ;
}

#endif