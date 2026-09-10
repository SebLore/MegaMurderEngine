#pragma once

#include "Common/D3D11Headers.h"

#include "Common/ShaderStage.h"

#include <Utility/ErrorHandling.h>
#include "Utils/DebugName.h"

namespace DX
{
    class TextureSRV
    {
      public:
        TextureSRV()  = default;
        ~TextureSRV() = default;
        TextureSRV(TextureSRV&& other) noexcept
            : m_texture(std::move(other.m_texture)), m_srv(std::move(other.m_srv)), m_stage(other.m_stage)
        {
            other.m_stage = NA; // reset stage so other doesn't accidentally bind to it
        }
        TextureSRV& operator=(TextureSRV&& other) noexcept
        {
            if (this != &other)
            {
                m_texture     = std::move(other.m_texture);
                m_srv         = std::move(other.m_srv);
                m_stage       = other.m_stage;
                other.m_stage = NA; // reset stage so other doesn't accidentally bind to it
            }
            return *this;
        };

        TextureSRV(
            ID3D11Device*                          device,
            UINT                                   width,
            UINT                                   height,
            DXGI_FORMAT                            format       = DXGI_FORMAT_R8G8B8A8_UNORM,
            bool                                   generateMips = true,
            const D3D11_SHADER_RESOURCE_VIEW_DESC& srvDesc      = DefaultSRVDesc(),
            D3D11_TEXTURE2D_DESC                   texDesc      = DefaultTexDesc())
        {
            Initialize(device, width, height, format, generateMips, srvDesc, texDesc);
        }
        TextureSRV(
            ID3D11Device*                   device,
            ID3D11Texture2D*                texture,
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = DefaultSRVDesc())
        {
            InitializeFromTexture(device, texture, srvDesc);
        }
        TextureSRV(ComPtr<ID3D11ShaderResourceView> srv) { InitializeFromSRV(std::move(srv)); }

        void Initialize(
            ID3D11Device*                          device,
            UINT                                   width,
            UINT                                   height,
            DXGI_FORMAT                            format       = DXGI_FORMAT_R8G8B8A8_UNORM,
            bool                                   generateMips = true,
            const D3D11_SHADER_RESOURCE_VIEW_DESC& srvDesc      = DefaultSRVDesc(),
            D3D11_TEXTURE2D_DESC                   texDesc      = DefaultTexDesc())
        {
            THROWIA_IF(!device, "Device is nullptr");

            texDesc.Width     = width;
            texDesc.Height    = height;
            texDesc.Format    = format;
            texDesc.MipLevels = generateMips ? 0 : 1; // 0 to generate full chain
            texDesc.MiscFlags = generateMips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

            HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, m_texture.GetAddressOf());
            if (FAILED(hr))
                THROWRE("Failed to create texture for shader resource view");

            // create the shader resource view
            hr = device->CreateShaderResourceView(m_texture.Get(), &srvDesc, m_srv.GetAddressOf());
            if (FAILED(hr))
                THROWRE("Failed to create shader resource view");
        }

        void InitializeFromTexture(
            ID3D11Device*                   device,
            ID3D11Texture2D*                texture,
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = DefaultSRVDesc())
        {
            THROWIA_IF(!device, "Device is nullptr");
            if (texture == NULL)
                throw std::invalid_argument("Texture is nullptr");

            // get the texture description
            D3D11_TEXTURE2D_DESC texDesc;
            texture->GetDesc(&texDesc);

            // make srv desc match texture desc
            srvDesc.Format                    = texDesc.Format;
            srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels       = texDesc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;

            // create the shader resource view
            HRESULT hr = device->CreateShaderResourceView(texture, &srvDesc, m_srv.GetAddressOf());
            if (FAILED(hr))
                THROWRE("Failed to create shader resource view");

            // reset local texture so they're not at odds
            m_texture.Reset();
        }

        void InitializeFromSRV(ComPtr<ID3D11ShaderResourceView> srv)
        {
            THROWIA_IF(!srv, "ShaderResourceView is nullptr");

            m_srv = std::move(srv); // just store reference to the srv
            m_texture.Reset();      // reset local texture so they're not at odds
        }

        void Bind(ID3D11DeviceContext* context, SHADER_STAGE stage = NA, UINT slot = 0) const
        {
            THROWIA_IF(!context, "Context is nullptr");
            THROWIA_IF(!m_srv, "ShaderResourceView is nullptr");

            stage = (stage == NA) ? m_stage : stage;

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
            case NA:
                // fallthrough
            default:
                break;
            }
        }

        void UnBind(ID3D11DeviceContext* context, SHADER_STAGE stage = NA, UINT slot = 0) const
        {
            THROWIA_IF(!context, "Context is nullptr");

            static ID3D11ShaderResourceView* nullSRV = nullptr; // static so only it doesn't get recreated every call

            if (stage == NA)
                stage = m_stage;

            // unbind from selected stage
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
            case NA:
                // fallthrough
            default:
                break;
            }
        }

        void SetShaderStage(SHADER_STAGE stage) { m_stage = stage; }

        void SetDebugName(const std::string& name)
        {
            Debug::SetDebugName(m_texture, (name + ".texture2D").c_str());
            Debug::SetDebugName(m_srv, (name + ".srv").c_str());
        }

        void Reset()
        {
            m_texture.Reset();
            m_srv.Reset();
            m_stage = PS;
        }

        ID3D11Texture2D*           GetTexture() const { return m_texture.Get(); }
        ID3D11ShaderResourceView*  GetSRV() const { return m_srv.Get(); }
        ID3D11ShaderResourceView** GetSRVAddress() { return m_srv.GetAddressOf(); }
        SHADER_STAGE               GetShaderStage() const { return m_stage; }

        // default descriptions
        /// @brief Generates a default texture2D description
        static D3D11_TEXTURE2D_DESC DefaultTexDesc()
        {
            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width                = 0;
            desc.Height               = 0;
            desc.MipLevels            = 1;
            desc.ArraySize            = 1;
            desc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count     = 1;
            desc.Usage                = D3D11_USAGE_DEFAULT;
            desc.BindFlags            = D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags       = 0;
            desc.MiscFlags            = 0;
            return desc;
        }

        static D3D11_SHADER_RESOURCE_VIEW_DESC DefaultSRVDesc()
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.Format                          = DXGI_FORMAT_UNKNOWN; // gets patched to texture format
            desc.ViewDimension                   = D3D11_SRV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MostDetailedMip       = 0;
            desc.Texture2D.MipLevels             = 1;
            return desc;
        }

        // no copying
        TextureSRV(const TextureSRV&)            = delete;
        TextureSRV& operator=(const TextureSRV&) = delete;

      private:
        ComPtr<ID3D11Texture2D>          m_texture;
        ComPtr<ID3D11ShaderResourceView> m_srv;
        SHADER_STAGE                     m_stage = PS; // default stage to bind to
    };
} // namespace DX
