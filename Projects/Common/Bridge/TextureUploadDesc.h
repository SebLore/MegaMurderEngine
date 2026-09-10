#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>

#include <Core/AssetTags.h>

namespace Murder
{
    struct TextureUploadDesc
    {
        TextureId                  textureId{};
        std::string_view           debugName{};
        std::string                sourcePath{};
        std::span<const std::byte> bytes{};
        uint32_t                   width = 0;
        uint32_t                   height = 0;
        uint32_t                   rowPitch = 0;
        bool                       isEncoded = true;
        bool                       isSrgb = true;
    };
}
