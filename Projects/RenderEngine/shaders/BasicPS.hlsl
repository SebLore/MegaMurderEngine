/**
 * @brief Simple pixel-shader without complicated things like shadow mapping.
 */
#include "Registers.hlsli"
#include "Structures.hlsli"
#include "LightCalculations.hlsli"

cbuffer Material : register(MATERIAL)
{
    MaterialProperties material;
}

cbuffer Camera : register(CAMERA)
{
    CameraProperties camera;
}

cbuffer LightCount : register(LIGHT_COUNT)
{
    LightCount lightCount;
}

Texture2D diffuseMap : register(DIFFUSE_MAP);

SamplerState defaultSampler : register(DEFAULT_SAMPLER);
StructuredBuffer<DirLightData> dirLights : register(DIR_LIGHTS);


float4 main(PUN_SV input) : SV_TARGET
{
    float3 normal = normalize(input.normal);
    float3 viewDir = normalize(camera.position - input.position);
    
    // diffuse color only
    float3 lightColor = DirLightColor(dirLights[0], input.position, normal, viewDir, material.shininess);

    // texture color
    float3 diffuseTexture = diffuseMap.Sample(defaultSampler, input.uv).rgb;

    float4 final = float4(diffuseTexture * lightColor, 1.0f);
    return final;

}