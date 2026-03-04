#ifndef _LM_LAMBERT_LIGHTING_
#define _LM_LAMBERT_LIGHTING_

float3 ApplyLighting_Lambert(float3 N, float3 L, float3 lightColor)
{
    return saturate(dot(N, L)) * lightColor;
}

#endif