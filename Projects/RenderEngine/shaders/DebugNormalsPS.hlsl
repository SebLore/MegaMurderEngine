#include "Registers.hlsli"
#include "Structures.hlsli"

float4 main(PUN_SV input) : SV_TARGET
{
    float3 normal = normalize(input.normal);

    return float4(normal, 1.0f);
}