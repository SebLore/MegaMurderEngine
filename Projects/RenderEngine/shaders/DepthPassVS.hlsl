/*
* ShadowVS.hlsl
*
* Vertex shader for shadow mapping. Outputs depth data for shadow maps. Only needs
* position data from the vertex input to capture the depth.
*/
#include "hlsli/Registers.hlsli"
#include "hlsli/Structures.hlsli"

cbuffer Transform : register(TRANSFORM)
{
    TransformData transform;
}
cbuffer Camera : register(CAMERA)
{
    CameraProperties camera;
}

float4 main(PUN input) : SV_POSITION
{
    float4x4 wvp = mul(transform.world, camera.vp);

    return mul(float4(input.position, 1.0f), wvp);
}