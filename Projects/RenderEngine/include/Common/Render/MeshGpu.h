#pragma once

#include <cstdint>
#include <vector>

#include "Common/D3D11Headers.h"

#include <Common/Assets/VertexLayout.h>

namespace Murder
{
    struct SubMeshGpu
    {
        uint32_t indexStart = 0;
        uint32_t indexCount = 0;
        int32_t  baseVertex = 0;
    };

    /// new gpu mesh stores data, does not bind or draw
    struct MeshGpu
    {
        ComPtr<ID3D11Buffer> vertexBuffer;
        ComPtr<ID3D11Buffer> indexBuffer;

        DXGI_FORMAT              indexFormat = DXGI_FORMAT_R32_UINT;
        D3D11_PRIMITIVE_TOPOLOGY topology    = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

        uint32_t vertexStride = 0;
        uint32_t vertexCount  = 0;
        uint32_t indexCount   = 0;

        VertexLayout            layout;
        std::vector<SubMeshGpu> submeshes;

        bool Valid() const noexcept { return vertexBuffer && vertexStride && vertexCount > 0; }
        bool HasIndices() const noexcept { return indexBuffer && indexCount > 0; }
    };
} // namespace Murder
