#include "hlsli/BoundResources.hlsli"

PUN_SV main(PUN input)
{
    PUN_SV output;
    
    float4x4 wvp = mul(transform.world, camera.vp);
    
    output.sv_pos = mul(float4(input.position, 1.0f), wvp);
    output.position = mul(float4(input.position, 1.0f), transform.world).xyz;
    output.normal = normalize(mul(input.normal, (float3x3) transform.normal));
    output.uv = input.uv;
    return output;
}
