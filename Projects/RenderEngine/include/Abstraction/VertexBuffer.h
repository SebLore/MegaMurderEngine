#pragma once

#include "Common/D3D11Headers.h"

#include <Utility/ErrorHandling.h>
#include "Utils/DebugName.h"

using Microsoft::WRL::ComPtr;

namespace DX
{
    /// templated wrapped for a vertex buffer
    template <typename T> class VertexBuffer
    {
      public:
        VertexBuffer()                                   = default;
        ~VertexBuffer()                                  = default;
        VertexBuffer(VertexBuffer&&) noexcept            = default;
        VertexBuffer& operator=(VertexBuffer&&) noexcept = default;

        /**
         * @brief Initializes the buffer with some data
         * @param device valid device
         * @param data pointer to vertex data
         * @param vertexCount number of vertices
         * @param desc description of underlying buffer object
         * @throws device is nullptr
         */
        VertexBuffer(
            ID3D11Device*     device,
            const void*       data,
            UINT              vertexCount,
            D3D11_BUFFER_DESC desc = DefaultDesc())
        {
            Initialize(device, data, vertexCount, desc);
        }

        /**
         * @brief Initializes the buffer with some data
         * @param device valid device
         * @param data pointer to vertex data
         * @param vertexCount number of vertices
         * @param desc description of underlying buffer object
         * @throws device is nullptr
         */
        void Initialize(
            ID3D11Device*     device,
            const void*       data,
            UINT              vertexCount,
            D3D11_BUFFER_DESC desc = DefaultDesc())
        {
            THROWIA_IF(!device, "device is nullptr");

            desc.ByteWidth = m_vertexSize * vertexCount;

            D3D11_SUBRESOURCE_DATA initData = {};
            initData.pSysMem                = data;
            initData.SysMemPitch            = 0;
            initData.SysMemSlicePitch       = 0;

            HRESULT hr =
                device->CreateBuffer(&desc, &initData, m_buffer.GetAddressOf());
            if (FAILED(hr))
                THROWRE("Failed to create vertex buffer");

            // store data after successful creation
            m_vertexCount = vertexCount;
        }

        /// Resets the buffer, ready for new data.
        void Reset()
        {
            m_buffer.Reset();
            m_vertexCount = 0;
            m_offset      = 0;
        }

        /**
         * @brief Bind buffer to the pipeline
         * @param context valid device context
         * @param slot start slot in the input assembler
         * @throw context is nullptr
         * @throw m_buffer is not initialized
         */
        void Bind(ID3D11DeviceContext* context, UINT slot = 0) const
        {
            THROWIA_IF(!context, "context is nullptr");
            THROWIA_IF(!m_buffer, "buffer is nullptr");

            context->IASetVertexBuffers(
                slot,
                1,
                m_buffer.GetAddressOf(),
                &m_vertexSize,
                &m_offset);
        }

        /// UnBind vertex buffer from the pipeline
        void UnBind(ID3D11DeviceContext* context, UINT slot = 0) const
        {
            THROWIA_IF(!context, "context is nullptr");
            ID3D11Buffer* nullbuffer = nullptr;

            context->IASetVertexBuffers(
                slot,
                1,
                &nullbuffer,
                &m_vertexSize,
                &m_offset);
        }

        /**
         * @brief Update the buffer with new data
         * @param context valid device conext
         * @param data pointer to the data
         * @param byteWidth size of the data
         * @throw context is nullptr
         * @throw m_buffer underlying buffer is not initialized
         */
        void Update(
            ID3D11DeviceContext* context,
            const void*          data,
            UINT                 byteWidth = m_vertexSize) const
        {
            THROWIA_IF(!context, "context is nullptr");
            THROWIA_IF(!m_buffer, "vertex buffer is nullptr");

            // map buffer
            D3D11_MAPPED_SUBRESOURCE mappedResource = {};

            HRESULT hr = context->Map(
                m_buffer.Get(),
                0,
                D3D11_MAP_WRITE_DISCARD,
                0,
                &mappedResource);

            // throw if failed
            if (FAILED(hr))
                THROWRE("Failed to map vertex buffer");

            // copy data to gpu
            memcpy(mappedResource.pData, data, byteWidth);
            context->Unmap(m_buffer.Get(), 0);
        }

        /// set new vertex offset
        void SetOffset(UINT newOffset) { m_offset = newOffset; }

        /// get current vertex offset
        constexpr UINT GetOffset() const { return m_offset; }

        /// Get number of vertices being managed
        constexpr UINT GetVertexCount() const { return m_vertexCount; }

        /// byte width of a single vertex
        constexpr UINT GetVertexSize() const { return m_vertexSize; }

        /// Get a default description for the underlying buffer
        static D3D11_BUFFER_DESC DefaultDesc()
        {
            D3D11_BUFFER_DESC desc   = {};
            desc.Usage               = D3D11_USAGE_DEFAULT;
            desc.BindFlags           = D3D11_BIND_VERTEX_BUFFER;
            desc.CPUAccessFlags      = 0;
            desc.MiscFlags           = 0;
            desc.StructureByteStride = 0;
            desc.ByteWidth           = sizeof(T);
            return desc;
        }

        void SetDebugName(const std::string& name)
        {
            Debug::SetDebugName(m_buffer, name.c_str());
        }

        // no copying
        /// no copy only move
        VertexBuffer(const VertexBuffer&)            = delete;
        /// no copy only move
        VertexBuffer& operator=(const VertexBuffer&) = delete;

      private:
        ComPtr<ID3D11Buffer>  m_buffer; ///< managed buffer
        static constexpr UINT m_vertexSize   = sizeof(T); ///< byte size of a single vertex
        UINT                  m_vertexCount = 0; ///< number of vertices
        UINT                  m_offset       = 0; ///< offset in the buffer to the start of the first vertex, useful for vertex buffers with multiple meshes
    };

} // namespace DX
