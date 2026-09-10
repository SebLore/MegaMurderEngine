#ifndef SHADER_STRUCTURES_HLSLI
#define SHADER_STRUCTURES_HLSLI

// -- Vertex In-data ------------------------------------------------
struct PC
{
    float3 position : POSITION;
    float3 color : COLOR;
};


struct PN
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct PUN
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

// -- Vertex out-data/Pixel in-data ---------------------------------
struct COLOR_SV
{
    float4 sv_pos : SV_POSITION;
    float3 color : COLOR;
};
struct PUN_SV
{
    float4 sv_pos : SV_POSITION;
    float3 position : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

// -- Hull Shader structures -----------------------------------------
struct HSConstData // patch constant data output
{
    float EdgeTess[3] : SV_TessFactor;
    float InsideTess : SV_InsideTessFactor;
};

struct GBUFFER_OUT // output struct for geometry pass pixel shader
{
    float4 pos : SV_TARGET0; // position + uv u
    float4 normal : SV_TARGET1; // normal + uv v
    float4 diffuse : SV_TARGET2; // diffuse color + shininess
    float4 specular : SV_TARGET3; // specular color + ambient factor
};

// -- Buffer data types ---------------------------------------------
struct TransformData
{
    row_major float4x4 world;
    column_major float4x4 normal;
};

struct CameraProperties
{
    row_major float4x4 view;
    row_major float4x4 projection;
    row_major float4x4 vp;
    float3 position;
    float dt;
};

// material data bound to b2
struct MaterialProperties
{
    float3 ambient;
    float shininess;
    float3 diffuse;
    float alpha;
    float3 specular;
    float reflect;
};

// Lights (bound to b3)
struct LightCount
{
    int pointLightCount;
    int dirLightCount;
    int spotLightCount;
    int shadowCount;
};

struct PointLightData
{
    // float4x4 vp[6]; // TODO: implement shadows
    float3 position;
    float range;
    float3 color;
    float intensity;
    float3 attenuation;
    float padding;
};

struct DirLightData
{
    row_major float4x4 vp;
    float3 dir;
    float intensity;
    float3 color;
    float padding;
};

struct SpotLightData
{
    row_major float4x4 vp;
    float3 dir;
    float intensity;
    float3 color;
    float range;
    float3 position;
    float innerCos;
    float3 attenuation;
    float outerCos;
};

struct TesselationParams
{
    float minD; // distance at which min tessellation is used
    float maxD; // distance at which max tessellation is used
    float minT; // minimum tessellation factor
    float maxT; // maximum tessellation factor
};
#endif // SHADER_STRUCTURES_HLSLI