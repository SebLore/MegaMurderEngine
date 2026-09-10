#include "RenderPrep.h"

#include <AssetManager.h>
#include <cmath>

#include "Common/Assets/ModelAssetUtils.h"

#include <Rework/RenderBackend.h>

namespace Murder::Render
{
    namespace
    {
        /// Build material for the gpu from a cpu material asset
        MaterialGpuData BuildMaterialGpuData(const MaterialAsset* material)
        {
            MaterialGpuData out{};

            if (!material)
                return out;

            out.ambient   = material->properties.ambient;
            out.shininess = material->properties.shininess;
            out.diffuse   = material->properties.diffuse;
            out.alpha     = material->properties.dissolve;
            out.specular  = material->properties.specular;
            out.reflect   = material->properties.reflect;

            return out;
        }

        /// Calculate attentuation based on max range over distance (fallback/default, don't keep it like this)
        [[nodiscard]] Vector3 CalculateAttenuation(float maxRange)
        {
            const float invRange2 = maxRange > 0.001f ? 1.0f / (maxRange * maxRange) : 1.0f;
            return Vector3{ 1.0f, 0.0f, invRange2 };
        }

        /// Build transform data for the gpu from a world matrix, including normal matrix build from the inverse
        [[nodiscard]] TransformGpuData BuildTransformGpuData(const Matrix& world)
        {
            using namespace DirectX;

            TransformGpuData out{};

            const Matrix normal = world.Invert().Transpose();

            XMStoreFloat4x4(&out.world, world);
            XMStoreFloat4x4(&out.normal, normal);

            return out;
        }

        /// Resolve the material for a given model instance and part, accounting for any overrides on the instance. we want to resolve the material before the texture so that we can know which textures to look for overrides on
        [[nodiscard]] MaterialId
        ResolveMaterialId(const ModelInstance& instance, const ModelPart& part, size_t partIndex, uint32_t submeshIndex)
        {
            MaterialId material{};

            if (submeshIndex < part.submeshMaterials.size())
                material = part.submeshMaterials[submeshIndex];

            if (instance.materialOverrides)
            {
                for (const auto& overrideMat : *instance.materialOverrides)
                {
                    if (overrideMat.partIndex != partIndex)
                        continue;
                    if (overrideMat.submeshIndex != submeshIndex)
                        continue;
                    if (!overrideMat.material)
                        continue;

                    material = overrideMat.material;
                    break;
                }
            }

            return material;
        }

        /// @brief Resolves the diffuse texture for a model instance, considering material and texture overrides.
        /// @param instance The model instance for which to resolve the diffuse texture.
        /// @param material Pointer to the material asset, used as the base source for the diffuse texture.
        /// @param partIndex The index of the model part to match for texture overrides.
        /// @param submeshIndex The index of the submesh to match for texture overrides.
        /// @return The resolved diffuse texture ID, taking into account material and any applicable texture overrides.
        [[nodiscard]] TextureId ResolveDiffuseTexture(
            const ModelInstance& instance,
            const MaterialAsset* material,
            size_t               partIndex,
            uint32_t             submeshIndex)
        {
            TextureId diffuse{};

            // if we have a material we want to use it
            if (material)
                diffuse = material->textures[ToIndex(TextureSlot::Diffuse)];

            // account for texture overrides
            if (instance.textureOverrides)
            {
                for (const auto& overrideTex : *instance.textureOverrides)
                {
                    if (overrideTex.partIndex != partIndex)
                        continue;
                    if (overrideTex.submeshIndex != submeshIndex)
                        continue;
                    if (overrideTex.slot != TextureSlot::Diffuse)
                        continue;
                    if (!overrideTex.texture)
                        continue;

                    diffuse = overrideTex.texture;
                    break;
                }
            }

            return diffuse;
        }

