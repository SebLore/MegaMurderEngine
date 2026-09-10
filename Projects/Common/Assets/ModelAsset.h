#pragma once

#include <optional>
#include <string>
#include <vector>

#include <Core/Math/Types.h> // AABB and matrices
#include <Core/AssetTags.h>  // Ids

namespace Murder
{
    struct AABB;

    // -- Flags --
    enum class ModelPartFlags : uint32_t
    {
        None          = 0,       // default
        Hidden        = 1u << 0, // for when we don't want to draw the part
        CollisionOnly = 1u << 1, // when a part is only meant for collision and not geometry
        NoShadow      = 1u << 2, // should be skipped in shadow mapping
        // add more as needed
    };

    // operators
    inline ModelPartFlags operator|(ModelPartFlags a, ModelPartFlags b)
    {
        return static_cast<ModelPartFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }
    inline ModelPartFlags& operator|=(ModelPartFlags& a, ModelPartFlags b)
    {
        a = a | b;
        return a;
    }
    inline bool HasFlag(ModelPartFlags flags, ModelPartFlags flag)
    {
        return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
    }

    /// matches node in the scene graph of the model file
    struct ModelNode
    {
        std::string name; // optional name for the node, useful for animation and debugging

        int              parent = -1; // index of parent node, -1 if root
        std::vector<int> children;    // indices of child nodes

        Matrix local = Matrix::Identity; // local transform relative to parent, from model file
    };

    // part of the model, i.e. "arm", head, etc.
    struct ModelPart
    {
        std::string name;  // optional name for the part, useful for debugging
        std::string group; // optional group name for grouping parts together (e.g., "body", "wheels", "colliders")

        MeshId meshId{}; // sub-asset

        std::vector<MaterialId> submeshMaterials; // match 1-1 with mesh submeshes

        int nodeIndex = -1; // index of the node this part is attached to, -1 if not attached

        std::optional<AABB> bounds; // local bounds for this part, optional since it can be calculated later

        ModelPartFlags flags = ModelPartFlags::None; // flags for rendering and behavior
    };

    // composite of all models in the blender file
    struct ModelAsset
    {
        std::string name;      // optional name for the model, useful for debugging
        std::string sourceKey; // normalized key from AssetRegistry, useful for debugging and reverse lookups

        std::vector<ModelNode> nodes; // scene graph nodes, indexed by nodeIndex in ModelPart
        std::vector<ModelPart> parts; // model parts, each references a mesh and materials, and is attached to a node

        std::optional<AABB> bounds; // local bounds for the whole model, optional since it can be calculated later
    };
} // namespace Murder
