#include "Structures.hlsli"

float4 main(PUN_SV input) : SV_TARGET
{
    float3 lightDir = { 0.0f, -1.0f, 1.0f };
    float3 lightColor = { 0.5f, 0.5f, 0.0f };
    float3 normal = normalize(input.normal);

    float3 L = normalize(-lightDir);
    float NdotL = saturate(dot(normal, L));
    float3 diffuse = lightColor * NdotL;

    float3 ambient = { 0.1f, 0.1f, 0.1f };
    return float4(saturate(ambient + diffuse), 1.0f);

}