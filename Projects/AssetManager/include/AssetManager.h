/**
 * @file AssetManager.h
 *
 * @author Sebastian L
 * @date feb 2026
 */
#pragma once

#include "Registry/AssetRegistry.h"
#include "Cache/AssetCache.h"

#include <Common/Scene/ModelInstance.h>
#include <Common/Bridge/TextureUploadDesc.h>
#include <Common/Bridge/ShaderUploadDesc.h>
#include <Common/Assets/ShaderStages.h>

#include <Load/AssimpLoader.h>

#include <optional>

namespace Murder
{
    // forward declaration
    struct ModelComponent;

    // Categorization of asset reference types for AssetInventory
    enum class AssetRefKind : uint8_t
    {
        Mesh,
        Material,
        Texture,
        Shader,
    };

    // A single asset reference in the AssetInventory
    struct AssetRefInfo
    {
        AssetRefKind  kind{ AssetRefKind::Mesh };
        Core::AssetId id{};
        std::string   key{};
    };

    // Data type to hold an inventory of all assets referenced by a model, for debugging
    struct AssetInventory
    {
        ModelId                   modelId{};
        std::string               modelKey{};
        std::vector<AssetRefInfo> meshes;
        std::vector<AssetRefInfo> materials;
        std::vector<AssetRefInfo> textures;

        static const char* ToString(AssetRefKind kind)
        {
            switch (kind)
            {
            case AssetRefKind::Mesh:
                return "Mesh";
            case AssetRefKind::Material:
                return "Material";
            case AssetRefKind::Texture:
                return "Texture";
            default:
                return "Unknown";
            }
        }
    };

    // clang-format off
    class AssetManager
    {
    public:
        AssetManager();
        ~AssetManager() = default;

        // non-copyable, non-movable
        AssetManager(const AssetManager&) = delete;
        AssetManager& operator=(const AssetManager&) = delete;
        AssetManager(AssetManager&&) = delete;
        AssetManager& operator=(AssetManager&&) = delete;

        // -- Register Assets --
        [[nodiscard]] MeshId RegisterMesh(std::string_view relativePath) { return m_Registry.RegisterMesh(relativePath); }
        [[nodiscard]] ModelId RegisterModel(std::string_view relativePath) { return m_Registry.RegisterModel(relativePath); }
        [[nodiscard]] TextureId RegisterTexture(std::string_view relativePath) { return m_Registry.RegisterTexture(relativePath); }
        [[nodiscard]] MaterialId RegisterMaterial(std::string_view relativePath) { return m_Registry.RegisterMaterial(relativePath); }
        [[nodiscard]] ShaderId RegisterShader(std::string_view relativePath) { return m_Registry.RegisterShader(relativePath); }
        [[nodiscard]] TextureId RegisterRuntimeTexture(std::string_view memoryKey, TextureAsset texture);

        // -- Load Assets --
        bool LoadModel(ModelId id, AssimpLoad::Config cfg = {});
        bool LoadTexture(TextureId id);

        // For render upload
        bool TryGetTextureUpload(TextureId id, TextureUploadDesc& out);
        bool TryGetShaderUpload(ShaderId id, ShaderStage stage, ShaderUploadDesc& out);

        // create an inventory of all assets referenced by a model, for debugging and auditing purposes
        std::optional<AssetInventory> BuildAssetInventory(ModelId id);

        // -- Cache Query --
        [[nodiscard]] const ModelAsset* TryGetModel(ModelId id) const { return m_ModelCache.TryGet(id.value); }
        [[nodiscard]] const MeshAsset* TryGetMesh(MeshId id) const { return m_MeshCache.TryGet(id.value); }
        [[nodiscard]] const MaterialAsset* TryGetMaterial(MaterialId id) const { return m_MaterialCache.TryGet(id.value); }
        [[nodiscard]] const TextureAsset* TryGetTexture(TextureId id) const { return m_TextureCache.TryGet(id.value); }

        // -- Getters --
        AssetRegistry& Registry() { return m_Registry; }
        const AssetRegistry& Registry() const { return m_Registry; }

        void Clear()
        {
            m_ModelCache.Clear();
            m_MeshCache.Clear();
            m_MaterialCache.Clear();
            m_TextureCache.Clear();
            m_ShaderCache.Clear();
            m_Registry.Clear();
        }

    private:
        // -- Logging --
        // TODO: move to dedicated class or something

    private:
        AssetRegistry m_Registry;

        // -- Caching --
        ModelCache    m_ModelCache;
        MeshCache     m_MeshCache;
        MaterialCache m_MaterialCache;
        TextureCache  m_TextureCache;
        ShaderCache   m_ShaderCache;
    };
    // clang-format on

} // namespace Murder
