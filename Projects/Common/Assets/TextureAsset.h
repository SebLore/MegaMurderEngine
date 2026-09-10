#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <Common/Assets/TextureFormats.h>

namespace Murder
{
    /// @brief Data structure for representing a texture asset on the CPU side, including metadata and optionally decoded pixel data.
    struct TextureAsset
    {
        // identity / source metadata
        std::string debugName;  // optional alias / asset name

        // image metadata
        uint32_t width        = 1; // default to 1x1 texture. should always be overriden
        uint32_t height       = 1;
        uint32_t depth        = 1; // 1 for regular 2D textures
        uint32_t mipCount     = 1; // 1 if no mip chain loaded on CPU
        uint32_t arraySize    = 1; // 1 for non-array textures
        uint32_t channelCount = 0; // source channels, if known
        uint32_t rowPitch     = 0; // bytes per row, if known (for calculating pixel offsets in uncompressed data)
        uint32_t slicePitch   = 0; // bytes per slice (for 3D textures), if known

        TextureFormat format = TextureFormat::Unknown;

        bool isSrgb = true;  // whether the texture is in sRGB color space, which affects sampling and gamma correction
        bool isCube = false; // whether this texture is a cubemap, which affects how it's sampled and used in shaders
        bool premultipliedAlpha = false; // whether the color channels have premultiplied alpha, which affects blending
        bool compressed       = false; // indicate if data stored in pixels is compressed

        // decoded pixel data (optional; can be empty if metadata-only or compressed blob path)
        std::vector<std::byte> pixels;

        [[nodiscard]] bool HasPixels() const noexcept { return !pixels.empty(); }

        [[nodiscard]] bool IsValid() const noexcept
        {
            return width > 0 && height > 0 && format != TextureFormat::Unknown;
        }
    };
} // namespace Murder
