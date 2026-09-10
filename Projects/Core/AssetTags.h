#pragma once

#include "AssetId.h"

/// Basic means to categorize assets by type, without needing to create separate AssetId types for each.
template <class Tag>
struct TaggedAssetId
{
    Core::AssetId value = 0; ///< A value of 0 for AssetId is invalid

    /// Check if the ID is valid
    explicit constexpr operator bool() const noexcept { return value != 0; }

    // for unordered_map key equality and other comparisons, we just compare the underlying AssetId value
    friend constexpr bool operator==(TaggedAssetId a, TaggedAssetId b) noexcept { return a.value == b.value; }
    friend constexpr bool operator!=(TaggedAssetId a, TaggedAssetId b) noexcept { return a.value != b.value; }
};

// clang-format off
// empty structs for tags, we add more as needed
struct MeshTag {};
struct ModelTag {};
struct TextureTag {};
struct MaterialTag {};
struct ShaderTag {};
// clang-format on

using MeshId     = TaggedAssetId<MeshTag>;
using ModelId    = TaggedAssetId<ModelTag>;
using TextureId  = TaggedAssetId<TextureTag>;
using MaterialId = TaggedAssetId<MaterialTag>;
using ShaderId   = TaggedAssetId<ShaderTag>;

// allow for hashing with std::unordered_map
namespace std
{
    template <class Tag>
    struct hash<TaggedAssetId<Tag>>
    {
        size_t operator()(const TaggedAssetId<Tag>& id) const noexcept { return std::hash<Core::AssetId>{}(id.value); }
    };
} // namespace std
