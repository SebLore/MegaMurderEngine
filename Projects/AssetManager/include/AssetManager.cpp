#include "AssetManager.h"

#include "Load/AssetPathUtils.h"

#include <Common/Geometry/Primitives.h>

#include <Utility/Logging.h>

#define LOG_TAG "AssetManager"

// STL
#include <filesystem>
#include <unordered_map>

namespace Murder
{
    namespace
    {
        /// create and add a default cube for fallback cases where a model doesn't exist
        MeshId CreateDefaultCube(AssetRegistry& registry, MeshCache& meshCache)
        {
            const MeshId meshId = registry.RegisterMemory<MeshId>("fallback:debug_cube");

            if (meshCache.TryGet(meshId.value) != nullptr)
                return meshId;

            std::vector<float>    vertices;
            std::vector<unsigned> indices;

            Primitives3D::GenerateCube<Primitives3D::PUN>(vertices, indices, 0.5f, true, true);

            MeshAsset mesh{};
            mesh.name            = "fallback_debug_cube";
            mesh.layout.stride   = sizeof(float) * 8;
            mesh.layout.elements = {
                VertexElement{ .semantic      = VertexSemantic::POSITION,
                               .semanticIndex = 0,
                               .format        = VertexFormat::FLOAT3,
                               .inputSlot     = 0,
                               .offset        = 0 },
                VertexElement{ .semantic      = VertexSemantic::TEXCOORD,
                               .semanticIndex = 0,
                               .format        = VertexFormat::FLOAT2,
                               .inputSlot     = 0,
                               .offset        = sizeof(float) * 3 },
                VertexElement{ .semantic      = VertexSemantic::NORMAL,
                               .semanticIndex = 0,
                               .format        = VertexFormat::FLOAT3,
                               .inputSlot     = 0,
                               .offset        = sizeof(float) * 5 },
            };

            mesh.vertexData.resize(vertices.size() * sizeof(float));
            std::memcpy(mesh.vertexData.data(), vertices.data(), mesh.vertexData.size());

            mesh.indices.reserve(indices.size());
            for (const auto idx : indices)
                mesh.indices.push_back(static_cast<uint32_t>(idx));

            mesh.submeshes.push_back(
                SubMeshCpu{ .indexStart = 0,
                            .indexCount = static_cast<uint32_t>(mesh.indices.size()),
                            .baseVertex = 0,
                            .name       = "fallback" });

            meshCache.Cache(meshId.value, std::make_unique<MeshAsset>(std::move(mesh)));
            LOG_DEBUG("Created fallback debug cube mesh for missing model resources");

            return meshId;
        }
    } // namespace

    AssetManager::AssetManager()
    {
        AssetRegistry::Dirs dirs = m_Registry.GetDirs();

        namespace fs = std::filesystem;

        const fs::path cwd          = fs::current_path();
        const fs::path candidates[] = {
            cwd / "assets",
            cwd / ".." / "assets",
            cwd / ".." / ".." / "assets",
            cwd / ".." / ".." / ".." / "assets",
        };

        for (const auto& candidate : candidates)
        {
            std::error_code ec;
            if (fs::exists(candidate, ec) && fs::is_directory(candidate, ec))
            {
                dirs.assetsRoot = fs::weakly_canonical(candidate).lexically_normal();
                break;
            }
        }

        m_Registry.SetDirs(std::move(dirs));
    }

