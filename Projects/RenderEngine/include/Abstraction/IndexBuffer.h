#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include "Utility/ErrorHandling.h"
#include "Utils/DebugName.h"

using Microsoft::WRL::ComPtr;

namespace DX
{
    class IndexBuffer
    {
      public:
        IndexBuffer()                                  = default;
        ~IndexBuffer()                                 = default;
        IndexBuffer(IndexBuffer&&) noexcept            = default;
        IndexBuffer& operator=(IndexBuffer&&) noexcept = default;
        IndexBuffer(
            ID3D11Device*     device,
            const void*       data,
            UINT              byteWidth,
            UINT              nrOfIndices,
            D3D11_BUFFER_DESC desc = DefaultDesc())
        {
            Initialize(device, data, byteWidth, nrOfIndices);
        }
        void Initialize(
            ID3D11Device*     device,
            const void*       data,
            UINT              indexSize,
            UINT              nrOfIndices,
            D3D11_BUFFER_DESC desc = DefaultDesc())
        {
            THROWIA_IF(!device, "device is nullptr");
            desc.ByteWidth = indexSize * nrOfIndices;

            D3D11_SUBRESOURCE_DATA initData = {};
            initData.pSysMem                = data;
            initData.SysMemPitch            = 0;
            initData.SysMemSlicePitch       = 0;

            HRESULT hr = device->CreateBuffer(
                &desc,
                &initData,
                m_indexBuffer.GetAddressOf());
            if (FAILED(hr))
                THROWRE("Failed to create index buffer");

            m_indexSize   = indexSize;
            m_nrOfIndices = nrOfIndices;
        }

        void Reset()
        {
            m_indexBuffer.Reset();
            m_indexSize   = 0;
            m_nrOfIndices = 0;
        }

        void Bind(ID3D11DeviceContext* m_context) const
        {
            THROWIA_IF(!m_context, "context is nullptr");
            THROWIA_IF(!m_indexBuffer, "index buffer is nullptr");
            m_context->IASetIndexBuffer(
                m_indexBuffer.Get(),
                DXGI_FORMAT_R32_UINT,
                0);
        }

        void UnBind(ID3D11DeviceContext* m_context) const
        {
            THROWIA_IF(!m_context, "m_context is nullptr");
            m_context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        }

        void SetIndexSize(UINT size) { m_indexSize = size; }

        UINT GetIndexSize() const { return m_indexSize; }

        UINT GetIndexCount() const { return m_nrOfIndices; }

        static D3D11_BUFFER_DESC DefaultDesc()
        {
            D3D11_BUFFER_DESC bufferDesc   = {};
            bufferDesc.Usage               = D3D11_USAGE_DEFAULT;
            bufferDesc.BindFlags           = D3D11_BIND_INDEX_BUFFER;
            bufferDesc.CPUAccessFlags      = 0;
            bufferDesc.MiscFlags           = 0;
            bufferDesc.StructureByteStride = 0;
            return bufferDesc;
        }

        void SetDebugName(const std::string& name)
        {
            Debug::SetDebugName(m_indexBuffer, name.c_str());
        }

      private:
        ComPtr<ID3D11Buffer> m_indexBuffer;
        UINT                 m_indexSize   = 0;
        UINT                 m_nrOfIndices = 0;
        UINT                 m_offset      = 0;
    };

} // namespace DX
