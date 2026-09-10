// Should be updated to match ResgisterConstants.h
#ifndef REGISTERS_HLSLI
#define REGISTERS_HLSLI
// -- Constant buffer registries ------------------
#define TRANSFORM b0
#define CAMERA b1
#define MATERIAL b2
#define LIGHT_COUNT b3
#define FRUSTUM b4
#define LOD_PARAMS b2
#define FRAME_DATA b5
// -- Texture & structured buffer registries ------------------
#define AMBIENT_MAP t0
#define DIFFUSE_MAP t1
#define SPECULAR_MAP t2
#define SHADOW_MAPS t3
#define POINT_LIGHTS t4
#define DIR_LIGHTS t5
#define SPOT_LIGHTS t6
#define GBUFFER0 t7
#define GBUFFER1 t8
#define GBUFFER2 t9
#define GBUFFER3 t10
#define CUBEMAP t11
// -- Sampler registries ------------------
#define DEFAULT_SAMPLER s0
#define SHADOW_SAMPLER s1
#define CUBEMAP_SAMPLER s2
// -- Unordered Access View registries ------------------
#define BACKBUFFER_UAV u0
// Particle is a separate pipeline from the rest
#define PARTICLE_DATA b2
#define PARTICLE_SRV t0
#define PARTICLE_TEX t1
#define PARTICLE_UAV u0
#endif // REGISTERS_HLSLI
