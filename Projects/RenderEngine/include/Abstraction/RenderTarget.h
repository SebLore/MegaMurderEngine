#pragma once

#include "Common/D3D11Headers.h"
#include "Utils/DebugName.h"


namespace DX
{
    class DepthStencil;

    class RenderTarget
    {
    public:
        RenderTarget() = default;
        ~RenderTarget() = default;

        RenderTarget(RenderTarget&&) = default;
        RenderTarget& operator=(RenderTarget&&) = default;

        RenderTarget(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT format, UINT flags);

        void Initialize(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT format, UINT flags);
        // INITIALIZER FUNCTIONS

        /*
         * @brief Initialize the render target from the backbuffer texture
         * @param device - D3D11 device
         * @param backBuffer - backbuffer texture
         * @param UAV - Flag to create as unordered access view
         * @param SRV - Flag to create as shader resource view
         * @note Swapchain owns the backbuffer texture, we are only binding it to views not owning it
         */
        void InitializeFromBackBuffer(
            ID3D11Device* device,
            ID3D11Texture2D* backBuffer,
            bool             UAV = false,
            bool             SRV = false);
        /**
         *
         * @param device valid device
         * @param width Width of the texture
         * @param height
         * @param format
         * @param bindFlags
         * @param mipLevels
         * @param arraySize
         */
        void InitializeTexture2D(
            ID3D11Device* device,
            UINT width,
            UINT height,
            DXGI_FORMAT format,
            UINT bindFlags,
            UINT mipLevels = 1,
            UINT arraySize = 1
        );

        /// same as backbuffer but with custom RTV description
        void InitializeFromTexture(
            ID3D11Device* device,
            ID3D11Texture2D* texture,
            const D3D11_RENDER_TARGET_VIEW_DESC& rtvDesc);

        // END OF INITIALIZERS

        // Member functions

        bool IsInitialized() const { return m_Texture && m_RTV; }

        void Reset();

        void         SetClearColor(float r, float g, float b, float a);
        void         SetClearColor(float* rgba);
        const float* GetClearColor() const { return m_ClearColor; }

        void Bind(ID3D11DeviceContext* context, const DepthStencil* dsv) const;
        void BindCSUAV(ID3D11DeviceContext* context, UINT startSlot = 0) const;

        static void UnBind(ID3D11DeviceContext* context);
        static void
            UnBindCSUAV(ID3D11DeviceContext* context, UINT startSlot = 0);

        void Clear(ID3D11DeviceContext* context) const;

        ID3D11RenderTargetView* const* GetRTVAddress() const
        {
            return m_RTV.GetAddressOf();
        }
        ID3D11RenderTargetView* GetRTV() const { return m_RTV.Get(); }
        ID3D11ShaderResourceView* GetSRV() const { return m_SRV.Get(); }
        ID3D11UnorderedAccessView* GetUAV() const { return m_UAV.Get(); }
        ID3D11Texture2D* GetTexture() const { return m_Texture.Get(); }

        // binding views

        void BindSRV(ID3D11DeviceContext* context, UINT slot = 0, UINT count = 1) const;

        void UnBindSRV(ID3D11DeviceContext* context, UINT slot = 0, UINT count = 1) const;



        /// returns buffer height, if not initialised returns 0
        UINT GetWidth() const;
        /// returns buffer height, if not initialised returns 0
        UINT GetHeight()const;

        bool Valid()const { return m_Valid; }

        void SetDebugName(const std::string& name)
        {
            Debug::SetDebugName(m_Texture, (name + ".texture2D").c_str());
            Debug::SetDebugName(m_RTV, (name + ".rtv").c_str());
            Debug::SetDebugName(m_UAV, (name + ".uav").c_str());
            Debug::SetDebugName(m_SRV, (name + ".srv").c_str());
        }

        // no copy
        RenderTarget(const RenderTarget&) = delete;
        RenderTarget& operator=(const RenderTarget&) = delete;

    public:
        static D3D11_RENDER_TARGET_VIEW_DESC GetViewDesc()
        {
            D3D11_RENDER_TARGET_VIEW_DESC desc = {};
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipSlice = 0;
            return desc;
        }

        // texture description for render target, when not using UAV or SRV
        static constexpr D3D11_TEXTURE2D_DESC GetTextureDesc()
        {
            return D3D11_TEXTURE2D_DESC{
                .Width = 0,
                .Height = 0,
                .MipLevels = 1,
                .ArraySize = 1,
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .SampleDesc = DXGI_SAMPLE_DESC{.Count = 1, .Quality = 0 },
                .Usage = D3D11_USAGE_DEFAULT,
                .BindFlags = D3D11_BIND_RENDER_TARGET,
                .CPUAccessFlags = 0,
                .MiscFlags = 0,
            };
        }

    private:
        void SetValid(bool value) { m_Valid = value; }
    private:
        float m_ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        ComPtr<ID3D11Texture2D>           m_Texture;
        ComPtr<ID3D11RenderTargetView>    m_RTV;
        ComPtr<ID3D11UnorderedAccessView> m_UAV;
        ComPtr<ID3D11ShaderResourceView>  m_SRV;

        bool m_Valid = false; ///< valid flag, set to true when the render target is successfully initialized, used to prevent invalid bind calls
    };

    inline
        RenderTarget MakeRenderTarget(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT format, UINT bindFlags)
    {
        RenderTarget rt;
        rt.InitializeTexture2D(device, width, height, format, bindFlags);
        return rt;
    }


} // namespace DX
