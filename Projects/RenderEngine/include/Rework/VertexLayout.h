#pragma once
#include <vector>
#include <d3d11.h>
#include <stdexcept>
#include <Common/Assets/VertexLayout.h>

namespace Murder::DxInput
{
    // https://learn.microsoft.com/en-us/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format
    inline DXGI_FORMAT ToDxgiFormat(VertexFormat f) noexcept
    {
        switch (f)
        {
        case VertexFormat::FLOAT1: return DXGI_FORMAT_R32_FLOAT;
        case VertexFormat::FLOAT2: return DXGI_FORMAT_R32G32_FLOAT;
        case VertexFormat::FLOAT3: return DXGI_FORMAT_R32G32B32_FLOAT;
        case VertexFormat::FLOAT4: return DXGI_FORMAT_R32G32B32A32_FLOAT;

        case VertexFormat::UINT1:  return DXGI_FORMAT_R32_UINT;
        case VertexFormat::UINT2:  return DXGI_FORMAT_R32G32_UINT;
        case VertexFormat::UINT3:  return DXGI_FORMAT_R32G32B32_UINT;
        case VertexFormat::UINT4:  return DXGI_FORMAT_R32G32B32A32_UINT;

        default: return DXGI_FORMAT_UNKNOWN;
        }
    }

    // https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-semantics
    inline const char* SemanticStringName(VertexSemantic s) noexcept
    {
        switch (s)
        {
        case VertexSemantic::POSITION:  return "POSITION";
        case VertexSemantic::NORMAL:    return "NORMAL";
        case VertexSemantic::TEXCOORD:  return "TEXCOORD";
        case VertexSemantic::TANGENT:   return "TANGENT";
        case VertexSemantic::BITANGENT: return "BINORMAL";
        case VertexSemantic::COLOR:     return "COLOR";
        case VertexSemantic::JOINTS:    return "BLENDINDICES";
        case VertexSemantic::WEIGHTS:   return "BLENDWEIGHT";
        default: return "TEXCOORD";
        }
    }

    // Build D3D11_INPUT_ELEMENT_DESC array from VertexLayout and pass to device->CreateInputLayout
    inline std::vector<D3D11_INPUT_ELEMENT_DESC> Build(const VertexLayout& layout)
    {
        std::vector<D3D11_INPUT_ELEMENT_DESC> descs;
        descs.reserve(layout.elements.size());

        for (const auto& e : layout.elements)
        {
            D3D11_INPUT_ELEMENT_DESC d{};
            d.SemanticName = SemanticStringName(e.semantic); // string literal
            d.SemanticIndex = e.semanticIndex;
            d.Format = ToDxgiFormat(e.format);
            d.InputSlot = e.inputSlot;
            d.AlignedByteOffset = e.offset;                 // maybe D3D11_APPEND_ALIGNED_ELEMENT
            d.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            d.InstanceDataStepRate = 0;

            // in case something wasn't mapped correctly, throw over using incorrect format
            if (d.Format == DXGI_FORMAT_UNKNOWN)
                throw std::runtime_error("Unknown VertexFormat in input layout mapping");

            descs.push_back(d);
        }

        return descs;
    }
}