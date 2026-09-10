#pragma once

#include <optional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <cstddef>

#include "Common/Render/MeshGpu.h"
#include "Core/AssetTags.h"

#include <Common/Bridge/TextureUploadDesc.h>

#include "Loader/GpuLoader.h"

#include <Abstraction/Shader.h>
#include <Abstraction/ShaderResourceView.h>

namespace Murder
{
    struct MeshAsset; // forward declare to prevent dependency in header

}
namespace Murder::Render
{

    /**
    * @brief Render backend class, responsible for holding and managing gpu resources. The renderer issues
    * draw calls and backend yields to the backend to execute them, allowing the backend to manage resources and state as needed.
    */
    class RenderBackend
    {
      public:
        RenderBackend(ID3D11Device* device, ID3D11DeviceContext* context);

        // -- Mesh management --
        const MeshGpu& GetOrCreateMesh(MeshId id, const MeshAsset& mesh);
        const MeshGpu* FindMesh(MeshId id) const noexcept;

        bool HasMesh(MeshId id) const noexcept;
        bool HasTexture(TextureId id) const noexcept;

        const std::shared_ptr<DX::TextureSRV>& GetOrCreateTexture(TextureId id, const TextureUploadDesc& upload);
        const std::shared_ptr<DX::TextureSRV>* FindTexture(TextureId id) const noexcept;
        const std::shared_ptr<DX::TextureSRV>& GetDefaultTexture() const { return m_DefaultTexture; }

        void EraseMesh(MeshId id) { m_Meshes.erase(id); }
        void ClearMeshes() { m_Meshes.clear(); }
        void EraseTexture(TextureId id) { m_Textures.erase(id); }
        void ClearTextures()
        {
            m_Textures.clear();
            m_TextureLoadFailed.clear();
        }

        // -- Render helpers --
        void               BindMesh(const MeshGpu& mesh) const;
        void               DrawMesh(const MeshGpu& mesh) const;
        void               DrawSubMesh(const MeshGpu& mesh, uint32_t submeshIndex) const;
        ID3D11InputLayout* GetOrCreateInputLayout(MeshId meshId, const MeshGpu& mesh, const DX::Shader& vs);

        size_t InputLayoutCacheSize() const { return m_InputLayouts.size(); }

        // Clear caches
        void ClearCaches()
        {
            ClearMeshes();
            ClearTextures();
            m_InputLayouts.clear();
            m_InputLayoutCreateFailed.clear();
        }

        // Reset resources
        void Reset() { ClearCaches(); }

      private:
        struct InputLayoutKey
        {
            Core::AssetId meshId         = 0;
            const void*   vsByteCode     = nullptr;
            size_t        vsByteCodeSize = 0;

            bool operator==(const InputLayoutKey& other) const
            {
                return meshId == other.meshId && vsByteCode == other.vsByteCode &&
                       vsByteCodeSize == other.vsByteCodeSize;
            }
        };

        struct InputLayoutKeyHash
        {
            size_t operator()(const InputLayoutKey& key) const
            {
                const size_t h1 = std::hash<Core::AssetId>{}(key.meshId);
                const size_t h2 = std::hash<const void*>{}(key.vsByteCode);
                const size_t h3 = std::hash<size_t>{}(key.vsByteCodeSize);
                return h1 ^ (h2 << 1) ^ (h3 << 2);
            }
        };

        struct CreateResult
        {
            bool                       success = false;
            std::optional<std::string> error;
        };

      private:
        CreateResult CreateMeshGpu(const MeshAsset& mesh, MeshGpu& out) const;

      private:
        ID3D11Device*        m_Device  = nullptr; // non-owning
        ID3D11DeviceContext* m_Context = nullptr; // non-owning

        GpuLoader m_GpuLoader;

        // caching
        std::unordered_map<MeshId, MeshGpu>                                               m_Meshes;
        std::unordered_map<TextureId, std::shared_ptr<DX::TextureSRV>>                    m_Textures;
        std::shared_ptr<DX::TextureSRV>                                                   m_DefaultTexture;
        std::unordered_map<InputLayoutKey, ComPtr<ID3D11InputLayout>, InputLayoutKeyHash> m_InputLayouts;

        // keep track of failed loads TODO: implement handles instead
        std::unordered_set<TextureId>                          m_TextureLoadFailed;
        std::unordered_set<InputLayoutKey, InputLayoutKeyHash> m_InputLayoutCreateFailed;
    };
} // namespace Murder::Render
