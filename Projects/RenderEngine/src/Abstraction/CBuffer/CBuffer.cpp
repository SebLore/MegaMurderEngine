#include "pch.h"

#include "Abstraction/CBuffer/CBuffer.h"

#include <Utility/ErrorHandling.h>
#include <Utility/Logging.h>

#define LOG_TAG "CBuffer"

namespace DX
{

    ConstantBuffer::ConstantBuffer(ConstantBuffer&& other) noexcept
        : m_Buffer(std::move(other.m_Buffer)),
          m_PipelineStage(other.m_PipelineStage)
    {
        other.m_PipelineStage = SHADER_STAGE::NA;
    }

    ConstantBuffer& ConstantBuffer::operator=(ConstantBuffer&& other) noexcept
    {
        if (this != &other)
        {
            m_Buffer              = std::move(other.m_Buffer);
            m_PipelineStage       = other.m_PipelineStage;
            other.m_PipelineStage = SHADER_STAGE::NA;
        }
        return *this;
    }

    ConstantBuffer::ConstantBuffer(
        ID3D11Device* device,
        SHADER_STAGE  pipelineStage,
        const void*   bytePtr,
        UINT          byteWidth,
        bool          dynamic)
        : ConstantBuffer(
              device,
              pipelineStage,
              bytePtr,
              byteWidth,
              dynamic,
              DefaultDesc(dynamic, byteWidth))
    {
    }

    ConstantBuffer::ConstantBuffer(
        ID3D11Device*            device,
        SHADER_STAGE             stage,
        bool                     dynamic,
        const D3D11_BUFFER_DESC& desc)
    {
        Initialize(device, stage, dynamic, desc);
    }

    ConstantBuffer::ConstantBuffer(
        ID3D11Device*            device,
        SHADER_STAGE             pipelineStage,
        const void*              bytePtr,
        UINT                     byteWidth,
        bool                     dynamic,
        const D3D11_BUFFER_DESC& desc)
    {
        Initialize(device, pipelineStage, bytePtr, byteWidth, dynamic, desc);
    }

    void ConstantBuffer::Initialize(
        ID3D11Device* device,
        SHADER_STAGE  stage,
        bool,
        const D3D11_BUFFER_DESC& desc)
    {
        HRESULT hr =
            device->CreateBuffer(&desc, nullptr, m_Buffer.GetAddressOf());

        m_PipelineStage = stage;
    }

    void ConstantBuffer::Initialize(
        ID3D11Device* device,
        SHADER_STAGE  stage,
        const void*   bytePtr,
        UINT,
        bool,
        const D3D11_BUFFER_DESC& desc)
    {
        D3D11_SUBRESOURCE_DATA initData = { .pSysMem          = bytePtr,
                                            .SysMemPitch      = 0,
                                            .SysMemSlicePitch = 0 };

        HRESULT hr = device->CreateBuffer(
            &desc,
            (bytePtr) ? &initData : nullptr,
            m_Buffer.GetAddressOf());

        THROWRE_IF(
            FAILED(hr),
            "Failed to create constant buffer. HRESULT:" << hr);

        m_PipelineStage = stage;
    }

    bool ConstantBuffer::IsInitialized() const { return m_Buffer != nullptr; }

    void ConstantBuffer::Reset()
    {
        m_Buffer.Reset();
        m_PipelineStage = SHADER_STAGE::NA;
    }

    void ConstantBuffer::Bind(
        ID3D11DeviceContext* context,
        SHADER_STAGE         stage,
        UINT                 startSlot) const
    {
        THROWIA_IF(!context, "Context was nullptr");
        if (!IsInitialized())
        {
            LOG_WARN_N("Buffer uninitialized", 5);
            return;
        }

        if (stage == SHADER_STAGE::NA)
            stage = m_PipelineStage;

        switch (stage)
        {
        case SHADER_STAGE::VS:
            context->VSSetConstantBuffers(
                startSlot,
                1,
                m_Buffer.GetAddressOf());
            break;
        case SHADER_STAGE::PS:
            context->PSSetConstantBuffers(
                startSlot,
                1,
                m_Buffer.GetAddressOf());
            break;
        case SHADER_STAGE::GS:
            context->GSSetConstantBuffers(
                startSlot,
                1,
                m_Buffer.GetAddressOf());
            break;
        case SHADER_STAGE::HS:
            context->HSSetConstantBuffers(
                startSlot,
                1,
                m_Buffer.GetAddressOf());
            break;
        case SHADER_STAGE::DS:
            context->DSSetConstantBuffers(
                startSlot,
                1,
                m_Buffer.GetAddressOf());
            break;
        case SHADER_STAGE::CS:
            context->CSSetConstantBuffers(
                startSlot,
                1,
                m_Buffer.GetAddressOf());
            break;
        default:
            break;
        }
    }

    void DX::ConstantBuffer::UnBind(
        ID3D11DeviceContext* context,
        SHADER_STAGE         stage,
        UINT                 startSlot) const
    {
        if (stage == SHADER_STAGE::NA)
            stage = m_PipelineStage;
        static ID3D11Buffer* nullBuffer = nullptr;
        switch (stage)
        {
        case SHADER_STAGE::VS:
            context->VSSetConstantBuffers(startSlot, 1, &nullBuffer);
            break;
        case SHADER_STAGE::PS:
            context->PSSetConstantBuffers(startSlot, 1, &nullBuffer);
            break;
        case SHADER_STAGE::GS:
            context->GSSetConstantBuffers(startSlot, 1, &nullBuffer);
            break;
        case SHADER_STAGE::HS:
            context->HSSetConstantBuffers(startSlot, 1, &nullBuffer);
            break;
        case SHADER_STAGE::DS:
            context->DSSetConstantBuffers(startSlot, 1, &nullBuffer);
            break;
        case SHADER_STAGE::CS:
            context->CSSetConstantBuffers(startSlot, 1, &nullBuffer);
            break;
        default:
            break;
        }
    }

    ID3D11Buffer* ConstantBuffer::GetBuffer() const { return m_Buffer.Get(); };

    void ConstantBuffer::Update(
        ID3D11DeviceContext* context,
        const void*          data,
        UINT                 size)
    {
        if (!data)
            return;

        D3D11_MAPPED_SUBRESOURCE mappedResource = {};
        HRESULT                  hr             = context->Map(
            m_Buffer.Get(),
            0,
            D3D11_MAP_WRITE_DISCARD,
            0,
            &mappedResource);

        memcpy(mappedResource.pData, data, size);
        context->Unmap(m_Buffer.Get(), 0);
    }

    // set the name in the graphics debugger
    void ConstantBuffer::SetDebugObjectName(const char* name)
    {
        if (name)
            m_Buffer->SetPrivateData(
                WKPDID_D3DDebugObjectName,
                (UINT)strlen(name),
                name);
    }

    void ConstantBuffer::SetDebugObjectNameW(const wchar_t* name)
    {
        if (name)
            m_Buffer->SetPrivateData(
                WKPDID_D3DDebugObjectName,
                (UINT)(wcslen(name) * sizeof(wchar_t)),
                name);
    }

} // namespace DX

#undef LOG_TAG
