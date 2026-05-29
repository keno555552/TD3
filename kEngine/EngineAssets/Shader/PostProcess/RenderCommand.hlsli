struct RenderCommand
{
    // guardColor
    float3 guardColor;
    float guardAmount;
    
    // Vignette
    float2 vignetteCenter;
    float vignetteRadius;
    float vignetteSoftness;
    float vignetteIntensity;
    float4 vignetteColor;
};