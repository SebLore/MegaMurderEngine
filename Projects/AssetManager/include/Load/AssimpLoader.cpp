// AssimpLoader.cpp
#include "AssimpLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <filesystem>
#include <sstream>
#include <cstring>

#include <Utility/Logging.h>

#include "AssimpMeshExtract.h" // BuildMeshCpuFromAssimp(...)

namespace fs = std::filesystem;

#define LOG_TAG "AssimpLoader"

namespace AssimpLoad
{

    namespace
    {
        using namespace Murder;

        std::string SubKey(std::string_view modelKey, std::string_view category, uint32_t index)
        {
            std::ostringstream oss;
            oss << modelKey << "#" << category << ":" << index;
            return oss.str();
        }

        std::string SubKeyEmbeddedTex(std::string_view modelKey, std::string_view token) // e.g. "*0"
        {
            std::ostringstream oss;
            oss << modelKey << "#tex:" << token;
            return oss.str();
        }

        std::string SubKeyNamedTex(std::string_view modelKey, std::string_view name)
        {
            std::ostringstream oss;
            oss << modelKey << "#tex:name:" << name;
            return oss.str();
        }

        fs::path ResolveTexturePath(const fs::path& modelFilePath, const std::string& texPath)
        {
            fs::path p(texPath);

            // embedded "*0"
            if (!texPath.empty() && texPath[0] == '*')
                return p;

            if (p.is_absolute())
                return p.lexically_normal();

            fs::path modelRelative = (modelFilePath.parent_path() / p).lexically_normal();

            std::error_code ec;
            if (fs::exists(modelRelative, ec))
                return modelRelative;

            if (ec)
                LOG_WARN(ec.message());

            return {};
        }

        std::optional<TextureSlot> MapTextureType(aiTextureType t)
        {
            switch (t)
            {
            case aiTextureType_UNKNOWN:
                return TextureSlot::Diffuse;
            case aiTextureType_BASE_COLOR:
                return TextureSlot::Diffuse;
            case aiTextureType_AMBIENT:
                return TextureSlot::Ambient;
            case aiTextureType_DIFFUSE:
                return TextureSlot::Diffuse;
            case aiTextureType_SPECULAR:
                return TextureSlot::Specular;
            case aiTextureType_NORMALS:
                return TextureSlot::Normal;
            case aiTextureType_EMISSIVE:
                return TextureSlot::Emissive;
            case aiTextureType_AMBIENT_OCCLUSION:
                return TextureSlot::Occlusion;

                // PBR-ish slots (depends on importer)
            case aiTextureType_DIFFUSE_ROUGHNESS:
                return TextureSlot::Roughness;
            case aiTextureType_METALNESS:
                return TextureSlot::Metallic;
            default:
                return std::nullopt;
            }
        }

        void FillTexture(const aiTexture& src, TextureAsset& out)
        {
            if (src.mHeight == 0)
            {
                out.compressed = true;
                out.pixels.resize(src.mWidth);
                std::memcpy(out.pixels.data(), src.pcData, src.mWidth);
                return;
            }

            out.width        = src.mWidth;
            out.height       = src.mHeight;
            out.channelCount = 4;
            out.rowPitch     = src.mWidth * 4;
            out.slicePitch   = out.rowPitch * out.height;
            out.format       = TextureFormat::RGBA8_UNORM;

            const size_t byteSize =
                static_cast<size_t>(src.mWidth) * static_cast<size_t>(src.mHeight) * sizeof(aiTexel);

            out.pixels.resize(byteSize);
            std::memcpy(out.pixels.data(), src.pcData, byteSize);
        }

        void StoreEmbeddedTexture(
            StringMap<TextureAsset>& embeddedTextures,
            const std::string&       texKey,
            const aiTexture&         embedded)
        {
            if (embeddedTextures.contains(texKey))
                return;

            TextureAsset tex{};
            tex.debugName = texKey;
            FillTexture(embedded, tex);
            embeddedTextures.emplace(texKey, std::move(tex));
        }

        /// convert aimatrix to SimpleMath Matrix
        Matrix ToMatrix(const aiMatrix4x4& m)
        {
            return Matrix{ m.a1, m.b1, m.c1, m.d1, m.a2, m.b2, m.c2, m.d2,
                           m.a3, m.b3, m.c3, m.d3, m.a4, m.b4, m.c4, m.d4 };
        }

