#pragma once

#include <cstdint>

namespace Murder
{
    enum class ShaderStage : uint8_t
    {
        Vertex,
        Pixel,
        Geometry,
        Hull,
        Domain, // DOMAIN is taken by CRT :(
        Compute
    };

    enum class RenderStage : uint8_t
    {
        PREPASS,
        MAIN,
        POSTPROCESS,
        UI,
    };

} // namespace Murder
