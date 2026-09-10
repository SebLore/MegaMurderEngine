#ifndef PARTICLE_HLSLI
#define PARTICLE_HLSLI

#include "Registers.hlsli"

// raw particle input with the structured buffer
struct Particle
{
    float3 position;
    float3 velocity;
    float lifetime;
    float age;
};

struct VS_PARTICLE
{
    float3 position : POSITION;
    float lifetime : LIFETIME;
    float age : AGE;
};

struct GS_PARTICLE
{
    float4 vpos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
};

// PS
cbuffer ParticleData : register(PARTICLE_DATA)
{
    float dt;
    uint particleCount;
    int usingTexture;
    float padding2;
}

// CS
RWStructuredBuffer<Particle> rw_Particles : register(PARTICLE_UAV); // unordered access buffer

// VS
StructuredBuffer<Particle> particles : register(PARTICLE_SRV);
Texture2D particleTex : register(PARTICLE_TEX);

#endif