        [[nodiscard]] Vector3 EstimateShadowTarget(const RenderWorld& world)
        {
            if (world.models.empty())
                return world.camera.position;

            Vector3 sum{ 0.0f, 0.0f, 0.0f };
            size_t  count = 0;

            for (const auto& instance : world.models)
            {
                sum += Vector3{ instance.world._41, instance.world._42, instance.world._43 };
                ++count;
            }

            return count > 0 ? (sum / static_cast<float>(count)) : world.camera.position;
        }
        [[nodiscard]] Vector3 SafeUp(const Vector3& forward)
        {
            Vector3 f = forward;
            if (f.LengthSquared() <= EPS)
                return Vector3{ 0.0f, 1.0f, 0.0f };

            f.Normalize();

            if (std::abs(f.Dot(Vector3{ 0.0f, 1.0f, 0.0f })) > 0.99f)
                return Vector3{ 1.0f, 0.0f, 0.0f };

            return Vector3{ 0.0f, 1.0f, 0.0f };
        }

        [[nodiscard]] CameraGpuData BuildDirLightShadowCamera(const DirectionalLight& light, const RenderWorld& world)
        {
            CameraGpuData out{};

            Vector3 dir = light.direction;
            if (dir.LengthSquared() <= EPS)
                dir = Vector3{ 0.0f, -1.0f, 0.0f };
            dir.Normalize();

            const Vector3 target   = EstimateShadowTarget(world);
            const Vector3 position = target - dir * 20.0f;
            const Vector3 up       = SafeUp(dir);
            const Matrix  view     = CameraMath::View(position, dir, up);
            const Matrix  proj     = CameraMath::Projection(light.shadow.projection);
            const Matrix  vp       = view * proj;

            DirectX::XMStoreFloat4x4(&out.view, view);
            DirectX::XMStoreFloat4x4(&out.proj, proj);
            DirectX::XMStoreFloat4x4(&out.viewProj, vp);
            out.eyePosition = position;

            return out;
        }

        [[nodiscard]] CameraGpuData BuildSpotLightShadowCamera(const SpotLight& light)
        {
            CameraGpuData out{};

            Vector3 dir = light.direction;
            if (dir.LengthSquared() <= EPS)
                dir = Vector3{ 0.0f, -1.0f, 0.0f };
            dir.Normalize();

            const Vector3 up   = SafeUp(dir);
            const Matrix  view = CameraMath::View(light.position, dir, up);
            const Matrix  proj = CameraMath::Projection(light.shadow.projection);
            const Matrix  vp   = view * proj;

            DirectX::XMStoreFloat4x4(&out.view, view);
            DirectX::XMStoreFloat4x4(&out.proj, proj);
            DirectX::XMStoreFloat4x4(&out.viewProj, vp);
            out.eyePosition = light.position;

            return out;
        }
    } // namespace

    void ResolveSceneResources(const RenderWorld& world, AssetManager& assets, RenderBackend& backend)
    {
        for (const auto& instance : world.models)
        {
            if (!assets.LoadModel(instance.id))
                continue;

            const ModelAsset* model = assets.TryGetModel(instance.id);
            if (!model)
                continue;

            for (size_t partIndex = 0; partIndex < model->parts.size(); ++partIndex)
            {
                const ModelPart& part = model->parts[partIndex];

                if (!backend.FindMesh(part.meshId))
                {
                    const MeshAsset* mesh = assets.TryGetMesh(part.meshId);
                    if (mesh)
                        backend.GetOrCreateMesh(part.meshId, *mesh);
                }

                const uint32_t submeshCount =
                    std::max<uint32_t>(1u, static_cast<uint32_t>(part.submeshMaterials.size()));

                for (uint32_t submeshIndex = 0; submeshIndex < submeshCount; ++submeshIndex)
                {
                    const MaterialId materialId = ResolveMaterialId(instance, part, partIndex, submeshIndex);

                    const MaterialAsset* material = materialId ? assets.TryGetMaterial(materialId) : nullptr;

                    const TextureId diffuse = ResolveDiffuseTexture(instance, material, partIndex, submeshIndex);

                    if (!diffuse || backend.FindTexture(diffuse))
                        continue;

                    TextureUploadDesc upload{};
                    if (assets.TryGetTextureUpload(diffuse, upload))
                        backend.GetOrCreateTexture(diffuse, upload);
                }
            }
        }

        for (auto* sprite : world.sprites)
        {
            if (!sprite)
                continue;

            if (!assets.LoadTexture(sprite->textureId))
                continue;

            TextureUploadDesc upload{};
            if (assets.TryGetTextureUpload(sprite->textureId, upload))
                backend.GetOrCreateTexture(sprite->textureId, upload);
        }
    }

