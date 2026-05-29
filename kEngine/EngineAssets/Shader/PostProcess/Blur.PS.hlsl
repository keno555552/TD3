#include "FullScreenQuadVSO.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

static const float2 kIndex3x3[3][3] =
{
    { float2(-1.0f, -1.0f), float2(0.0f, -1.0f), float2(1.0f, -1.0f) },
    { float2(-1.0f, 0.0f), float2(0.0f, 0.0f), float2(1.0f, 0.0f) },
    { float2(-1.0f, 1.0f), float2(0.0f, 1.0f), float2(1.0f, 1.0f) }
};

struct KernelCB
{
    int kernelSize;
    float kKernel[49];
};

KernelCB MakeTestKernel5x5()
{
    KernelCB r;
    r.kernelSize = 5;

    // 5x5 = 25 elements
    r.kKernel[0] = 1.0f / 25.0f;
    r.kKernel[1] = 1.0f / 25.0f;
    r.kKernel[2] = 1.0f / 25.0f;
    r.kKernel[3] = 1.0f / 25.0f;
    r.kKernel[4] = 1.0f / 25.0f;

    r.kKernel[5] = 1.0f / 25.0f;
    r.kKernel[6] = 1.0f / 25.0f;
    r.kKernel[7] = 1.0f / 25.0f;
    r.kKernel[8] = 1.0f / 25.0f;
    r.kKernel[9] = 1.0f / 25.0f;

    r.kKernel[10] = 1.0f / 25.0f;
    r.kKernel[11] = 1.0f / 25.0f;
    r.kKernel[12] = 1.0f / 25.0f;
    r.kKernel[13] = 1.0f / 25.0f;
    r.kKernel[14] = 1.0f / 25.0f;

    r.kKernel[15] = 1.0f / 25.0f;
    r.kKernel[16] = 1.0f / 25.0f;
    r.kKernel[17] = 1.0f / 25.0f;
    r.kKernel[18] = 1.0f / 25.0f;
    r.kKernel[19] = 1.0f / 25.0f;

    r.kKernel[20] = 1.0f / 25.0f;
    r.kKernel[21] = 1.0f / 25.0f;
    r.kKernel[22] = 1.0f / 25.0f;
    r.kKernel[23] = 1.0f / 25.0f;
    r.kKernel[24] = 1.0f / 25.0f;

    return r;
}


//static const float kKernel3x3[3][3] =
//{
//    { 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f },
//    { 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f },
//    { 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f }
//};


float3 Convolve(float2 vertexcoord, KernelCB cb)
{
    
    int2 size;
    gTexture.GetDimensions(size.x, size.y);
    float2 texelSize = float2(rcp((float) size.x), rcp((float) size.y));

    int k = cb.kernelSize / 2;
    float3 sum = float3(0.0f, 0.0f, 0.0f);
    
    for (int j = -k; j <= k; ++j)
    {
        for (int i = -k; i <= k; ++i)
        {
            float2 texcoord = vertexcoord + float2(j, i) * texelSize;
            float3 fetchColor = gTexture.Sample(gSampler, texcoord).rgb;
            sum += fetchColor * cb.kKernel[(j + k) * cb.kernelSize + (i + k)];
        }
    }
    return sum;
    
}

PixelShaderOutput main(VertexShaderOutput input)
{
    KernelCB cb = MakeTestKernel5x5();
    
    PixelShaderOutput output;
    output.color.rgb = float3(0.0f, 0.0f, 0.0f);
    output.color.a = 1.0f;

    output.color.rgb += Convolve(input.texcoord, cb);
    
    return output;
}
