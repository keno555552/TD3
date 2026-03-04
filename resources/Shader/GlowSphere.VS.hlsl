
struct GlowVSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct GlowVSOutput
{
    float4 position : SV_Position;
    float3 worldPosition : POSITION0;
    float3 normal : NORMAL0;
};

GlowVSOutput main(GlowVSInput input, uint instanceId : SV_InstanceID)
{
    uint actualIndex = instanceId + instanceOffset;
    TransformationMatrix transform = gTransformationMatrices[actualIndex];

    GlowVSOutput output;
    output.position = mul(float4(input.position, 1.0f), transform.WVP);
    output.worldPosition = mul(float4(input.position, 1.0f), transform.world).xyz;
    output.normal = normalize(mul(input.normal, (float3x3) transform.worldInversTranspose));

    return output;
}