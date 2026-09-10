#pragma once

#include "Common/D3D11Headers.h"

#include "Utility/ErrorHandling.h"
#include "Utils/DebugName.h"

namespace DX
{
    class RenderTarget;

    class DepthStencil
    {
      public:
        DepthStencil()  = default;
        ~DepthStencil() = default;
        /**
         * @brief Constructor that only sets width and height for the buffer.
         * @param device valid device pointer
         * @param width buffer width, same as render target
         * @param height buffer height, same as render target
         * @param viewDesc description for the depth stencil view, defaults to standard view desc.
         */
        DepthStencil(
            ID3D11Device*                        device,
            UINT                                 width,
            UINT                                 height,
            const D3D11_DEPTH_STENCIL_VIEW_DESC& viewDesc = GetViewDesc());
        /**
         * @brief Constructor that user descriptions for both buffer and view
         * @param device valid device pointer
         * @param viewDesc description for depth stencil view
         * @param depthStencilDesc description struct for depth stencil buffer (texture2D)
         */
        DepthStencil(
            ID3D11Device*                        device,
            const D3D11_DEPTH_STENCIL_VIEW_DESC& viewDesc,
            const D3D11_TEXTURE2D_DESC&          depthStencilDesc);

        /**
         * @brief Initializes the object after default construction, sets height and width for the buffer.
         * @param device valid device pointer
         * @param width buffer width, same as render target
         * @param height buffer height, same as render target
         * @param viewDesc description for the depth stencil view, defaults to standard view desc.
         */
        void Initialize(
            ID3D11Device*                        device,
            UINT                                 width,
            UINT                                 height,
            const D3D11_DEPTH_STENCIL_VIEW_DESC& viewDesc = GetViewDesc());

        /**
         * @brief Initializes the object after default construction
         * @param device valid device pointer
         * @param viewDesc description for depth stencil view
         * @param depthStencilDesc description struct for depth stencil buffer (texture2d)
         */
        void Initialize(
            ID3D11Device*                        device,
            const D3D11_DEPTH_STENCIL_VIEW_DESC& viewDesc,
            const D3D11_TEXTURE2D_DESC&          depthStencilDesc);

        /**
         * @brief Sets depth stencil view as depth target
         * @param context valid device context
         * @param rt render target being set with the depth stencil
         * @param viewCount amount of views, default 1
         */
        void Bind(
            ID3D11DeviceContext* context,
            const RenderTarget*  rt,
            UINT                 viewCount = 1) const;

        /// Remove the depth stencil view from depth target slot
        static void UnBind(ID3D11DeviceContext* context, UINT viewCount = 1);

        /// Clears the depth buffer
        void Clear(ID3D11DeviceContext* context) const;

        /// Reset the object so it can be reinitialized. Clears all the buffers.
        void Reset();

        /// Get raw pointer to ID3D11DepthStencilView
        ID3D11DepthStencilView* GetDSV() const
        {
            return m_depthStencilView.Get();
        }

        /// Get raw pointer to ID3D11Texture2D depth stencil buffer
        ID3D11Texture2D* GetDSBuffer() const
        {
            return m_depthStencilBuffer.Get();
        }

        void SetDebugName(const std::string& name)
        {
            Debug::SetDebugName(m_depthStencilBuffer, (name + ".texture2D").c_str());
            Debug::SetDebugName(m_depthStencilView, (name + ".dsv").c_str());
        }

        // Default descriptions

        /// Default DSV description, 24 bits for depth, 8 bits for stencil, dimension is Texture2D for single-sampled textures
        static D3D11_DEPTH_STENCIL_VIEW_DESC GetViewDesc()
        {
            return { .Format        = DXGI_FORMAT_D24_UNORM_S8_UINT,
                     .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
                     .Texture2D     = { .MipSlice = 0 } };
        }

        /**
         * @brief Create a default depth stencil buffer description.
         * @details Creates a D3D11_TEXTURE2D_DESC that sets format and flags for depth stencil, 
         * depth 24 and stencil 8 uint
         * @param width width of the buffer
         * @param height height of the buffer
         * @return Description to initialize a depth stencil buffer width
         * @note width and height NEED to be set during initialization, or it will have dimensions 0x0
         */
        static D3D11_TEXTURE2D_DESC
        GetTextureDesc(UINT width = 0, UINT height = 0)
        {
            return { .Width      = width,
                     .Height     = height,
                     .MipLevels  = 1,
                     .ArraySize  = 1,
                     .Format     = DXGI_FORMAT_D24_UNORM_S8_UINT,
                     .SampleDesc = { .Count = 1, .Quality = 0 },
                     .Usage      = D3D11_USAGE_DEFAULT,
                     .BindFlags  = D3D11_BIND_DEPTH_STENCIL,
                     .MiscFlags  = 0 };
        }

        // no copying
        /// No copy only move
        DepthStencil(const DepthStencil&)            = delete;
        /// No copy only move
        DepthStencil& operator=(const DepthStencil&) = delete;

      private:
        ComPtr<ID3D11Texture2D>        m_depthStencilBuffer;    ///< buffer
        ComPtr<ID3D11DepthStencilView> m_depthStencilView;      ///< view
    };
} // namespace DX
