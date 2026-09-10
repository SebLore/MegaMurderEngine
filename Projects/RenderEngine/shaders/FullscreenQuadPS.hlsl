Texture2D offscreenTex : register(t0);
SamplerState samp : register(s0);

struct VSOut
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(VSOut input) : SV_TARGET
{
    return offscreenTex.Sample(samp, input.uv);
}