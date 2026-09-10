// AssimpLoader.h
#pragma once

#include <Common/Assets/ModelAsset.h>
#include <Common/Assets/MeshAsset.h>
#include <Common/Assets/MaterialAsset.h>
#include <Common/Assets/TextureAsset.h>

// STL
#include <string>
#include <unordered_map>
#include <vector>
#include <array>
#include <filesystem>
#include <optional>

// TODO: use this to load the data directed instead of using the Loaded* resources
#include "Registry/AssetRegistry.h"

namespace AssimpLoad
{
    template <typename T>
    using StringMap = std::unordered_map<std::string, T>;

    struct LoadedMaterial
    {
        Murder::MaterialAsset                             material;
        std::array<std::string, Murder::TextureSlotCount> textureKeys{};
    };

    struct LoadedModelPart
    {
        std::string name;
        std::string group;

        std::string              meshKey;
        std::vector<std::string> submeshMaterialKeys;

        int32_t nodeIndex = -1;

        std::optional<Murder::AABB> bounds;

        Murder::ModelPartFlags flags = Murder::ModelPartFlags::None;
    };

    struct LoadedModel
    {
        std::string name;
        std::string sourceKey;

        std::vector<Murder::ModelNode> nodes;
        std::vector<LoadedModelPart>   parts;

        std::optional<Murder::AABB> bounds;
    };

    struct Config
    {
        float globalScale = 1.0f;

        bool convertToLeftHanded = true;
        bool triangulate         = true;
        bool genNormalsIfMissing = true;
        bool calcTangents        = false; // requires aiProcess_CalcTangentSpace
        bool genBoundingBoxes    = false;
        bool excludeFaceless     = true;
    };

    struct LoadOutput
    {
        LoadedModel model;

        StringMap<Murder::MeshAsset>    meshes;
        StringMap<LoadedMaterial>       materials;
        StringMap<Murder::TextureAsset> embeddedTextures;
        StringMap<std::string>          texturePaths;
    };

    struct LoadResult
    {
        bool        ok = false;
        std::string error;
    };

    LoadResult LoadModelData(const std::filesystem::path& modelPath, LoadOutput& out, const Config& cfg = {});

} // namespace AssimpLoad
