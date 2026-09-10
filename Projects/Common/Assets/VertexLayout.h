#pragma once

#include <cstdint>
#include <vector>

namespace Murder
{
    /// defines common vertex semantics matching d3d11 (or others) for dynamic vertex buffer creation
    enum class VertexSemantic : uint8_t
    {
        POSITION,
        NORMAL,
        UV,
        TEXCOORD,  // because texcoord is a common 'default' semantic
        TANGENT,   // normal mapping
        BITANGENT, // normal mapping
        COLOR,
        JOINTS, // animate
        WEIGHTS // animate
        // add more as needed
    };

    /// Defines common vertex formats that match d3d11(or other)
    enum class VertexFormat : uint8_t
    {
        // clang-format off
        FLOAT1, FLOAT2, FLOAT3, FLOAT4,
        UINT1, UINT2, UINT3, UINT4,
        // clang-format on

        UBYTE4,      // raw 0..255 (good for JOINTS)
        USHORT4,     //
        UBYTE4NORM,  // normalized 0..1 (good for COLOR/WEIGHTS)
        BYTE4NORM,   // normalized -1..1 (good for NORMAL/TANGENT if you go byte)
        SHORT4NORM,  // normalized -1..1 (better NORMAL/TANGENT)
        USHORT4NORM, // normalized 0..1 (better WEIGHTS)
        // add more as needed
    };

    /**
     * @brief A single element of the VertexLayout struct
     * @details Defines an element in the VertexLayout struct's elements vector. Each element is made to match the
     *
     */
    struct VertexElement
    {
        VertexSemantic semantic{};
        uint8_t        semanticIndex{}; // for multiple semantics of the same type, e.g. TEXCOORD0, TEXCOORD1
        VertexFormat   format;
        uint8_t        inputSlot{}; // for interleaved vs separate buffers
        uint16_t       offset{};    // byte offset in the vertex structure
    };

    struct VertexLayout
    {
        std::vector<VertexElement> elements;
        uint16_t                   stride = 0; // total byte size of one vertex
    };
} // namespace Murder
