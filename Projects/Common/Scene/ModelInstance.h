#pragma once

#include <cstdint>
#include <vector>

#include <Core/AssetTags.h>
#include <Core/Math/Types.h>

#include <Common/Assets/ModelAsset.h>
#include <Common/Scene/ModelComponent.h>

namespace Murder
{
    enum class ModelKind : uint8_t
    {
        World,
        ViewModel
    };
    /**
     * @brief Derived runtime render model snapshot used by render-world building.
     *
     * Responsibilities compared to nearby types:
     * - `ModelAsset`: canonical cached asset data (most complete, import/cached model info)
     * - `ModelComponent`: ECS intent (asset id, transform, sparse overrides)
     * - `ModelInstance`: resolved per-instance runtime expansion (nodeWorld + part bindings)
     */
    struct ModelInstance
    {
        ModelId   id{};
        Matrix    world{};
        ModelKind kind = ModelKind::World;

        const std::vector<MaterialOverride>* materialOverrides = nullptr;
        const std::vector<TextureOverride>*  textureOverrides  = nullptr;
    };
} // namespace Murder
