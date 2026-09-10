#pragma once

#include "Common/D3D11Headers.h"
#include "Common/ShaderStage.h"

using Microsoft::WRL::ComPtr;

namespace DX
{
    /**
     * @brief Return a D3D11_BUFFER_DESC that can be used to generate a ID3D11Buffer as a constant buffer.
     * @param dynamic If true, flags D3D11_USAGE_DYNAMIC and D3D11_CPU_ACCESS_WRITE are set.
     * @param byteWidth size of the data being bound to the GPU.
     * @note A buffer should be dynamic if it's expected to update often (every frame). If it updates rarely it should be default.
     */
    static constexpr D3D11_BUFFER_DESC DefaultCbufferDesc(bool dynamic = true, UINT byteWidth = 0)
    {
        return {
            .ByteWidth           = byteWidth,
            .Usage               = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT, // for per-frame update
            .BindFlags           = D3D11_BIND_CONSTANT_BUFFER,
            .CPUAccessFlags      = dynamic ? D3D11_CPU_ACCESS_WRITE : static_cast<UINT>(0),
            .MiscFlags           = 0,
            .StructureByteStride = 0,
        };
    }

    /// pure virtual interface for a constant buffer abstraction class.
    class ICBuffer
    {
      public:
        virtual ~ICBuffer()                                                                  = default;
        virtual void Initialize(ID3D11Device*, SHADER_STAGE, bool, const D3D11_BUFFER_DESC&) = 0;
        virtual void Initialize(ID3D11Device*, SHADER_STAGE, const void*, UINT, bool, const D3D11_BUFFER_DESC&) = 0;

        virtual bool IsInitialized() const = 0;
        virtual void Reset()               = 0;

        virtual void Update(ID3D11DeviceContext*, const void*, UINT)        = 0;
        virtual void Bind(ID3D11DeviceContext*, SHADER_STAGE, UINT) const   = 0;
        virtual void UnBind(ID3D11DeviceContext*, SHADER_STAGE, UINT) const = 0;

        virtual SHADER_STAGE GetPipelineStage() const noexcept = 0;
        virtual void         SetPipelineStage(SHADER_STAGE)    = 0;

        virtual void SetDebugObjectName(const char* name)     = 0;
        virtual void SetDebugObjectNameW(const wchar_t* name) = 0;
    };
} // namespace DX
