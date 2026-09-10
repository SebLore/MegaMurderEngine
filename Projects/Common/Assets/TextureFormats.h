#pragma once

#include <cstdint>

namespace Murder
{
    enum class TextureFormat : uint8_t
    {
        Unknown = 0,

        R8_UNORM,
        RG8_UNORM,
        RGB8_UNORM,
        RGBA8_UNORM,
        BGRA8_UNORM,

        R16_FLOAT,
        RG16_FLOAT,
        RGBA16_FLOAT,

        R32_FLOAT,
        RG32_FLOAT,
        RGBA32_FLOAT,
    };
}