    void BuildCameraGpu(const Camera& camera, DrawList& out) { out.camera = CameraMath::ToGpuData(camera); }

    void BuildLightsGpu(const RenderWorld& world, DrawList& out)
    {
        out.dirLights.clear();
        out.pointLights.clear();
        out.spotLights.clear();

        const size_t dirCount =
            std::min(world.directionalLights.size(), static_cast<size_t>(MAX_DIRECTIONAL_LIGHTS_GPU));
        const size_t pointCount = std::min(world.pointLights.size(), static_cast<size_t>(MAX_POINT_LIGHTS_GPU));
        const size_t spotCount  = std::min(world.spotLights.size(), static_cast<size_t>(MAX_SPOT_LIGHTS_GPU));

        out.dirLights.reserve(dirCount);
        out.pointLights.reserve(pointCount);
        out.spotLights.reserve(spotCount);

        for (size_t i = 0; i < dirCount; ++i)
        {
            const auto& src = world.directionalLights[i];

            DirectionalLightGpuData gpu{};
            gpu.direction = src.direction;
            gpu.intensity = src.intensity;
            gpu.color     = src.color;

            out.dirLights.push_back(gpu);
        }

        for (size_t i = 0; i < pointCount; ++i)
        {
            const auto& src = world.pointLights[i];

            PointLightGpuData gpu{};
            gpu.position    = src.position;
            gpu.range       = src.maxRange;
            gpu.color       = src.color;
            gpu.intensity   = src.intensity;
            gpu.attenuation = CalculateAttenuation(src.maxRange);

            out.pointLights.push_back(gpu);
        }

        for (size_t i = 0; i < spotCount; ++i)
        {
            const auto& src = world.spotLights[i];

            SpotLightGpuData gpu{};
            gpu.position    = src.position;
            gpu.range       = src.maxRange;
            gpu.direction   = src.direction;
            gpu.intensity   = src.intensity;
            gpu.color       = src.color;
            gpu.innerCos    = cosf(src.innerAngleRadians);
            gpu.outerCos    = cosf(src.outerAngleRadians);
            gpu.attenuation = CalculateAttenuation(src.maxRange);

            out.spotLights.push_back(gpu);
        }

        out.lightCollection.pointCount       = static_cast<uint32_t>(out.pointLights.size());
        out.lightCollection.directionalCount = static_cast<uint32_t>(out.dirLights.size());
        out.lightCollection.spotCount        = static_cast<uint32_t>(out.spotLights.size());
        out.lightCollection.ambient          = world.ambientLight;
    }

    void BuildShadows(const RenderWorld& world, DrawList& out, uint32_t maxShadowCount)
    {
        out.shadows.clear();

        uint32_t slice = 0;

        for (uint32_t i = 0; i < world.directionalLights.size() && slice < maxShadowCount; ++i)
        {
            const auto& light = world.directionalLights[i];
            if (light.shadow.enabled)
            {
                out.shadows.push_back(
                    DrawList::Shadow{ .kind        = DrawList::Shadow::Kind::Directional,
                                      .lightIndex  = i,
                                      .shadowSlice = slice++,
                                      .camera      = BuildDirLightShadowCamera(light, world),
                                      .depthBias   = light.shadow.depthBias,
                                      .normalBias  = light.shadow.normalBias,
                                      .strength    = 1.0f });
            }
        }

        for (uint32_t i = 0; i < world.spotLights.size() && slice < maxShadowCount; ++i)
        {
            const auto& light = world.spotLights[i];
            if (!light.shadow.enabled)
                continue;

            out.shadows.push_back(
                DrawList::Shadow{ .kind        = DrawList::Shadow::Kind::Spot,
                                  .lightIndex  = i,
                                  .shadowSlice = slice++,
                                  .camera      = BuildSpotLightShadowCamera(light),
                                  .depthBias   = light.shadow.depthBias,
                                  .normalBias  = light.shadow.normalBias,
                                  .strength    = 1.0f });
        }
    }

