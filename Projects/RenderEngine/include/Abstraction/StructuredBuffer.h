#pragma once

#include "Common/D3D11Headers.h"
#include "Common/RegisterConstants.h"
#include "Common/ShaderStage.h"
#include <Utility/ErrorHandling.h>
#include "Utils/DebugName.h"

namespace DX
{
    class StructuredBuffer
    {
      public:
        StructuredBuffer()  = default;
        ~StructuredBuffer() = default;

        StructuredBuffer(StructuredBuffer&& other) noexcept
            : m_buffer(std::move(other.m_buffer)), m_srv(std::move(other.m_srv)), m_uav(std::move(other.m_uav)),
              m_elementCount(other.m_elementCount), m_elementSize(other.m_elementSize)
        {
        }
        StructuredBuffer& operator=(StructuredBuffer&& other) noexcept
        {
            if (this != &other)
            {
                m_buffer       = std::move(other.m_buffer);
                m_srv          = std::move(other.m_srv);
                m_uav          = std::move(other.m_uav);
                m_elementCount = other.m_elementCount;
                m_elementSize  = other.m_elementSize;
            }
            return *this;
        }

        /// @brief Initializes the structured buffer
        /// @param device valid device pointer
        /// @param elementSize byte size of one element, e.g. 16 bytes for a float4
        /// @param elementCount Number of elements to be stored
        /// @param srv Flag for shader reading (Shader resource view)
        /// @param uav Flag for unordered access view (compute shader). Mutually exclusive with
        /// @param dynamic Flag for mapping/unmapping and CPU_WRITE access flag
        /// @param initData Pointer to initial data. Needs to be sizeof(elementSize * elementCount)
        void Initialize(
            ID3D11Device* device,
            UINT          elementSize,
            UINT          elementCount,
            bool          srv,
            bool          uav,
            bool          dynamic  = true,
            const void*   initData = nullptr)
        {
            THROWIA_IF(!device, "Device is nullptr");

            // Buffer description
            // If buffer is not dynamic (CPU write) or allows UAV access (GPU write/read) then it is
            // immutable.
            D3D11_BUFFER_DESC bufferDesc{};
            bufferDesc.ByteWidth           = elementSize * elementCount;
            bufferDesc.StructureByteStride = elementSize;
            bufferDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
            bufferDesc.BindFlags           = srv ? D3D11_BIND_SHADER_RESOURCE : 0;
            bufferDesc.Usage               = D3D11_USAGE_IMMUTABLE;
            bufferDesc.CPUAccessFlags      = 0;
            if (uav)
            {
                bufferDesc.Usage      = D3D11_USAGE_DEFAULT;
                bufferDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
            }
            else if (dynamic)
            {
                bufferDesc.Usage          = D3D11_USAGE_DYNAMIC;
                bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            }

            D3D11_SUBRESOURCE_DATA data{};
            data.pSysMem     = initData;
            data.SysMemPitch = data.SysMemSlicePitch = 0;
            HRESULT hr                               = S_OK;

            hr = initData ? device->CreateBuffer(&bufferDesc, &data, m_buffer.GetAddressOf())
                          : device->CreateBuffer(&bufferDesc, nullptr, m_buffer.GetAddressOf());

            if (FAILED(hr))
                THROWRE("Failed to create buffer for StructuredBuffer");

            // If enabled, create the SRV
            if (srv)
            {
                const D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{ .Format        = DXGI_FORMAT_UNKNOWN,
                                                               .ViewDimension = D3D11_SRV_DIMENSION_BUFFER,
                                                               .Buffer        = { .FirstElement = 0,
                                                                                  .NumElements  = elementCount } };
                hr = device->CreateShaderResourceView(m_buffer.Get(), &srvDesc, m_srv.GetAddressOf());

                if (FAILED(hr))
                    THROWRE("Failed to create SRV for StructuredBuffer");
            }

            // If enabled, create the UAV
            if (uav)
            {
                const D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{
                    .Format        = DXGI_FORMAT_UNKNOWN,
                    .ViewDimension = D3D11_UAV_DIMENSION_BUFFER,
                    .Buffer        = { .FirstElement = 0, .NumElements = elementCount, .Flags = 0 }
                };

                hr = device->CreateUnorderedAccessView(m_buffer.Get(), &uavDesc, m_uav.GetAddressOf());

                if (FAILED(hr))
                    THROWRE("Failed to create UAV for StructuredBuffer");
            }

            m_elementCount = elementCount;
            m_elementSize  = elementSize;
        }

