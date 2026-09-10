#ifndef LIGHT_CALCULATIONS_HLSLI
#define LIGHT_CALCULATIONS_HLSLI

#define EPS 0.0001f // small epsilon for float cmp

#include "Structures.hlsli"

float3 GetLightsColor(
    LightCount lightCount,
    StructuredBuffer<PointLightData> pointLights,
    StructuredBuffer<DirLightData> dirLights,
    StructuredBuffer<SpotLightData> spotLights,
    Texture2DArray<float> shadowMaps,
    SamplerState shadowSampler,
    float3 normal,
    float3 position,
    float3 viewDir,
    float shininess);

float3 DiffuseColor(float3 normal, float3 fragToLight, float3 color, float intensity)
{
    float NdotL = max(0.0f, dot(normal, fragToLight));
    float3 diffuse = color * NdotL * intensity;
    return diffuse;
}

// Blinn-Phong specular calculation
float3 SpecularColor(float3 normal, float3 fragToLight, float3 viewDir, float3 color, float intensity, float shininess)
{
    // get the halfway vector between the camera's view direction and the light's direction
    float3 H = normalize(fragToLight + viewDir);
    float NdotH = saturate(dot(normal, H));
    float spec = pow(NdotH, shininess);
    float3 specular = color * spec * intensity; // make specular not super bright
    return specular;
}

// calculate the light fall off over distance
// att = (constant, linear, quadratic), d = distance to light
float Attenuation(float3 att, float d)
{
    return 1.0f
           / (att.x 
              + (att.y * d)
              + (att.z * d * d));
}

float3 PointLightColor(
    PointLightData light,
    float3 worldPos,
    float3 normal, // normalized normal vector
    float3 viewDir, // direction from the camera
    float shininess // material shininess
)
{
    // calculate if it's within range
    float3 fragToLight = light.position - worldPos;
    float d = length(fragToLight); // distance to light
    float3 black = float3(0.0f, 0.0f, 0.0f);
    
    //return light.color;
    
    // if the distance is greater than the light's maximum range, return no contribution
    if ((light.range - d) < EPS)
    {
        return black;
    }
    else
    {
        float a = Attenuation(light.attenuation, d);
        if (a < EPS)
        {
            return black;
        }
        else
        {
            fragToLight = normalize(fragToLight);
            float3 diffuse = DiffuseColor(normal, fragToLight, light.color, light.intensity);
            float3 specular = SpecularColor(normal, fragToLight, viewDir, light.color, light.intensity, shininess);
            return a * float3(saturate(diffuse + specular));
        }
    }
}

float3 DirLightColor(
    DirLightData light,
    float3 worldPos,
    float3 normal, // normalized normal vector
    float3 viewDir, // direction from the camera
    float shininess // material shininess
)
{
    // get light direction to fragment, dot product with normal and calculate diffuse
    float3 L = normalize(-light.dir);
    float NdotL = max(0.0f, dot(normal, L));
    float3 diffuse = light.color * NdotL * light.intensity;
    
    // get half vector between light direction and view direction, dot product with normal and calculate specular
    float3 H = normalize(L + viewDir);
    float NdotH = max(0.0f, dot(normal, H));
    float spec = pow(NdotH, shininess);
    float3 specular = light.color * spec * light.intensity;

    // return the color contribution
    return float3(diffuse + specular);
}


float3 SpotlightColor(
    SpotLightData light,
    float3 worldPos,
    float3 normal, // normalized normal vector
    float3 viewDir, // direction from the camera
    float shininess // material shininess
)
{
    // calculate if it's within range
    float3 color = { 0.0f, 0.0f, 0.0f };

    float3 fragToLight = light.position - worldPos;
    float d = length(fragToLight);
    //if (d <= EPS)
    //{
    //    return color; // out of range
    //}
    //else
    //{
    float a = Attenuation(light.attenuation, d);
    //float rangeFade = saturate(1.0f - (d / light.range));

    //return float3(rangeFade, rangeFade, rangeFade); // debug range fade
    //a *= rangeFade;
    //if (a <= EPS)
    //{
    //    return color; // negligible contribution
    //}
    //else
    //{
        // cone factor
    float3 L = normalize(fragToLight);
    float cosTh = dot(L, normalize(-light.dir));
    float cone = saturate((cosTh - light.outerCos) / (light.innerCos - light.outerCos));
    
        // diffuse factor
    float NdotL = max(0.0f, dot(normal, L));
    float3 diffuse = light.color * NdotL * light.intensity;
    
        // specular factor
    float3 H = normalize(L + viewDir);
    float NdotH = max(0.0f, dot(normal, H));
    float3 specular = light.color * pow(NdotH, shininess) * light.intensity; // make specular not super bright
    
    color = (diffuse + specular) * cone * a;
    //}
    //}
    return color;
}

