#pragma once

#include <vector>

#include <Common/Assets/ModelAsset.h>

#include <Common/Scene/ModelInstance.h>
#include <Common/Scene/RenderWorld.h>

#include <Common/Render/CameraGpu.h>
#include <Common/Render/MaterialGpu.h>
#include <Common/Render/TransformGpu.h>
#include <Common/Render/MeshGpu.h>
#include <Common/Render/LightCollection.h>
#include <Common/Render/DrawList.h>

namespace Murder
{
    class AssetManager;
}

namespace Murder::Render
{
    class RenderBackend;

    void ResolveSceneResources(const RenderWorld& world, AssetManager& assets, RenderBackend& backend);

    void BuildDrawList(
        const RenderWorld&   world,
        AssetManager&        assets,
        const RenderBackend& backend,
        DrawList&            out,
        uint32_t             maxShadowCount = 4);

    void BuildCameraGpu(const Camera& camera, DrawList& out);
    void BuildLightsGpu(const RenderWorld& world, DrawList& out);
    void BuildShadows(const RenderWorld& world, DrawList& out, uint32_t maxShadowCount = 4);

    void AppendPartDrawItems(
        const ModelInstance& instance,
        size_t               partIndex,
        const ModelPart&     part,
        const Matrix&        partWorld,
        AssetManager&        assets,
        DrawList&            out);
} // namespace Murder::Render
