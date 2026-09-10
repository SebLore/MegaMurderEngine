#pragma once
#include <string>
#include <vector>

#include "Common/D3D11Headers.h"

#include "Abstractions.h"

#include <Utility/Clock.h>

namespace DX
{
    class RenderEngine
    {
      public:
        // structures and enums
        enum RasterizerMode : int
        {
            RASTERIZER_DEFAULT = 0,
            RASTERIZER_WIREFRAME,
            RASTERIZER_WIREFRAME_CULL_NONE,
            RASTERIZER_CULL_NONE,
            RASTERIZER_CULL_FRONT,
            RASTERIZER_COUNT
        };

        enum DepthStencilMode : int
        {
            DEPTH_DEFAULT = 0,
            DEPTH_DISABLE,
            STENCIL_ENABLED
        };

        enum BlendMode : int
        {
            BLEND_DEFAULT = 0,
            BLEND_DISABLED
        };

        struct Modes
        {
            RasterizerMode   rasterizerMode   = RASTERIZER_DEFAULT;
            DepthStencilMode depthStencilMode = DEPTH_DEFAULT;
            BlendMode        blendMode        = BLEND_DEFAULT;
        };

      public:
        explicit RenderEngine(HWND hwnd, UINT clientWidth, UINT clientHeight);
        ~RenderEngine()                                  = default;
        RenderEngine(RenderEngine&&) noexcept            = default;
        RenderEngine& operator=(RenderEngine&&) noexcept = default;

        void Initialize(HWND hwnd, UINT width, UINT height);

        /// Begin the frame by clearing the render target and depth stencil views
        void BeginFrame() const;

        /// Direction the frame by presenting the swapchain
        HRESULT PresentFrame(UINT vsync = 1) const;

        /// Bind Depth Target only
        void BindDSVOnly() const;

        /// Bind Backbuffer Render Target View and Depth Stencil View
        void BindBackBufferRTVAndDSV() const;

        /// Bind Backbuffer as Unordered Access View
        void BindBackBufferAsUAV() const;

        /// Unbind Backbuffer as Unordered Access View
        void UnBindBackBufferAsUAV() const;

        /// Unbind Backbuffer as Render Target View
        void UnBindBackBufferAsRTV() const;

        void ClearBackBufferRTV() const;
        void ClearDSV() const;
        void ClearScreen() const;

        // state/mode management
        const Modes& GetModes() const { return m_Modes; }
        Modes&       GetModes() { return m_Modes; }

        DepthStencilMode GetDepthStencilMode() const { return m_Modes.depthStencilMode; }
        void             SetDepthStencilMode(DepthStencilMode mode);

        RasterizerMode         GetRasterizerMode() const { return m_Modes.rasterizerMode; }
        void                   SetRasterizerMode(RasterizerMode mode);
        const RasterizerState* GetRasterizerState(RasterizerMode* state = nullptr) const;
        void                   SetRasterizerState(RasterizerMode state) const;

        const SamplerState& GetDefaultSamplerState() const { return m_DefaultSamplerState; }

        // COM accessors
        // TODO: replace these with functions that do specific things, don't keep the COM objects exposed
        ID3D11Device*        GetDevice() const { return m_Device.Get(); }
        ID3D11DeviceContext* GetContext() const { return m_Context.Get(); }
        IDXGISwapChain*      GetSwapChain() const { return m_SwapChain.Get(); }

        D3D11_TEXTURE2D_DESC GetBackBufferDesc() const;

        RenderTarget&       GetBackBufferRT() { return m_BackBufferRT; }
        const RenderTarget& GetBackBufferRT() const { return m_BackBufferRT; }
        DepthStencil&       GetDepthStencil() { return m_DepthStencil; }
        const DepthStencil& GetDepthStencil() const { return m_DepthStencil; }

        void ResizeBackBuffer(UINT width, UINT height);

        UINT GetBackBufferWidth() const { return m_BackBufferRT.GetWidth(); }
        UINT GetBackBufferHeight() const { return m_BackBufferRT.GetHeight(); }

        D3D11_PRIMITIVE_TOPOLOGY GetTopology() const;
        void SetTopology(D3D11_PRIMITIVE_TOPOLOGY top) const { m_Context->IASetPrimitiveTopology(top); }

        void SetClearColor(float r, float g, float b, float a);
        void SetClearColor(float rgba[]);

        /// Set the current blend mode
        void SetBlendMode(BlendMode mode) const;
        void SetDepthMode(DepthStencilMode mode) const;

        /// Clear ALL bound render targets
        void ClearRenderTargets() const;
        /// Clear ALL bound unordered access views
        void ClearUnorderedAccessViews() const;
        /// Clear ALL SRVs in all pipelines
        void ClearShaderResourceViews() const;

        /// Gets the current viewport
        const D3D11_VIEWPORT& GetViewport() const { return m_DefaultVP; }
        void                  SetViewport(unsigned int width, unsigned int height);
        void                  SetViewport(const D3D11_VIEWPORT& viewport);

        /// Get handle to the window engine is rendering to
        HWND GetHWND() const { return m_HWND; }

        // no copying
        RenderEngine(const RenderEngine&)            = delete;
        RenderEngine& operator=(const RenderEngine&) = delete;

      private:
        /// Initialize D3D11 state
        bool InitD3D(HWND, UINT, UINT);

        // generators
        // TODO: make configurable, right now they are hard-coded

        /// Generate various rasterizer states for later use
        bool GenerateRasterizerStates();

        /// Generate various depth stencil states for later use
        bool GenerateDepthStencilStates();

        /// Generate various blend states for later use
        bool GenerateBlendStates();

      private:
        HWND           m_HWND      = nullptr;
        int            m_Width     = 0;              ///< window client width
        int            m_Height    = 0;              ///< window client height
        D3D11_VIEWPORT m_DefaultVP = { 0, 0, 0, 0 }; ///< viewport

        ComPtr<ID3D11Device>        m_Device;
        ComPtr<ID3D11DeviceContext> m_Context;
        ComPtr<IDXGISwapChain>      m_SwapChain;

        DepthStencil m_DepthStencil = {};
        RenderTarget m_BackBufferRT;

        // states
        // TODO: move these into some dedicated class instead
        std::vector<BlendState>        m_BlendStates        = {};
        std::vector<DepthStencilState> m_DepthStencilStates = {};
        std::vector<RasterizerState>   m_RasterizerStates   = {};
        SamplerState                   m_DefaultSamplerState;

        // Settings
        Modes m_Modes = { RASTERIZER_DEFAULT, DEPTH_DEFAULT, BLEND_DEFAULT };
    };
} // namespace DX
