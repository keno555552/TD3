
// ==============================
// Constant Buffers
// ==============================
struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 world;
    float4x4 worldInversTranspose;
};
StructuredBuffer<TransformationMatrix> gTransformationMatrices : register(t0);

cbuffer InstanceOffset : register(b1)
{
    uint instanceOffset;
}

// ==============================
// Vertex Input / Output
// ==============================
struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal   : NORMAL0;
};

// ネオン用：PSに「ローカルY」を渡す
struct FlameVertexShaderOutput
{
    float4 position : SV_Position;
    float3 normal : NORMAL0;
    float3 worldPosition : POSITION0;
    float  localY   : TEXCOORD0;
};

FlameVertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    uint actualIndex = instanceId + instanceOffset;
    TransformationMatrix transform = gTransformationMatrices[actualIndex];
    
    // Unity版は「ローカル座標のY」を使ってグラデーションしているので同じにする
    FlameVertexShaderOutput output;
    output.position = mul(input.position, transform.WVP);
    //output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) transform.worldInversTranspose));
    output.worldPosition = mul(input.position, transform.world).xyz;
    output.localY = input.position.y;


    return output;
}
