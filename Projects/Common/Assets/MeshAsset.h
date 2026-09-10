#pragma once

#include <optional>
#include <string>
#include <vector>

#include "VertexLayout.h"

#include <Core/AssetTags.h>
#include <Core/MurderCore.hpp>
#include <memory>

namespace Murder
{
    // data struct for defining a submesh of a mesh.
    struct SubMeshCpu
    {
        uint32_t indexStart = 0;
        uint32_t indexCount = 0;
        int  baseVertex = 0; ///< fallback for non-indexed calls

        std::string         name;   // optional name for the submesh
        std::optional<AABB> bounds; // local bounds for this specific submesh
    };

    // geometry data stored for collision
    struct MeshGeometryData
    {
        std::vector<Vector3> positions;
        std::vector<index_t> indices;
    };

    // data struct for defining a mesh only on the cpu, no buffers
    struct MeshAsset
    {
        // Meta-data for loading and debugging, not needed for rendering
        ModelId     ownerModel{}; ///< entry into asset manager mesh cache
        std::string name;         ///< optional name of self

        // Per-mesh data
        VertexLayout layout; // used to generate an input layout and when uploading mesh data

        std::vector<std::byte>     vertexData; ///< raw vertex data
        std::vector<std::uint32_t> indices;
        std::vector<SubMeshCpu>    submeshes;

        // collision
        std::vector<Vector3> positions; ///< vertex positions, stored for collision
        std::vector<std::uint32_t> invIndices;

        // optional so it can be calculated later
        std::optional<AABB>               bounds; // local bounds for the whole mesh, including all submeshes
        std::unique_ptr<MeshGeometryData> geometryData;
    };
} // namespace Murder