        void LoadMaterials(std::string_view modelKey, const fs::path& modelPath, const aiScene* scene, LoadOutput& out)
        {
            out.materials.clear();
            out.materials.reserve(scene->mNumMaterials);

            constexpr aiTextureType typesToCheck[] = {
                aiTextureType_UNKNOWN,   aiTextureType_BASE_COLOR,        aiTextureType_AMBIENT,
                aiTextureType_DIFFUSE,   aiTextureType_SPECULAR,          aiTextureType_NORMALS,
                aiTextureType_EMISSIVE,  aiTextureType_AMBIENT_OCCLUSION, aiTextureType_DIFFUSE_ROUGHNESS,
                aiTextureType_METALNESS,
            };

            for (unsigned mi = 0; mi < scene->mNumMaterials; ++mi)
            {
                const aiMaterial* m = scene->mMaterials[mi];
                if (!m)
                    continue;

                const std::string matKey = SubKey(modelKey, "mat", mi);

                LoadedMaterial mat{};

                // name
                {
                    aiString n;
                    if (m->Get(AI_MATKEY_NAME, n) == AI_SUCCESS)
                        mat.material.name = n.C_Str();
                    else
                        mat.material.name = "material_" + std::to_string(mi);
                }

                // basic properties (optional; extend later)
                {
                    aiColor3D c;
                    if (m->Get(AI_MATKEY_COLOR_AMBIENT, c) == AI_SUCCESS)
                        mat.material.properties.ambient = Vector3{ c.r, c.g, c.b };
                    if (m->Get(AI_MATKEY_COLOR_DIFFUSE, c) == AI_SUCCESS)
                        mat.material.properties.diffuse = Vector3{ c.r, c.g, c.b };
                    if (m->Get(AI_MATKEY_COLOR_SPECULAR, c) == AI_SUCCESS)
                        mat.material.properties.specular = Vector3{ c.r, c.g, c.b };
                    if (m->Get(AI_MATKEY_COLOR_EMISSIVE, c) == AI_SUCCESS)
                        mat.material.properties.emissive = Vector3{ c.r, c.g, c.b };

                    float shininess = mat.material.properties.shininess;
                    if (m->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
                        mat.material.properties.shininess = shininess;

                    float opacity = mat.material.properties.dissolve;
                    if (m->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS)
                        mat.material.properties.dissolve = opacity;

                    float roughness = mat.material.properties.roughness;
                    if (m->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS)
                        mat.material.properties.roughness = roughness;

                    float metallic = mat.material.properties.metallic;
                    if (m->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS)
                        mat.material.properties.metallic = metallic;
                }

                // textures (first texture per type for now)
                for (aiTextureType tt : typesToCheck)
                {
                    const auto slotOpt = MapTextureType(tt);
                    if (!slotOpt)
                        continue;

                    aiString texAi;
                    if (m->GetTexture(tt, 0, &texAi) != AI_SUCCESS)
                        continue;

                    const std::string texPath = texAi.C_Str();
                    const size_t      slot    = ToIndex(*slotOpt);

                    const aiTexture* embedded = nullptr;
                    std::string      texKey;

                    // Assimp embedded token, e.g. "*0"
                    if (!texPath.empty() && texPath[0] == '*')
                    {
                        texKey   = SubKeyEmbeddedTex(modelKey, texPath);
                        embedded = scene->GetEmbeddedTexture(texPath.c_str());
                    }
                    // Named embedded texture
                    else if ((embedded = scene->GetEmbeddedTexture(texPath.c_str())) != nullptr)
                    {
                        texKey = SubKeyNamedTex(modelKey, texPath);
                    }

                    if (embedded != nullptr)
                    {
                        mat.textureKeys[slot] = texKey;
                        StoreEmbeddedTexture(out.embeddedTextures, texKey, *embedded);
                        continue;
                    }

                    // external file
                    const fs::path resolved = ResolveTexturePath(modelPath, texPath);
                    texKey                  = SubKeyNamedTex(modelKey, texPath);
                    mat.textureKeys[slot]   = texKey;
                    out.texturePaths.try_emplace(texKey, resolved.generic_string());
                }

                out.materials.emplace(matKey, std::move(mat));

                const auto& diffuseKey = out.materials.at(matKey).textureKeys[ToIndex(TextureSlot::Diffuse)];
                LOG_DEBUG(
                    "Material[" << mi << "] name='" << out.materials.at(matKey).material.name << "' diffuseKey='"
                                << diffuseKey << "'"
                                << " texCount(diff/base/unknown)=" << m->GetTextureCount(aiTextureType_DIFFUSE) << "/"
                                << m->GetTextureCount(aiTextureType_BASE_COLOR) << "/"
                                << m->GetTextureCount(aiTextureType_UNKNOWN));
            }
        }

        void LoadMeshes(
            std::string_view modelKey,
            const aiScene*   scene,
            const Config&    cfg,
            LoadOutput&      out,
            std::string&     ioError)
        {
            out.meshes.clear();
            out.meshes.reserve(scene->mNumMeshes);

            VertexBuildOptions opt{
                .wantNormals  = true,
                .wantUV0      = true,
                .wantTangents = cfg.calcTangents,
                .wantColors0  = false,
                .wantSkinning = false,
            };

            for (unsigned si = 0; si < scene->mNumMeshes; ++si)
            {
                const aiMesh* m = scene->mMeshes[si];
                if (!m)
                    continue;

                if (cfg.excludeFaceless && (!m->HasFaces() || m->mNumFaces == 0))
                    continue;

                const std::string meshKey = SubKey(modelKey, "mesh", si);

                MeshAsset   meshCpu{};
                std::string err;
                if (!BuildMeshCpuFromAssimp(m, meshCpu, opt, err))
                {
                    ioError = "BuildMeshCpuFromAssimp failed for mesh[" + std::to_string(si) + "]: " + err;
                    return;
                }

                // metadata for caching
                meshCpu.ownerModel = {};
                meshCpu.name       = (m->mName.length > 0) ? m->mName.C_Str() : ("mesh_" + std::to_string(si));

                out.meshes.emplace(meshKey, std::move(meshCpu));
            }
        }

        int32_t AddNodeRecursive(
            const aiScene*    scene,
            const aiNode*     node,
            int32_t           parentIndex,
            LoadedModel&      model,
            std::string_view  modelKey,
            const LoadOutput& loaded,
            const Config&     cfg)
        {
            const int32_t myIdx = static_cast<int32_t>(model.nodes.size());

            ModelNode n{};
            n.name   = node->mName.C_Str();
            n.parent = parentIndex;
            n.local  = ToMatrix(node->mTransformation);

            model.nodes.push_back(std::move(n));

            // Parts: one per node-mesh reference.
            for (unsigned i = 0; i < node->mNumMeshes; ++i)
            {
                const unsigned meshIndex = node->mMeshes[i];
                const aiMesh*  mesh      = scene->mMeshes[meshIndex];
                if (!mesh)
                    continue;

                if (cfg.excludeFaceless && (!mesh->HasFaces() || mesh->mNumFaces == 0))
                    continue;
                LoadedModelPart p{};
                p.name = /*(mesh->mName.length 
> 0) ? mesh->mName.C_Str() : */
                    node->mName.C_Str();
                p.group     = "";
                p.meshKey   = SubKey(modelKey, "mesh", meshIndex);
                p.nodeIndex = myIdx;

                // Material binding: aiMesh has one material index.
                const unsigned    matIndex = mesh->mMaterialIndex;
                const std::string matKey   = SubKey(modelKey, "mat", matIndex);

                // Match 1-1 with mesh submeshes (currently extractor creates exactly 1).
                p.submeshMaterialKeys.clear();
                p.submeshMaterialKeys.push_back(matKey);

                // bounds: pull from the MeshCpu if available
                if (auto it = loaded.meshes.find(p.meshKey); it != loaded.meshes.end())
                    p.bounds = it->second.bounds;

                model.parts.push_back(std::move(p));
            }

            model.nodes[myIdx].children.reserve(node->mNumChildren);
            for (unsigned c = 0; c < node->mNumChildren; ++c)
            {
                const int32_t childIdx =
                    AddNodeRecursive(scene, node->mChildren[c], myIdx, model, modelKey, loaded, cfg);
                model.nodes[myIdx].children.push_back(childIdx);
            }

            return myIdx;
        }
    } // namespace