        void BindAsSRV(ID3D11DeviceContext* context, SHADER_STAGE stage = VS, UINT slot = 0) const
        {
            THROWIA_IF(!context, "Context is nullptr");

            switch (stage)
            {
            case VS:
                context->VSSetShaderResources(slot, 1, m_srv.GetAddressOf());
                break;
            case PS:
                context->PSSetShaderResources(slot, 1, m_srv.GetAddressOf());
                break;
            case GS:
                context->GSSetShaderResources(slot, 1, m_srv.GetAddressOf());
                break;
            case HS:
                context->HSSetShaderResources(slot, 1, m_srv.GetAddressOf());
                break;
            case DS:
                context->DSSetShaderResources(slot, 1, m_srv.GetAddressOf());
                break;
            case CS:
                context->CSSetShaderResources(slot, 1, m_srv.GetAddressOf());
                break;
            default:
                THROWRE("Invalid shader stage");
                break;
            }
        }

        void BindUAV(ID3D11DeviceContext* context, UINT slot = 0) const
        {
            THROWIA_IF(!context, "Context is nullptr");

            context->CSSetUnorderedAccessViews(slot, 1, m_uav.GetAddressOf(), nullptr);
        }

        void UnBindSRV(ID3D11DeviceContext* context, SHADER_STAGE stage = VS, UINT slot = 0) const
        {
            THROWIA_IF(!context, "Context is nullptr");

            ID3D11ShaderResourceView* nullSRV = nullptr;
            switch (stage)
            {
            case VS:
                context->VSSetShaderResources(slot, 1, &nullSRV);
                break;
            case PS:
                context->PSSetShaderResources(slot, 1, &nullSRV);
                break;
            case GS:
                context->GSSetShaderResources(slot, 1, &nullSRV);
                break;
            case HS:
                context->HSSetShaderResources(slot, 1, &nullSRV);
                break;
            case DS:
                context->DSSetShaderResources(slot, 1, &nullSRV);
                break;
            case CS:
                context->CSSetShaderResources(slot, 1, &nullSRV);
                break;
            default:
                THROWRE("Invalid shader stage");
                break;
            }
        }

        void UnBindUAV(ID3D11DeviceContext* context, UINT slot = 0) const
        {
            THROWIA_IF(!context, "Context is nullptr");
            static ID3D11UnorderedAccessView* nullUAV = nullptr;
            context->CSSetUnorderedAccessViews(slot, 1, &nullUAV, nullptr);
        }

        void UnBind(ID3D11DeviceContext* context) const
        {
            THROWIA_IF(!context, "Context is nullptr");

            static ID3D11ShaderResourceView* nullSRV = nullptr;
            context->PSSetShaderResources(0, 1, &nullSRV);

            static ID3D11UnorderedAccessView* nullUAV = nullptr;
            context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
        }

        void UpdateData(ID3D11DeviceContext* context, const void* data, UINT size = 0) const
        {
            THROWIA_IF(!context, "Context is nullptr");
            THROWIA_IF(!m_buffer, "structured buffer is nullptr");

            D3D11_MAPPED_SUBRESOURCE mappedResource{};
            HRESULT                  hr = context->Map(m_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

            if (FAILED(hr))
                THROWRE("Failed to map buffer");

            memcpy(
                mappedResource.pData,
                data,
                size ? static_cast<size_t>(size) : static_cast<size_t>(m_elementSize) * m_elementCount);
            context->Unmap(m_buffer.Get(), 0);
        }

        UINT GetElementCount() const { return m_elementCount; }

        UINT GetElementSize() const { return m_elementSize; }

        ID3D11ShaderResourceView* GetSRV() const { return m_srv.Get(); }

        ID3D11UnorderedAccessView* GetUAV() const { return m_uav.Get(); }

        // default description
        constexpr static D3D11_BUFFER_DESC defaultDesc()
        {
            D3D11_BUFFER_DESC desc{};
            desc.Usage               = D3D11_USAGE_DEFAULT;
            desc.BindFlags           = 0;
            desc.CPUAccessFlags      = 0;
            desc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
            desc.StructureByteStride = sizeof(float);
            desc.ByteWidth           = sizeof(float) * 1024;
            desc.CPUAccessFlags      = 0;

            return desc;
        }

        // no copying
        StructuredBuffer(const StructuredBuffer&)            = delete;
        StructuredBuffer& operator=(const StructuredBuffer&) = delete;

        void SetDebugName(const std::string& name)
        {
            Debug::SetDebugName(m_buffer, (name + ".buffer").c_str());
            Debug::SetDebugName(m_srv, (name + ".srv").c_str());
            Debug::SetDebugName(m_uav, (name + ".uav").c_str());
        }

      private:
        ComPtr<ID3D11Buffer>              m_buffer;
        ComPtr<ID3D11ShaderResourceView>  m_srv;
        ComPtr<ID3D11UnorderedAccessView> m_uav;

        UINT m_elementCount = 0;
        UINT m_elementSize  = 0;
    };
} // namespace DX