    void AppendPartDrawItems(
        const ModelInstance& instance,
        size_t               partIndex,
        const ModelPart&     part,
        const Matrix&        partWorld,
        AssetManager&        assets,
        DrawList&            out)
    {
        const uint32_t submeshCount = std::max<uint32_t>(1, static_cast<uint32_t>(part.submeshMaterials.size()));

        for (uint32_t i = 0; i < submeshCount; ++i)
        {
            const MaterialId materialId = ResolveMaterialId(instance, part, partIndex, i);

            const MaterialAsset* material = materialId ? assets.TryGetMaterial(materialId) : nullptr;

            DrawList::Item item{};
            item.meshId        = part.meshId;
            item.submeshIndex  = i;
            item.transform     = BuildTransformGpuData(partWorld);
            item.materialData  = BuildMaterialGpuData(material);
            item.textureId     = ResolveDiffuseTexture(instance, material, partIndex, i);
            item.transparent   = material ? (material->properties.dissolve < 0.999f) : false;
            item.viewModel     = (instance.kind == ModelKind::ViewModel);
            item.dynamic       = true;
            item.materialIndex = static_cast<int>(materialId.value);

            out.items.push_back(std::move(item));
        }
    }

    void BuildDrawList(
        const RenderWorld&   world,
        AssetManager&        assets,
        const RenderBackend& backend,
        DrawList&            out,
        uint32_t             maxShadowCount)
    {
        out.Clear();

        BuildCameraGpu(world.camera, out);
        BuildLightsGpu(world, out);
        BuildShadows(world, out, maxShadowCount);

        std::vector<Matrix> nodeWorlds;
        for (const auto& instance : world.models)
        {
            const ModelAsset* model = assets.TryGetModel(instance.id);
            if (!model)
                continue;

            Assets::BuildNodeWorld(*model, instance.world, nodeWorlds);

            for (size_t partIndex = 0; partIndex < model->parts.size(); ++partIndex)
            {
                const ModelPart& part = model->parts[partIndex];

                if (HasFlag(part.flags, ModelPartFlags::Hidden) || HasFlag(part.flags, ModelPartFlags::CollisionOnly))
                    continue;

                if (!backend.FindMesh(part.meshId))
                    continue;

                Matrix partWorld = instance.world;
                size_t index     = static_cast<size_t>(part.nodeIndex);
                if (part.nodeIndex >= 0 && index < nodeWorlds.size())
                    partWorld = nodeWorlds[index];

                AppendPartDrawItems(instance, partIndex, part, partWorld, assets, out);
            }
        }

        // Sprites
        for (auto* sprite : world.sprites)
        {
            if (!sprite || !sprite->visible)
                continue;

            std::shared_ptr<const DX::TextureSRV> tex = backend.GetDefaultTexture();

            if (sprite->textureId)
            {
                if (const auto* cached = backend.FindTexture(sprite->textureId))
                    tex = *cached;
            }

            if (!tex)
                continue;

            RenderSprite rs{};
            rs.texture   = tex->GetSRV();
            rs.transform = sprite->transform;
            rs.origin    = sprite->origin;
            rs.srcRect   = sprite->srcRect;
            rs.dstRect   = sprite->dstRect;
            rs.anchor    = sprite->anchor;
            rs.tint      = sprite->tint;
            rs.layer     = sprite->layer;
            rs.visible   = sprite->visible;

            out.sprites.push_back(std::move(rs));
        }

        for (auto* textSprite : world.spriteTexts)
        {
            if (!textSprite || !textSprite->Visible)
                continue;
            out.textSprites.push_back(textSprite);
        }
    }
} // namespace Murder::Render