    bool AssetManager::LoadModel(ModelId id, AssimpLoad::Config cfg)
    {
        // early out if model with id already exists
        if (m_ModelCache.TryGet(id.value) != nullptr)
            return true;

        // if model path does not exist (has not been registered) we exit
        const fs::path* modelPath = m_Registry.TryGetFullPath(id.value);
        if (modelPath == nullptr)
            return false;

        // if file does not exist at model path we exist
        std::error_code ec;
        if (!fs::exists(*modelPath, ec) || !fs::is_regular_file(*modelPath, ec))
            return false;

        AssimpLoad::LoadOutput out{};

        const auto result = AssimpLoad::LoadModelData(*modelPath, out, cfg);

        if (!result.ok)
            return false;

        // ensure unique ids
        std::unordered_map<std::string, MeshId>     meshIds;
        std::unordered_map<std::string, MaterialId> materialIds;
        std::unordered_map<std::string, TextureId>  textureIds;

        // -- Meshes --
        for (auto& [meshKey, mesh] : out.meshes)
        {
            MeshId meshId    = m_Registry.RegisterMemory<MeshId>(meshKey);
            meshIds[meshKey] = meshId;

            if (m_MeshCache.TryGet(meshId.value) == nullptr)
            {
                mesh.ownerModel = id;
                m_MeshCache.Cache(meshId.value, std::make_unique<MeshAsset>(std::move(mesh)));
            }
        }

        // -- Materials --

        // -- Textures --
        // Textures can either exist on file or be embedded in the data, we have to solve for both

        // Embedded textures we can just insert into the cache and register as memory assets
        for (const auto& [textureKey, texture] : out.embeddedTextures)
        {
            TextureId tid          = m_Registry.RegisterMemory<TextureId>(textureKey);
            textureIds[textureKey] = tid;

            if (m_TextureCache.TryGet(tid.value) == nullptr)
                m_TextureCache.Cache(tid.value, std::make_unique<TextureAsset>(texture));
        }

        // external textures (for .mtl files, for example) we need to find and attempt to load.
        for (const auto& [textureKey, texturePath] : out.texturePaths)
        {
            // use asset registry's Dirs to find where to look for texture files
            const fs::path resolvedPath = AssetPathUtils::ResolveAssetPath(
                AssetType::Texture,
                fs::path(texturePath),
                m_Registry.GetDirs(),
                modelPath);

            // register resolved file
            TextureId tid          = m_Registry.RegisterTexture(resolvedPath.generic_string());
            textureIds[textureKey] = tid;

            // load file bytes if not already cached
            if (m_TextureCache.TryGet(tid.value) == nullptr)
            {
                auto texture       = std::make_unique<TextureAsset>();
                texture->debugName = resolvedPath.generic_string();

                if (AssetPathUtils::ReadFileBytes(resolvedPath, texture->pixels))
                    texture->compressed = true;

                m_TextureCache.Cache(tid.value, std::move(texture));
            }

            // attempt to load and cache the texture
            if (!LoadTexture(tid))
                LOG_WARN(
                    "Failed to load external texture id=" << Core::ToHexString(tid.value) << " key='" << textureKey
                                                          << "' path='" << resolvedPath.string() << "'");
        }

        // -- Materials --

        for (const auto& [materialKey, materialEntry] : out.materials)
        {
            MaterialId materialId    = m_Registry.RegisterMemory<MaterialId>(materialKey);
            materialIds[materialKey] = materialId;

            MaterialAsset material = materialEntry.material;
            for (size_t slot = 0; slot < materialEntry.textureKeys.size(); ++slot)
            {
                const auto& key = materialEntry.textureKeys[slot];
                if (key.empty())
                    continue;

                if (const auto it = textureIds.find(key); it != textureIds.end())
                    material.textures[slot] = it->second;
            }

            if (m_MaterialCache.TryGet(materialId.value) == nullptr)
                m_MaterialCache.Cache(materialId.value, std::make_unique<MaterialAsset>(std::move(material)));
        }

        // -- Model --
        ModelAsset model{};
        model.name      = out.model.name;
        model.sourceKey = out.model.sourceKey;
        model.nodes     = out.model.nodes;
        model.bounds    = out.model.bounds;
        model.parts.reserve(out.model.parts.size());

        for (const auto& rawPart : out.model.parts)
        {
            ModelPart part{};
            part.name      = rawPart.name;
            part.group     = rawPart.group;
            part.nodeIndex = rawPart.nodeIndex;
            part.bounds    = rawPart.bounds;
            part.flags     = rawPart.flags;

            if (const auto meshIt = meshIds.find(rawPart.meshKey); meshIt != meshIds.end())
                part.meshId = meshIt->second;

            for (const auto& materialKey : rawPart.submeshMaterialKeys)
                if (const auto matIt = materialIds.find(materialKey); matIt != materialIds.end())
                    part.submeshMaterials.push_back(matIt->second);

            model.parts.push_back(std::move(part));
        }

        m_ModelCache.Cache(id.value, std::make_unique<ModelAsset>(std::move(model)));
        return true;
    }

    bool AssetManager::LoadTexture(TextureId id)
    {
        if (!id)
            return false;

        TextureAsset* texture = m_TextureCache.TryGet(id.value);
        if (texture == nullptr)
        {
            auto t = std::make_unique<TextureAsset>();
            try
            {
                t->debugName = std::string(m_Registry.GetNormalizedKey(id));
            }
            catch (...)
            {
                t->debugName = "texture_" + std::to_string(id.value);
            }

            m_TextureCache.Cache(id.value, std::move(t));
            texture = m_TextureCache.TryGet(id.value);
        }

        if (texture && texture->HasPixels())
            return true;

        AssetRegistry::AssetInfo info{};
        if (!m_Registry.TryGetInfo(id.value, info))
            return false;

        // Virtual/memory assets have no file path and should already carry their payload in cache.
        if (info.source != AssetRegistry::AssetSource::File || info.fullPath == nullptr)
            return false;

        if (!AssetPathUtils::ReadFileBytes(*info.fullPath, texture->pixels))
            return false;

        texture->compressed = true;
        return true;
    }

