#pragma once

#include <d3d11.h>

#include <cstdint>

namespace DX
{
    enum SHADER_STAGE : uint8_t
    {
        VS = 1 << 0,
        PS = 1 << 1,
        GS = 1 << 2,
        HS = 1 << 3,
        DS = 1 << 4,
        CS = 1 << 5,
        NA = 0
    };

    //constexpr inline SHADER_STAGE operator|(SHADER_STAGE a, SHADER_STAGE b)
    //{
    //	return static_cast<SHADER_STAGE>(static_cast<int>(a) | static_cast<int>(b));
    //   }

    //constexpr inline SHADER_STAGE operator&(SHADER_STAGE a, SHADER_STAGE b)
    //{
    //	return static_cast<SHADER_STAGE>(static_cast<int>(a) & static_cast<int>(b));
    //   }

    //constexpr inline SHADER_STAGE& operator|=(SHADER_STAGE& a, SHADER_STAGE b)
    //{
    //	a = a | b;
    //	return a;
    //   }

    //constexpr inline SHADER_STAGE& operator&=(SHADER_STAGE& a, SHADER_STAGE b)
    //{
    //	a = a & b;
    //	return a;
    //   }

    static void BindShaderSRV(
        ID3D11DeviceContext*             context,
        UINT                             slot,
        DX::SHADER_STAGE                 stage,
        ID3D11ShaderResourceView* const* srv,
        UINT                             count = 1)
    {
        if (stage & SHADER_STAGE::VS)
            context->VSSetShaderResources(slot, count, srv);
        if (stage & SHADER_STAGE::PS)
            context->PSSetShaderResources(slot, count, srv);
        if (stage & SHADER_STAGE::GS)
            context->GSSetShaderResources(slot, count, srv);
        if (stage & SHADER_STAGE::CS)
            context->CSSetShaderResources(slot, count, srv);
        // HS and DS stages do not support SRVs
    }

    static void BindConstantBuffer(
        ID3D11DeviceContext* context,
        UINT                 slot,
        SHADER_STAGE         stage,
        ID3D11Buffer* const* buffer,
        UINT                 count = 1)
    {
        if (stage & SHADER_STAGE::VS)
            context->VSSetConstantBuffers(slot, count, buffer);
        if (stage & SHADER_STAGE::PS)
            context->PSSetConstantBuffers(slot, count, buffer);
        if (stage & SHADER_STAGE::GS)
            context->GSSetConstantBuffers(slot, count, buffer);
        if (stage & SHADER_STAGE::HS)
            context->HSSetConstantBuffers(slot, count, buffer);
        if (stage & SHADER_STAGE::DS)
            context->DSSetConstantBuffers(slot, count, buffer);
        if (stage & SHADER_STAGE::CS)
            context->CSSetConstantBuffers(slot, count, buffer);
    }

    static void UnBindShaderSRV(ID3D11DeviceContext* context, UINT slot, SHADER_STAGE stage, UINT count = 1)
    {
        ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
        if (stage & SHADER_STAGE::VS)
            context->VSSetShaderResources(slot, count, nullSRV);
        if (stage & SHADER_STAGE::PS)
            context->PSSetShaderResources(slot, count, nullSRV);
        if (stage & SHADER_STAGE::GS)
            context->GSSetShaderResources(slot, count, nullSRV);
        if (stage & SHADER_STAGE::CS)
            context->CSSetShaderResources(slot, count, nullSRV);
        // HS and DS stages do not support SRVs
    }

    static void UnBindConstantBuffer(ID3D11DeviceContext* context, UINT slot, SHADER_STAGE stage, UINT count = 1)
    {
        ID3D11Buffer* nullBuffer[1] = { nullptr };
        if (stage & SHADER_STAGE::VS)
            context->VSSetConstantBuffers(slot, count, nullBuffer);
        if (stage & SHADER_STAGE::PS)
            context->PSSetConstantBuffers(slot, count, nullBuffer);
        if (stage & SHADER_STAGE::GS)
            context->GSSetConstantBuffers(slot, count, nullBuffer);
        if (stage & SHADER_STAGE::HS)
            context->HSSetConstantBuffers(slot, count, nullBuffer);
        if (stage & SHADER_STAGE::DS)
            context->DSSetConstantBuffers(slot, count, nullBuffer);
        if (stage & SHADER_STAGE::CS)
            context->CSSetConstantBuffers(slot, count, nullBuffer);
    }
} // namespace DX
