/*****************************************************************/ /**
 * @file AssetTraits.h
 * @brief Defines traits for assets to allow for simple insertion of "AssetId"
 * in AssetManager, i.e. RegisterMesh(string path) --> registered internally as AssetId, but
 * getting id for path returns the relevant id
 *
 * @author Sebastian
 * @date   February 2026
 *********************************************************************/
#pragma once

#include <cstdint>

#include <Core/AssetTags.h>

/// add more as needed, should match AssetTags.h in Core
enum class AssetType : uint8_t
{
    Mesh,
    Texture,
    Material,
    Model,
    Shader
};

/// Primary template
template <class Id>
struct AssetTraits;

// specializations
template <>
struct AssetTraits<MeshId>
{
    static constexpr AssetType type = AssetType::Mesh;
};

template <>
struct AssetTraits<TextureId>
{
    static constexpr AssetType type = AssetType::Texture;
};

template <>
struct AssetTraits<MaterialId>
{
    static constexpr AssetType type = AssetType::Material;
};

template <>
struct AssetTraits<ModelId>
{
    static constexpr AssetType type = AssetType::Model;
};

template <>
struct AssetTraits<ShaderId>
{
    static constexpr AssetType type = AssetType::Shader;
};
