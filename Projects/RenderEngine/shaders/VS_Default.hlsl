

cbuffer Transform : register(b0)
{
    row_major float4x4 world;
    column_major float4x4 normal;
};

cbuffer Camera : register(b1)
{
    row_major float4x4 view;
    row_major float4x4 projection;
    row_major float4x4 vp;
    float3 cameraPosition;
    float cameraDt;
};

struct VSIn
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

struct VSOut
{
    float4 sv_pos : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float3 normal : NORMAL;
};

VSOut main(VSIn input)
{
    VSOut output;

    const float4 worldPos = mul(float4(input.position, 1.0f), world);
    output.sv_pos = mul(worldPos, vp);
    output.worldPos = worldPos.xyz;
    output.uv = input.uv;
    output.normal = mul(float4(input.normal, 0.0f), world).xyz; // normal should not be affected by translation, so w=0

    return output;
}
