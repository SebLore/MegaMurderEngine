#pragma once

#include <cstddef>
#include <span>
#include <string>

#include <Core/AssetTags.h>
#include <Common/Assets/ShaderStages.h>

namespace Murder
{
    struct ShaderUploadDesc
    {
        ShaderId                   shaderId{};
        ShaderStage                shaderStage = ShaderStage::Vertex;
        std::string                sourcePath{};
        std::span<const std::byte> byteCode{};
    };
} // namespace Murder