    LoadResult LoadModelData(const fs::path& modelPath, LoadOutput& out, const Config& cfg)
    {
        out = LoadOutput{}; // reset

        Assimp::Importer importer;

        unsigned flags = 0;

        if (cfg.triangulate)
            flags |= aiProcess_Triangulate;

        flags |= aiProcess_JoinIdenticalVertices;
        flags |= aiProcess_ImproveCacheLocality;
        flags |= aiProcess_SortByPType;
        flags |= aiProcess_FindDegenerates;
        flags |= aiProcess_FindInvalidData;

        if (cfg.genNormalsIfMissing)
            flags |= aiProcess_GenSmoothNormals;

        if (cfg.calcTangents)
            flags |= aiProcess_CalcTangentSpace;

        if (cfg.convertToLeftHanded)
            flags |= aiProcess_ConvertToLeftHanded;

        if (cfg.genBoundingBoxes)
            flags |= aiProcess_GenBoundingBoxes;

        if (cfg.globalScale != 1.0f)
        {
            importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, cfg.globalScale);
            flags |= aiProcess_GlobalScale;
        }

        const aiScene* scene = importer.ReadFile(modelPath.string(), flags);
        if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE))
            return { false, importer.GetErrorString() };

        const std::string modelKey = modelPath.lexically_normal().generic_string();

        out.model.name      = modelPath.stem().string();
        out.model.sourceKey = modelKey;

        // Materials first (so part bindings resolve)
        LoadMaterials(modelKey, modelPath, scene, out);

        // Meshes
        std::string meshErr;
        LoadMeshes(modelKey, scene, cfg, out, meshErr);
        if (!meshErr.empty())
            return { false, meshErr };

        // Nodes + parts
        AddNodeRecursive(scene, scene->mRootNode, -1, out.model, modelKey, out, cfg);

        // Compute overall model bounds from part local bounds transformed by node world.
        if (!out.model.parts.empty())
        {
            std::vector<Matrix>  nodeWorld(out.model.nodes.size(), Matrix::Identity);
            std::vector<uint8_t> nodeComputed(out.model.nodes.size(), 0);

            std::function<Matrix(size_t)> computeNodeWorld = [&](size_t nodeIndex) -> Matrix
            {
                if (nodeComputed[nodeIndex])
                    return nodeWorld[nodeIndex];

                const auto& node = out.model.nodes[nodeIndex];
                Matrix      outM = node.local;

                if (node.parent >= 0 && static_cast<size_t>(node.parent) < out.model.nodes.size())
                    outM = outM * computeNodeWorld(static_cast<size_t>(node.parent));

                nodeWorld[nodeIndex]    = outM;
                nodeComputed[nodeIndex] = 1;
                return outM;
            };

            for (size_t i = 0; i < out.model.nodes.size(); ++i)
                computeNodeWorld(i);

            AABB modelBounds;
            modelBounds.Reset();

            for (const auto& part : out.model.parts)
            {
                if (!part.bounds.has_value())
                    continue;

                const auto&   b          = *part.bounds;
                const Vector3 corners[8] = {
                    { b.min.x, b.min.y, b.min.z }, { b.max.x, b.min.y, b.min.z }, { b.min.x, b.max.y, b.min.z },
                    { b.max.x, b.max.y, b.min.z }, { b.min.x, b.min.y, b.max.z }, { b.max.x, b.min.y, b.max.z },
                    { b.min.x, b.max.y, b.max.z }, { b.max.x, b.max.y, b.max.z },
                };

                Matrix nodeM = Matrix::Identity;
                if (part.nodeIndex >= 0 && static_cast<size_t>(part.nodeIndex) < nodeWorld.size())
                    nodeM = nodeWorld[static_cast<size_t>(part.nodeIndex)];

                for (const auto& c : corners)
                    modelBounds.Expand(Vector3::Transform(c, nodeM));
            }

            if (modelBounds.IsValid())
                out.model.bounds = modelBounds;
        }

        return { true, {} };
    }
} // namespace AssimpLoad

#undef LOG_TAG
