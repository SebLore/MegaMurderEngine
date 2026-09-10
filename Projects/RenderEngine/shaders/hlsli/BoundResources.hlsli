#ifndef BOUND_RESOURCES_HLSLI
#define BOUND_RESOURCES_HLSLI

#include "Registers.hlsli"
#include "Structures.hlsli"

#define CP_COUNT 3

// -- buffers ---
cbuffer Transform : register(TRANSFORM)
{
    TransformData transform;
}
cbuffer Camera : register(CAMERA)
{
    CameraProperties camera;
}

cbuffer Material : register(MATERIAL)
{
    MaterialProperties material;
}

cbuffer LightCount : register(LIGHT_COUNT)
{
    LightCount lightCount;
}

// Frustum buffer for culling, bound to b4. used to invert camera frustum from viewproj to world space
cbuffer FrustumBuffer : register(FRUSTUM)
{
    CameraProperties frustum; // view and proj need to be inverted on the cpu side before being sent in
};

Texture2D ambientMap : register(AMBIENT_MAP);
Texture2D diffuseMap : register(DIFFUSE_MAP);
Texture2D specularMap : register(SPECULAR_MAP);

Texture2DArray<float> shadowMaps : register(SHADOW_MAPS);

SamplerState defaultSampler : register(DEFAULT_SAMPLER);
//SamplerState shadowSampler : register(SHADOW_SAMPLER);
SamplerComparisonState shadowSampler : register(SHADOW_SAMPLER);

StructuredBuffer<PointLightData> pointLights : register(POINT_LIGHTS);
StructuredBuffer<DirLightData> dirLights : register(DIR_LIGHTS);
StructuredBuffer<SpotLightData> spotLights : register(SPOT_LIGHTS);

RWTexture2D<float4> backBufferUAV : register(BACKBUFFER_UAV);
#endif