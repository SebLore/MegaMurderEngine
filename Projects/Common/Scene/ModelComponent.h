#pragma once

#include <vector>

#include <Core/Math/Transform.h>
#include <Core/AssetTags.h>

#include <Common/Assets/MaterialAsset.h>

namespace Murder
{

    // override for a single Texture
    struct TextureOverride
    {
        uint32_t partIndex    = 0; // which ModelCpu::parts
        uint32_t submeshIndex = 0; // which submesh inside that mesh/part

        TextureSlot slot{};
        TextureId   texture{};
    };

    struct MaterialOverride
    {
        uint32_t partIndex    = 0;
        uint32_t submeshIndex = 0;

        MaterialId material{};
    };

    /**
     * @brief Authoritative ECS-side model state.
     *
     * This is the gameplay/authored input for an entity:
     * - which model asset to use (`modelId`),
     * - where it is (`transform`
     * - optional overrides
     */

    struct ModelComponent
    {
        ModelId id{};

        Transform transform{};

        // overrides for specific materials/textures
        std::vector<MaterialOverride> materialOverrides;
        std::vector<TextureOverride>  textureOverrides;
    };
} // namespace Murder
