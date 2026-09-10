/**
 * @file BasicVS.hlsl
 * @brief Vertex Shader without any bells and whistles, for debugging
 */
#include "Structures.hlsli"

cbuffer Transform : register(b0)
{
    TransformData transform;
}

cbuffer Camera : register(b1)
{
    CameraProperties camera;
}

PUN_SV main(PUN input )
{
    PUN_SV output;

    float4x4 wvp = mul(transform.world, camera.vp);

    output.sv_pos = mul(float4(input.position, 1.0f), wvp);
    output.position = mul(float4(input.position, 1.0f), transform.world).xyz;
    output.normal = mul(input.normal, (float3x3) transform.normal);
    output.uv = input.uv;

    return output;
}