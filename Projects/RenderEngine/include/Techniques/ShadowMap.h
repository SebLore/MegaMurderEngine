#pragma once

#include <memory>
#include <vector>

#include "Abstraction/States.h"

#include "Common/RegisterConstants.h"
#include "Common/ShaderStage.h"

#include "Utils/DebugName.h"

namespace DX
{
    class ShadowMap
    {
      public:
        ShadowMap()  = default;
        ~ShadowMap() = default;
        ShadowMap(ID3D11Device* device, UINT shadowCount, UINT width, UINT height);

        void Initialize(ID3D11Device* device, UINT shadowCount, UINT width, UINT height = 0);

        void Clear(ID3D11DeviceContext* context, UINT index = 0) const;
        void SetViewport(ID3D11DeviceContext* context) const;
        void SetAsDSV(ID3D11DeviceContext* context, UINT index = 0) const;
        void BindSRV(ID3D11DeviceContext* context, SHADER_STAGE stage = PS, UINT slot = R_SHADOW_MAPS) const;
        void UnBindSRV(ID3D11DeviceContext* context, SHADER_STAGE stage = PS, UINT slot = R_SHADOW_MAPS) const;
        void UnSetDSV(ID3D11DeviceContext* context) const;

        void
        BindSamplerState(ID3D11DeviceContext* context, SHADER_STAGE stage = PS, UINT slot = R_SHADOW_SAMPLER) const;
        void
        UnBindSamplerState(ID3D11DeviceContext* context, SHADER_STAGE stage = PS, UINT slot = R_SHADOW_SAMPLER) const;

        void BindRasterizerState(ID3D11DeviceContext* context) const;
        void UnBindRasterizerState(ID3D11DeviceContext* context) const;

        ID3D11DepthStencilView* GetDSV(UINT index = 0) const;

        UINT           GetShadowCount() const;
        D3D11_VIEWPORT GetShadowViewport() const;

      public:
        static constexpr D3D11_SAMPLER_DESC GetSamplerStateDesc()
        {
            return D3D11_SAMPLER_DESC{
                .Filter         = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, //D3D11_FILTER_MIN_MAG_MIP_LINEAR,
                .AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP,
                .AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP,
                .AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP,
                .MipLODBias     = 0.0f,
                .MaxAnisotropy  = 1,
                .ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL,
                .BorderColor    = { 1.0f, 1.0f, 1.0f, 1.0f },
                .MinLOD         = 0.0f,
                .MaxLOD         = D3D11_FLOAT32_MAX
            };
        }
        static constexpr D3D11_RASTERIZER_DESC GetRasterizerStateDesc()
        {
            D3D11_RASTERIZER_DESC rd = {};
            rd.FillMode              = D3D11_FILL_SOLID;
            rd.CullMode              = D3D11_CULL_BACK;
            rd.DepthClipEnable       = TRUE;

            rd.DepthBias            = 100;
            rd.SlopeScaledDepthBias = 1.0f;
            rd.DepthBiasClamp       = 0.01f;

            return rd;
        }

        // no copying
        ShadowMap(const ShadowMap&)            = delete;
        ShadowMap& operator=(const ShadowMap&) = delete;

#ifdef _DEBUG
        void SetDebugName(const std::string& name)
        {
            Debug::SetDebugName(m_texture, (name + ".texture2D").c_str());
            Debug::SetDebugName(m_srv, (name + ".srv").c_str());
            m_samplerState.SetDebugName(name + ".samplerState");
            m_RasterizerState.SetDebugName(name + ".rasterizerState");
            for (size_t i = 0; i < m_DSVs.size(); ++i)
                Debug::SetDebugName(m_DSVs[i], Debug::IndexedName((name + ".dsv").c_str(), i).c_str());
        }
#endif

      private:
        ComPtr<ID3D11Texture2D>                     m_texture;
        ComPtr<ID3D11ShaderResourceView>            m_srv;
        SamplerState                                m_samplerState;
        RasterizerState                             m_RasterizerState;
        std::vector<ComPtr<ID3D11DepthStencilView>> m_DSVs;
        D3D11_VIEWPORT                              m_viewport = {};
    };

} // namespace DX
