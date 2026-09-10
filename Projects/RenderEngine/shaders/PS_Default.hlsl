#include "Registers.hlsli"

cbuffer Material : register(MATERIAL)
{
    float3 ambient;
    float shininess;
    float3 diffuse;
    float alpha;
    float3 specular;
    float reflect;
};

cbuffer LightCollection : register(LIGHT_COUNT)
{
    uint pointLightCount;
    uint directionalLightCount;
    uint spotLightCount;
    float _pad0;
    float3 ambientLight;
    float _pad1;
};

struct ShadowSettings
{
    row_major float4x4 vp;
    float bias;
    float strength;
    bool enabled;
    float _pad0;
    float2 texelSize;
    float2 _pad1;
};

// TODO: maybe just pass general settings, every light that casts a shadow should pass its own vp matrix instead of just the one
cbuffer ShadowData : register(b4)
{
    ShadowSettings shadow;
};

struct PointLightData
{
    float3 position;
    float range;
    float3 color;
    float intensity;
    float3 attenuation;
    float _pad0;
};

struct DirectionalLightData
{
    float3 direction;
    float intensity;
    float3 color;
    float _pad0;
};


struct SpotLightData
{
    float3 position;
    float range;
    float3 direction;
    float intensity;
    float3 color;
    float innerCos;
    float3 attenuation;
    float outerCos;
};

StructuredBuffer<PointLightData> pointLights : register(t4);
StructuredBuffer<DirectionalLightData> dirLights : register(t5);
StructuredBuffer<SpotLightData> spotLights : register(t6);

// textures
Texture2D diffuseMap : register(t1);    // just diffuse for now
Texture2DArray<float> shadowMaps : register(t3); // one shadow per light up to the max TODO: include with light collection

// samplers
SamplerState defaultSampler : register(s0);
SamplerComparisonState shadowSampler : register(s1);

struct VSOut
{
    float4 sv_pos   : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float3 normal : NORMAL;
};

float4 main(VSOut input) : SV_TARGET
{
    float3 normal = normalize(input.normal);

    // sum of diffuse for each light type
    float3 pointDiffuse = 0.0f;
    float3 directionalDiffuse = 0.0f;
    float3 spotDiffuse = 0.0f;

    // used for shadow biasing
    float directionalNdotL = 0.0f;

    // point lights
    [loop]
    for (uint i = 0; i < pointLightCount; ++i)
    {
        const PointLightData l = pointLights[i];
        const float3 toLight = l.position - input.worldPos;
        const float dist = length(toLight);
        const float3 L = dist > 1e-4f ? (toLight / dist) : float3(0.0f, 1.0f, 0.0f);
        const float NdotL = saturate(dot(normal, L));
        const float attenuation = 1.0f / (l.attenuation.x + l.attenuation.y * dist + l.attenuation.z * dist * dist);
        const float distNorm = saturate(dist / max(l.range, 1e-3f));

        float rangeMask = 1.0f - distNorm;
        rangeMask = rangeMask * rangeMask * (3.0f - 2.0f * rangeMask); // smoothstep-like falloff
        pointDiffuse += (l.color * l.intensity) * NdotL * attenuation * rangeMask;
    }

    // dir lights
    [loop]
    for (uint i = 0; i < directionalLightCount; ++i)
    {
        const DirectionalLightData l = dirLights[i];
        const float3 L = normalize(-l.direction);
        const float NdotL = saturate(dot(normal, L));
        if (i == 0)
            directionalNdotL = NdotL;

        directionalDiffuse += (l.color * l.intensity) * NdotL;
    }

    // spot lights
    [loop]
    for (uint i = 0; i < spotLightCount; ++i)
    {
        const SpotLightData l = spotLights[i];
        const float3 toLight = l.position - input.worldPos;
        const float dist = length(toLight);
        const float3 L = dist > 1e-4f ? (toLight / dist) : float3(0.0f, 1.0f, 0.0f);
        const float NdotL = saturate(dot(normal, L));

        const float3 spotDir = normalize(-l.direction);
        const float cosTheta = dot(L, spotDir);
        const float cone = saturate((cosTheta - l.outerCos) / max(l.innerCos - l.outerCos, 1e-4f));

        const float attenuation = 1.0f / (l.attenuation.x + l.attenuation.y * dist + l.attenuation.z * dist * dist);
        const float distNorm = saturate(dist / max(l.range, 1e-3f));
        float rangeMask = 1.0f - distNorm;
        rangeMask = rangeMask * rangeMask * (3.0f - 2.0f * rangeMask); // smoothstep-like falloff
        spotDiffuse += (l.color * l.intensity) * NdotL * attenuation * rangeMask * cone;
    }

    float shadowFactor = 1.0f;

    if (shadow.enabled && directionalLightCount > 0)
    {
        const float4 shadowClip = mul(float4(input.worldPos, 1.0f), shadow.vp);
        const float3 shadowNdc  = shadowClip.xyz / max(shadowClip.w, 1e-6f);
        const float2 shadowUv   = shadowNdc.xy * float2(0.5f, -0.5f) + 0.5f;

        const float normalBias = shadow.bias * (1.0f - directionalNdotL);
        const float  shadowDepth = shadowNdc.z - (shadow.bias + normalBias);

        const bool inBounds = shadowUv.x >= 0.0f && shadowUv.x <= 1.0f &&
                              shadowUv.y >= 0.0f && shadowUv.y <= 1.0f &&
                              shadowDepth >= 0.0f && shadowDepth <= 1.0f;

        if (inBounds)
        {
            float visibility = 0.0f;
            [unroll]
            for (int oy = -2; oy <= 2; ++oy)
            {
                [unroll]
                for (int ox = -2; ox <= 2; ++ox)
                {
                    const float2 uv = shadowUv + float2(ox, oy) * shadow.texelSize;
                    visibility += shadowMaps.SampleCmpLevelZero(shadowSampler, float3(uv, 0), shadowDepth);
                }
            }
            visibility /= 25.0f;
            shadowFactor = lerp(1.0f - shadow.strength, 1.0f, visibility);
        }
    }

    /**
     * Combine lighting contributions
     */
    directionalDiffuse *= shadowFactor;
    float3 lighting = saturate(ambientLight + pointDiffuse + directionalDiffuse + spotDiffuse);

    const float3 texColor = diffuseMap.Sample(defaultSampler, input.uv).rgb;
    const float3 materialColor = diffuse * texColor;

    // Final colour, no specular or reflection yet
    const float3 litColor = materialColor * lighting;
    return float4(litColor, alpha);
}
