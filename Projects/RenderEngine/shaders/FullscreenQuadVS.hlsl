struct VSOut
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VSOut main(uint vid : SV_VertexID)
{
    // Clip-space quad
    float2 pos[4] =
    {
        { -1.0f, 1.0f }, // top-left
        { 1.0f, 1.0f }, // top-right
        { -1.0f, -1.0f }, // bottom-left
        { 1.0f, -1.0f } // bottom-right
    };

    // Texture UVs (0,0) top-left in D3D
    float2 uv[4] =
    {
        { 0.0f, 0.0f }, // top left
        { 1.0f, 0.0f }, // top right
        { 0.0f, 1.0f }, // bottom left
        { 1.0f, 1.0f } // bottom right
    };

    VSOut o;
    o.pos = float4(pos[vid], 0.0f, 1.0f);
    o.uv = uv[vid];
    return o;
}