// Shadow calculation using shadow mapping with comparison sampling
// shadowMaps: Texture2DArray containing the shadow maps
// shadowSampler: SamplerState for sampling the shadow maps, with comparison enabled and set to LESS_EQUAL to determine what's in shadow
// position: World space position of the fragment
// shadowNo: Index of the shadow map to use (for multiple lights casting shadows)
// vp: View-projection matrix of the light source, inverted to transform to light clip space/view space

float FragVisibility
    (
    Texture2DArray<float> shadowMaps,
    SamplerComparisonState shadowSampler, // <- only type changed
    float3 position,
    int shadowNo,
    float4x4 vp)
{
    const float defaultLit = 1.0f;

    float4 lp = mul(float4(position, 1.0f), vp);
    if (lp.w <= 0.0f)
    {
        return defaultLit;
    }
    else
    {
    

    float3 ndc = lp.xyz / lp.w;
    float2 uv = float2(ndc.x * 0.5f + 0.5f, -ndc.y * 0.5f + 0.5f);
    float z = ndc.z;

    if (z <= 0.0f || z >= 1.0f || any(uv < 0.0f) || any(uv > 1.0f))
        return defaultLit;

    const float depthBias = 0.0005f;
    float slice = (float) shadowNo;

    // Built-in 2x2 PCF compare at LOD 0
    return shadowMaps.SampleCmpLevelZero(shadowSampler, float3(uv, slice), z - depthBias);
    }

}

struct LightResult
{
    float3 diff;
    float3 spec;
};
// TODO: rework this
float3 GetLightsColor(
    LightCount lightCount,
    StructuredBuffer<PointLightData> pointLights,
    StructuredBuffer<DirLightData> dirLights,
    StructuredBuffer<SpotLightData> spotLights,
    Texture2DArray<float> shadowMaps,
    SamplerComparisonState shadowSampler,
    float3 normal,
    float3 position,
    float3 viewDir,
    float shininess)
{
// output
    LightResult result;
    result.diff = float3(0, 0, 0);
    result.spec = float3(0, 0, 0);

    // lighting
    int shadows = 0; // to track how many shadows have been processed, even if there are lights that cast shadows we can't go over lightCount.shadowCount
    int i = 0;
    float3 plColor = { 0.0f, 0.0f, 0.0f };
    for (; i < lightCount.pointLightCount; ++i)
    {
        plColor += PointLightColor(
            pointLights[i],
            position,
            normal,
            viewDir,
            shininess
        );
    }
   //TODO: manage shadow count properly
    float3 dlColor = { 0.0f, 0.0f, 0.0f };
    for (i = 0; i < lightCount.dirLightCount; ++i)
    {
        float visibility = 1.0f; // default visibility
        if (i < lightCount.shadowCount) // if max shadows, skip shadow calculation
        {
            visibility = FragVisibility(shadowMaps, shadowSampler, position, i, dirLights[i].vp);
        }
        dlColor += DirLightColor(
            dirLights[i],
            position,
            normal,
            viewDir,
            shininess)
        * visibility;
    }
    
    float3 slColor = { 0.0f, 0.0f, 0.0f };
    for (i = 0; i < lightCount.spotLightCount; ++i)
    {
        float visibility = 1.0f; // default visibility
        int slice = i + lightCount.dirLightCount; // spotlights shadows come after dir light shadows
        if (slice < lightCount.shadowCount) // if max shadows, skip shadow calculation 
        {
            visibility = FragVisibility(shadowMaps, shadowSampler, position, slice, spotLights[i].vp);
        }
        slColor += SpotlightColor(
                spotLights[i],
                position,
                normal,
                viewDir,
                shininess)
        * visibility;
    }
    
    return (plColor + dlColor + slColor);
}
#endif // LIGHT_CALCULATIONS_HLSLI