    bool AssetManager::TryGetTextureUpload(TextureId id, TextureUploadDesc& out)
    {
        if (!id)
            return false;

        if (!LoadTexture(id))
            return false;

        TextureAsset* texture = m_TextureCache.TryGet(id.value);

        AssetRegistry::AssetInfo info{};
        m_Registry.TryGetInfo(id.value, info);
        const fs::path* fullPath = (info.source == AssetRegistry::AssetSource::File) ? info.fullPath : nullptr;

        if (!texture->HasPixels())
            return false;

        // empty
        out           = TextureUploadDesc{};
        out.textureId = id;
        out.debugName = texture->debugName;
        if (fullPath)
            out.sourcePath = fullPath->generic_string();
        out.bytes     = texture->pixels;
        out.width     = texture->width;
        out.height    = texture->height;
        out.rowPitch  = texture->rowPitch;
        out.isEncoded = texture->compressed || texture->format == TextureFormat::Unknown || texture->rowPitch == 0;
        out.isSrgb    = texture->isSrgb;
        return true;
    }

    TextureId AssetManager::RegisterRuntimeTexture(std::string_view memoryKey, TextureAsset texture)
    {
        const TextureId id = m_Registry.RegisterMemory<TextureId>(memoryKey);

        if (texture.debugName.empty())
            texture.debugName = std::string(memoryKey);

        m_TextureCache.Cache(id.value, std::make_unique<TextureAsset>(std::move(texture)));
        return id;
    }

    bool AssetManager::TryGetShaderUpload(ShaderId id, ShaderStage stage, ShaderUploadDesc& out)
    {
        out = ShaderUploadDesc{};

        if (!id)
            return false;

        ShaderAsset* shader = m_ShaderCache.TryGet(id.value);
        if (shader == nullptr)
        {
            auto s         = std::make_unique<ShaderAsset>();
            s->sourceId    = id;
            s->sourceKind  = ShaderSourceKind::CSO;
            s->shaderStage = stage;
            m_ShaderCache.Cache(id.value, std::move(s));
            shader = m_ShaderCache.TryGet(id.value);
        }

        const auto* fullPath = m_Registry.TryGetFullPath(id.value);
        if (fullPath == nullptr)
            return false;

        if (shader->byteCode.empty())
        {
            if (!AssetPathUtils::ReadFileBytes(*fullPath, shader->byteCode))
                return false;
        }

        out.shaderId    = id;
        out.shaderStage = shader->shaderStage;
        out.sourcePath  = fullPath->generic_string();
        out.byteCode    = shader->byteCode;
        return true;
    }

    std::optional<AssetInventory> AssetManager::BuildAssetInventory(ModelId id)
    {
        if (!id)
            return std::nullopt;

        if (!LoadModel(id))
            return std::nullopt;

        const ModelAsset* model = TryGetModel(id);

        if (!model)
            return std::nullopt;

        AssetInventory inventory{};
        inventory.modelId = id;

        try
        {
            inventory.modelKey = std::string(m_Registry.GetNormalizedKey(id));
        }
        catch (...)
        {
            inventory.modelKey = "<unknown-model-key>";
        }

        const auto pushRef = [&](AssetRefKind kind, Core::AssetId idValue, std::vector<AssetRefInfo>& target)
        {
            AssetRefInfo ref{};
            ref.kind = kind;
            ref.id   = idValue;

            try
            {
                ref.key = std::string(m_Registry.GetNormalizedKey(idValue));
            }
            catch (...)
            {
                ref.key = "<unregistered>";
            }

            target.push_back(std::move(ref));
        };

        for (const auto& part : model->parts)
        {
            if (part.meshId)
                pushRef(AssetRefKind::Mesh, part.meshId.value, inventory.meshes);

            for (const auto materialId : part.submeshMaterials)
            {
                if (!materialId)
                    continue;

                pushRef(AssetRefKind::Material, materialId.value, inventory.materials);

                const MaterialAsset* material = TryGetMaterial(materialId);
                if (!material)
                    continue;

                for (const TextureId textureId : material->textures)
                {
                    if (!textureId)
                        continue;

                    pushRef(AssetRefKind::Texture, textureId.value, inventory.textures);
                }
            }
        }

        return inventory;
    }
} // namespace Murder

#undef LOG_TAG
