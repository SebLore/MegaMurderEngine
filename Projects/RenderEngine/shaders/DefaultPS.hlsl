#include "hlsli/BoundResources.hlsli"
#include "hlsli/LightCalculations.hlsli"

cbuffer PerFrameData : register(FRAME_DATA)
{
    float AmbientFactor;
    float3 Padding;
}

float3 ApplyFog(float3 rgb, float3 worldPos, float3 camPos, float3 fogColor, float density)
{
    float d = length(worldPos - camPos);
    float fog = 1.0f - exp(-d * density); // exponential fog
    return lerp(rgb, fogColor, saturate(fog));
}

float4 main(PUN_SV input) : SV_TARGET
{
    float3 ambiTX = ambientMap.Sample(defaultSampler, input.uv).rgb;
    float3 diffTX = diffuseMap.Sample(defaultSampler, input.uv).rgb;
    float3 specTX = specularMap.Sample(defaultSampler, input.uv).rgb;
    
 
    float ambiF = clamp(AmbientFactor, 0.0f, 1.0f);
    const float3 surface = input.position;
    // 1.2 apply material properties, colors
    float3 Kd = material.diffuse * diffTX;
    float3 Ka = material.ambient * ambiTX * ambiF * Kd;
    float3 Ks = material.specular * specTX;
    
    // vectors 
    float3 viewDir = normalize(camera.position - surface);
    float3 normal = normalize(input.normal);
    
 
    // 2. calculate lighting
    float3 lightsColor = GetLightsColor(
                                 lightCount,
                                 pointLights,
                                 dirLights,
                                 spotLights,
                                 shadowMaps,
                                 shadowSampler,
                                 normal,
                                 surface,
                                 viewDir,
                                 material.shininess
    );

    // 3. apply fog
    float3 fogColor = float3(0.2f, 0.2f, 0.2f); // gray fog


    float3 final = lightsColor * (Kd + Ks) + Ka;
    float3 fogged = ApplyFog(final, surface, camera.position, fogColor, 0.01f);

    return float4(fogged, material.alpha); // clamp final color to [0,1] range and use material alpha to enable blending
}
