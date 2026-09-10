#pragma once

#include <array>
#include <cstdint>
#include <string>

#include <Core/Math/Types.h>
#include <Core/AssetTags.h>

namespace Murder
{
    enum class TextureSlot : uint8_t
    {
        Ambient = 0,
        Diffuse,
        Specular,
        Normal,
        Emissive,
        Occlusion,
        Roughness,
        Metallic,
        Count
    };

    inline constexpr std::size_t TextureSlotCount = static_cast<std::size_t>(TextureSlot::Count);
    constexpr std::size_t        ToIndex(TextureSlot s) { return static_cast<std::size_t>(s); }

    enum class ShadingModel : uint8_t
    {
        Default = 0,
        // TODO: add more when/if this is used
    };

    struct MaterialProperties
    {
        // regular phong values
        Vector3 ambient   = { 1.0f, 1.0f, 1.0f };
        float   shininess = 32.0f;

        Vector3 diffuse  = { 1.0f, 1.0f, 1.0f };
        float   dissolve = 1.0f;

        Vector3 specular = { 0.0f, 0.0f, 0.0f };
        float   reflect  = 0.0f;

        //TODO: implement more modern-ish materials used in PBR if there is time
        Vector3 emissive  = { 0.0f, 0.0f, 0.0f };
        float   roughness = 1.0f;

        float metallic = 0.0f;
        float ao = 1.0f; // occlusion multiplier, not a map, since it can be baked into other maps or vertex colors
    };

    struct MaterialAsset
    {
        // identity / import metadata
        std::string name; ///< material name

        MaterialProperties properties{};

        // Runtime asset references (preferred after registration)
        std::array<TextureId, TextureSlotCount> textures{};

        // CPU-only metadata flags/tags
        uint32_t     flags   = 0;
        ShadingModel shading = ShadingModel::Default;

        [[nodiscard]] bool HasTexture(TextureSlot slot) const noexcept
        {
            return static_cast<bool>(textures[ToIndex(slot)]);
        }
    };

} // namespace Murder
