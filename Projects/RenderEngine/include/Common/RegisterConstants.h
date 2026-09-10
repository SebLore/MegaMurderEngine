// Should be updated to match Registers.hlsli
#pragma once
using CEXPRU = const unsigned int;
// -- Constant buffer (b#) registries -------------
CEXPRU R_TRANSFORM = 0;
CEXPRU R_CAMERA = 1;
CEXPRU R_MATERIAL = 2;
CEXPRU R_LIGHT_COUNT = 3;
CEXPRU R_FRUSTUM = 4; // binding the frustum cbuffer
CEXPRU R_LOD_PARAMS = 2;
CEXPRU R_FRAME_DATA = 5; // per-frame data
// -- Texture & structured buffer (t= #;) registries -
CEXPRU R_AMBIENT_MAP = 0;
CEXPRU R_DIFFUSE_MAP = 1;
CEXPRU R_SPECULAR_MAP = 2;
CEXPRU R_SHADOW_MAPS = 3;
CEXPRU R_POINT_LIGHTS = 4;
CEXPRU R_DIR_LIGHTS = 5;
CEXPRU R_SPOT_LIGHTS = 6;
CEXPRU R_GBUFFER0 = 7;
CEXPRU R_GBUFFER1 = 8;
CEXPRU R_GBUFFER2 = 9;
CEXPRU R_GBUFFER3 = 10;
CEXPRU R_CUBEMAP = 11;
// -- Sampler registries (s#) ---------------------
CEXPRU R_DEFAULT_SAMPLER = 0;
CEXPRU R_SHADOW_SAMPLER = 1;
CEXPRU R_CUBEMAP_SAMPLER = 2;
// -- Particle system registries ------------------
CEXPRU R_PARTICLE_DATA = 2;
CEXPRU R_PARTICLE_SRV = 0;
CEXPRU R_PARTICLE_TEX = 1;
CEXPRU R_PARTICLE_UAV = 0;
//-------------------------------------------------
