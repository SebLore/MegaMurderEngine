#pragma once

#include <memory>
#include <stdexcept>

#include "Common/D3D11Headers.h"
#include "Common/RegisterConstants.h"
#include "Common/ShaderStage.h"
#include "Utils/DebugName.h"

using Microsoft::WRL::ComPtr;

namespace DX
{
    /// GBuffer class for stuff like deferred rendering
    class GBuffer
    {
      public:
        GBuffer()                     = default;
        ~GBuffer()                    = default;
        GBuffer(GBuffer&&)            = default;
        GBuffer& operator=(GBuffer&&) = default;

        /// Initializes the GBuffer buffer with width and height. Dimensions should match back buffer for deferred rendering.
        void Initialize(ID3D11Device* device, UINT width, UINT height)
        {
            D3D11_TEXTURE2D_DESC textureDesc = {};
            textureDesc.Width                = width;
            textureDesc.Height               = height;
            textureDesc.MipLevels            = 1;
            textureDesc.ArraySize            = 1;
            textureDesc.Format               = DXGI_FORMAT_R32G32B32A32_FLOAT;
            textureDesc.SampleDesc.Count     = 1;
            textureDesc.Usage                = D3D11_USAGE_DEFAULT;
            textureDesc.BindFlags =
                D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

            HRESULT hr = device->CreateTexture2D(
                &textureDesc,
                nullptr,
                m_texture.GetAddressOf());

            if (FAILED(hr))
                throw std::runtime_error("Failed to create GBuffer texture");

            // bind srv to the texture
            hr = device->CreateShaderResourceView(
                m_texture.Get(),
                nullptr,
                m_srv.GetAddressOf());
            if (FAILED(hr))
                throw std::runtime_error(
                    "Failed to create GBuffer shader resource view");

            // bind rtv to the texture
            hr = device->CreateRenderTargetView(
                m_texture.Get(),
                nullptr,
                m_rtv.GetAddressOf());
            if (FAILED(hr))
                throw std::runtime_error(
                    "Failed to create GBuffer render look view");

            m_Width  = width;
            m_Height = height;
        }

        /// Get raw pointer to buffer render target view to
        ID3D11RenderTargetView* GetRTV() const { return m_rtv.Get(); }

        /// Get raw pointer to buffer shader resource view
        ID3D11ShaderResourceView* GetSRV() const { return m_srv.Get(); }

        /// Get width of the underlying buffer
        UINT GetWidth() const { return m_Width; }
        /// Get height of the underlying buffer
        UINT GetHeight() const { return m_Height; }

        void SetDebugName(const std::string& name)
        {
            Debug::SetDebugName(m_texture, (name + ".texture2D").c_str());
            Debug::SetDebugName(m_srv, (name + ".srv").c_str());
            Debug::SetDebugName(m_rtv, (name + ".rtv").c_str());
        }

        // no copying
        /// don't copy only move
        GBuffer(const GBuffer&)            = delete;
        /// don't copy only move
        GBuffer& operator=(const GBuffer&) = delete;

      private:
        ComPtr<ID3D11Texture2D>          m_texture; ///< buffer object
        ComPtr<ID3D11ShaderResourceView> m_srv; ///< srv of underlying buffer
        ComPtr<ID3D11RenderTargetView>   m_rtv; ///< rtv of underlying buffer

        UINT m_Width  = 1; ///< width of the buffer
        UINT m_Height = 1; ///< height of the buffer
    };

    /// Collection of gbuffers for deferred rendering
    class GBufferCollection
    {
      public:
        GBufferCollection()  = default;
        ~GBufferCollection() = default;

        /// Initializes all the underlying gbuffers to the same size
        void Initialize(ID3D11Device* device, UINT width, UINT height)
        {
            for (UINT i = 0; i < m_bufferCount; i++)
                m_gbuffers[i].Initialize(device, width, height);
        }

        /// Binds all the gbuffers as an array of render targets, optionally with depth
        void BindAsRTV(
            ID3D11DeviceContext*    context,
            ID3D11DepthStencilView* dsv = nullptr) const
        {
            if (!context)
                throw std::runtime_error("Context is nullptr");

            // bind the render targets
            ID3D11RenderTargetView* RTV[m_bufferCount] = { nullptr };
            for (UINT i = 0; i < m_bufferCount; i++)
                RTV[i] = m_gbuffers[i].GetRTV();
            context->OMSetRenderTargets(m_bufferCount, RTV, dsv);
        }

        /// Unbinds gbuffers from the render target slot
        void UnBindRTV(ID3D11DeviceContext* context) const
        {
            if (!context)
                throw std::runtime_error("Context is nullptr");

            ID3D11RenderTargetView* rts[m_bufferCount] = { nullptr };
            context->OMSetRenderTargets(m_bufferCount, rts, nullptr);
        }

        /// Binds gbuffers to the chosen shader stage, default compute shader, as SRV starting from slot.
        void BindAsSRV(
            ID3D11DeviceContext* context,
            SHADER_STAGE         stage = CS,
            UINT                 slot  = R_GBUFFER0) const
        {
            if (!context)
                throw std::runtime_error("Context is nullptr");

            ID3D11ShaderResourceView* srv[m_bufferCount] = { nullptr };
            for (UINT i = 0; i < m_bufferCount; i++)
                srv[i] = m_gbuffers[i].GetSRV();

            switch (stage)
            {
            case SHADER_STAGE::CS:
                context->CSSetShaderResources(slot, m_bufferCount, srv);
                break;
            case SHADER_STAGE::PS:
                context->PSSetShaderResources(slot, m_bufferCount, srv);
                break;
            default:
                throw std::runtime_error(
                    "Unsupported shader stage for GBuffer binding");
            }
        }

        /// Unbinds the gbuffers as SRV from the chosen shader stage, starting at the selected slot
        void UnBindSRV(
            ID3D11DeviceContext* context,
            SHADER_STAGE         stage = CS,
            UINT                 slot  = R_GBUFFER0) const
        {
            if (!context)
                throw std::runtime_error("Context is nullptr");

            ID3D11ShaderResourceView* srv[m_bufferCount] = { nullptr };

            if (stage & PS)
                context->PSSetShaderResources(slot, m_bufferCount, srv);
            if (stage & CS)
                context->CSSetShaderResources(slot, m_bufferCount, srv);
        }

        /// Clear RTV of all gbuffers in collection
        void Clear(ID3D11DeviceContext* context, const float color[4]) const
        {
            if (!context)
                throw std::runtime_error("Context is nullptr");
            for (UINT i = 0; i < m_bufferCount; i++)
                context->ClearRenderTargetView(m_gbuffers[i].GetRTV(), color);
        }

        /// Get reference to GBuffer at specific slot
        const GBuffer& GetGBuffer(UINT index) const
        {
            if (index >= m_bufferCount)
                throw std::out_of_range("GBuffer index out of range");
            return m_gbuffers[index];
        }

        void SetDebugName(const std::string& name)
        {
            for (UINT i = 0; i < m_bufferCount; i++)
                m_gbuffers[i].SetDebugName(name + "[" + std::to_string(i) + "]");
        }

        // no copying
        /// no copy only move
        GBufferCollection(const GBufferCollection&)            = delete;
        /// no copy only move
        GBufferCollection& operator=(const GBufferCollection&) = delete;

      private:
        static constexpr UINT m_bufferCount = 4;         ///< constant  value for the array, could maybe be dynamic
        GBuffer               m_gbuffers[m_bufferCount]; ///< array of GBuffer objects
    };

} // namespace DX
