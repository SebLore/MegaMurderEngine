#include "pch.h"

#include "RenderEngine.h"

#include <Utility/ErrorHandling.h>
#include <Utility/Logging.h>

#define LOG_TAG "DX_Engine"

namespace DX
{
    RenderEngine::RenderEngine(HWND hwnd, UINT clientWidth, UINT clientHeight)
    {
        Initialize(hwnd, clientWidth, clientHeight);
    }

    void RenderEngine::Initialize(HWND hwnd, UINT width, UINT height)
    {
        m_HWND = hwnd;

        THROWRE_IF(!InitD3D(hwnd, width, height), "Failed to initialize D3D");
    }

    // return true if the window should close, i.e. if a WM_QUIT message was
    // received
    D3D11_TEXTURE2D_DESC RenderEngine::GetBackBufferDesc() const
    {
        ID3D11Texture2D* backBuffer = nullptr;
        m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
        D3D11_TEXTURE2D_DESC bbDesc;
        backBuffer->GetDesc(&bbDesc);
        backBuffer->Release();

        return bbDesc;
    }

    void RenderEngine::BeginFrame() const
    {
        m_BackBufferRT.Clear(m_Context.Get());
        m_DepthStencil.Clear(m_Context.Get());
    }

    void RenderEngine::BindDSVOnly() const
    {
        ClearRenderTargets();
        m_DepthStencil.Bind(m_Context.Get(), nullptr, 1);
    }

    void RenderEngine::BindBackBufferRTVAndDSV() const { m_DepthStencil.Bind(m_Context.Get(), &m_BackBufferRT, 1); }

    void RenderEngine::BindBackBufferAsUAV() const { m_BackBufferRT.BindCSUAV(m_Context.Get()); }

    void RenderEngine::UnBindBackBufferAsUAV() const
    {
        static ID3D11UnorderedAccessView* nullUAV = nullptr;
        m_Context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
    }

    void RenderEngine::UnBindBackBufferAsRTV() const
    {
        static ID3D11RenderTargetView* nullRTV = nullptr;
        m_Context->OMSetRenderTargets(1, &nullRTV, nullptr);
    }

    void RenderEngine::ClearBackBufferRTV() const { m_BackBufferRT.Clear(m_Context.Get()); }

    void RenderEngine::ClearDSV() const { m_DepthStencil.Clear(m_Context.Get()); }

    void RenderEngine::ClearScreen() const
    {
        m_BackBufferRT.Clear(m_Context.Get());
        m_DepthStencil.Clear(m_Context.Get());
    }

    HRESULT RenderEngine::PresentFrame(UINT vsync) const
    {
        return m_SwapChain->Present(vsync, 0); // Present with vsync
    }

    void RenderEngine::SetViewport(unsigned int width, unsigned int height)
    {
        m_DefaultVP.Width    = static_cast<float>(width);
        m_DefaultVP.Height   = static_cast<float>(height);
        m_DefaultVP.MinDepth = 0.0f;
        m_DefaultVP.MaxDepth = 1.0f;
        m_Context->RSSetViewports(1, &m_DefaultVP);
    }

    void RenderEngine::SetViewport(const D3D11_VIEWPORT& viewport)
    {
        m_DefaultVP = viewport;
        m_Context->RSSetViewports(1, &m_DefaultVP);
    }

    // == Rasterizer States ==

    const RasterizerState* RenderEngine::GetRasterizerState(RasterizerMode* state) const
    {
        if (state == nullptr)
            return &m_RasterizerStates[m_Modes.rasterizerMode];

        THROWOOR_IF(*state >= RasterizerMode::RASTERIZER_COUNT, "Requested rasterizer state is out of bounds.");

        if (*state < RasterizerMode::RASTERIZER_COUNT)
            return &m_RasterizerStates[static_cast<size_t>(*state)];

        // shouldn't hit but just in case
        LOG_ERROR("Requested rasterizer state is out of bounds.");
        return nullptr;
    }

    // Back Buffer
    void RenderEngine::ResizeBackBuffer(UINT width, UINT height)
    {
        if (width == 0 || height == 0)
            return;

        if (!m_Device || !m_Context || !m_SwapChain)
            return;

        ClearRenderTargets();
        ClearUnorderedAccessViews();
        ClearShaderResourceViews();

        m_BackBufferRT.Reset();
        m_DepthStencil.Reset();

        // resize backbuffer internally
        /*DirectX::ThrowIfFailed(*/
        m_SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

        // get the resized backbuffer
        ID3D11Texture2D* bb = nullptr;
        /*DirectX::ThrowIfFailed(*/ m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&bb));

        // reinitialize the back buffer
        m_BackBufferRT.InitializeFromBackBuffer(m_Device.Get(), bb, true, true);

        // reinitialize depth buffer
        m_DepthStencil.Initialize(m_Device.Get(), width, height);

        // release the buffer
        bb->Release();

        // update viewport to match the new size
        SetViewport(width, height);
    }

    D3D11_PRIMITIVE_TOPOLOGY RenderEngine::GetTopology() const
    {
        D3D11_PRIMITIVE_TOPOLOGY top{};
        m_Context->IAGetPrimitiveTopology(&top);
        return top;
    }

    bool RenderEngine::InitD3D(HWND hwnd, UINT width, UINT height)
    {
        // swap chain description
        DXGI_SWAP_CHAIN_DESC scd = {};
        scd.BufferCount          = 2;
        scd.BufferDesc.Width     = width;
        scd.BufferDesc.Height    = height;
        scd.BufferDesc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
        scd.BufferUsage          = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS;
        scd.OutputWindow         = hwnd;
        scd.SampleDesc.Count     = 1;
        scd.Windowed             = TRUE;
        scd.SwapEffect           = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        UINT flags               = 0;
#ifdef _DEBUG
        flags = D3D11_CREATE_DEVICE_DEBUG;
#endif

        // create device, swap chain and context
        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            flags,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            &scd,
            m_SwapChain.ReleaseAndGetAddressOf(),
            m_Device.ReleaseAndGetAddressOf(),
            nullptr,
            m_Context.ReleaseAndGetAddressOf());

        if (FAILED(hr))
        {
            LOG_ERROR("D3D11CreateDeviceAndSwapChain failed. HRESULT: 0x%X" << hr);
            return false;
        }

        // get back buffer
        ComPtr<ID3D11Texture2D> backBuffer;
        hr = m_SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
        if (FAILED(hr))
        {
            LOG_ERROR("GetBuffer for back buffer failed. HRESULT: 0x" << std::hex << hr);
            return false;
        }

        // create render target view from back buffer
        m_BackBufferRT.InitializeFromBackBuffer(m_Device.Get(), backBuffer.Get(), true, true);

        // create depth stencil
        m_DepthStencil.Initialize(m_Device.Get(), width, height);

        // rasterizer state
        if (!GenerateRasterizerStates())
        {
            LOG_ERROR("Failed to generate rasterizer states.");
            return false;
        }

        if (!GenerateDepthStencilStates())
        {
            LOG_ERROR("Failed to generate depth stencil states.");
            return false;
        }

        if (!GenerateBlendStates())
        {
            LOG_ERROR("Failed to generate blend states.");
            return false;
        }

        // Set viewport
        m_DefaultVP.Width    = static_cast<float>(width);
        m_DefaultVP.Height   = static_cast<float>(height);
        m_DefaultVP.MinDepth = 0.0f;
        m_DefaultVP.MaxDepth = 1.0f;
        m_Context->RSSetViewports(1, &m_DefaultVP);

        // create basic texture sampler state
        D3D11_SAMPLER_DESC samplerDesc = SamplerState::DefaultDesc();
        m_DefaultSamplerState.Initialize(m_Device.Get(), 0);
        m_DefaultSamplerState.Set(m_Context.Get(), PS);

        // set default blend state for alpha channel blending
        m_BlendStates[BLEND_DEFAULT].Set(m_Context.Get());

        // finally set topology to triangles
        m_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        return true;
    }

    // Generate various rasterizer states for later use
    bool RenderEngine::GenerateRasterizerStates()
    {
        m_RasterizerStates.resize(RasterizerMode::RASTERIZER_COUNT);
        // default rasterizer state
        D3D11_RASTERIZER_DESC rasterDesc = RasterizerState::DefaultDesc();
        rasterDesc.DepthClipEnable       = false; // disable depth clipping for now
        if (!m_RasterizerStates[RASTERIZER_DEFAULT].Initialize(m_Device.Get(), rasterDesc))
            return false;

        // wireframe rasterizer state
        rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
        if (!m_RasterizerStates[RASTERIZER_WIREFRAME].Initialize(m_Device.Get(), rasterDesc))
            return false;

        // wireframe cull none rasterizer state
        rasterDesc.CullMode = D3D11_CULL_NONE;
        if (!m_RasterizerStates[RASTERIZER_WIREFRAME_CULL_NONE].Initialize(m_Device.Get(), rasterDesc))
            return false;

        // cull none rasterizer state
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.CullMode = D3D11_CULL_NONE;
        if (!m_RasterizerStates[RASTERIZER_CULL_NONE].Initialize(m_Device.Get(), rasterDesc))
            return false;

        // cull front rasterizer state
        rasterDesc.CullMode = D3D11_CULL_FRONT;
        if (!m_RasterizerStates[RASTERIZER_CULL_FRONT].Initialize(m_Device.Get(), rasterDesc))
            return false;

        return true;
    }

    /// @brief Generate various depth stencil states for later use
    /// @return true if successful, false otherwise
    bool RenderEngine::GenerateDepthStencilStates()
    {
        D3D11_DEPTH_STENCIL_DESC dsDesc = DepthStencilState::DefaultDesc();

        // default depth stencil state
        DepthStencilState defaultDS{};

        if (!defaultDS.Initialize(m_Device.Get(), dsDesc))
            return false;

        m_DepthStencilStates.push_back(std::move(defaultDS));

        // no depth stencil state
        dsDesc.DepthEnable = FALSE;
        DepthStencilState noDepthDS{};
        if (!noDepthDS.Initialize(m_Device.Get(), dsDesc))
            return false;

        m_DepthStencilStates.push_back(std::move(noDepthDS));

        // stencil testing enabled
        dsDesc.DepthEnable   = TRUE;
        dsDesc.StencilEnable = TRUE;
        DepthStencilState stencilDS{};
        if (!stencilDS.Initialize(m_Device.Get(), dsDesc))
            return false;

        m_DepthStencilStates.push_back(std::move(stencilDS));

        return true;
    }

    bool RenderEngine::GenerateBlendStates()
    {
        D3D11_BLEND_DESC blendDesc      = BlendState::DefaultDesc();
        constexpr FLOAT  blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; // RGBA
        constexpr UINT   sampleMask     = 0xFFFFFFFF;
        // create blend state
        BlendState defaultBlend;
        defaultBlend.Initialize(m_Device.Get(), blendFactor, sampleMask, blendDesc);
        m_BlendStates.push_back(std::move(defaultBlend));

        // blend state without alpha blending for deferred rendering
        blendDesc.RenderTarget[0].BlendEnable = FALSE;
        BlendState noBlend;
        noBlend.Initialize(m_Device.Get(), blendFactor, sampleMask, blendDesc);
        m_BlendStates.push_back(std::move(noBlend));

        return true;
    }

    void RenderEngine::SetDepthStencilMode(DepthStencilMode mode)
    {
        THROWOOR_IF(mode >= DepthStencilMode::DEPTH_DISABLE + 1, "Requested depth stencil mode is out of bounds.");

        m_DepthStencilStates[static_cast<size_t>(mode)].Set(m_Context.Get());
        m_Modes.depthStencilMode = mode;
    }

    void RenderEngine::SetRasterizerMode(RasterizerMode mode)
    {
        THROWOOR_IF((mode >= RasterizerMode::RASTERIZER_COUNT + 1), "mode is OOR of m_Modes.");

        m_RasterizerStates[static_cast<size_t>(mode)].Set(m_Context.Get());
        m_Modes.rasterizerMode = mode;
    }

    void RenderEngine::SetRasterizerState(RasterizerMode state) const
    {
        if (state < RasterizerMode::RASTERIZER_COUNT)
            m_RasterizerStates[state].Set(m_Context.Get());
        else
            LOG_ERROR("Requested rasterizer state is out of bounds.");
    }

    void RenderEngine::SetBlendMode(BlendMode mode) const
    {
        THROWOOR_IF((mode > BlendMode::BLEND_DISABLED), "Requested blend mode is out of bounds.");

        m_BlendStates[static_cast<size_t>(mode)].Set(m_Context.Get());
    }

    void RenderEngine::SetDepthMode(DepthStencilMode mode) const
    {
        THROWOOR_IF((mode > DepthStencilMode::STENCIL_ENABLED), "Requested depth stencil mode is out of bounds.");

        m_DepthStencilStates[static_cast<size_t>(mode)].Set(m_Context.Get());
    }

    void RenderEngine::SetClearColor(float r, float g, float b, float a) { m_BackBufferRT.SetClearColor(r, g, b, a); }

    void RenderEngine::ClearRenderTargets() const
    {
        if (!m_Context)
            return;
        ID3D11RenderTargetView* nullRTVs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};

        m_Context->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, nullRTVs, nullptr);
    }

    void RenderEngine::ClearUnorderedAccessViews() const
    {
        if (!m_Context)
            return;

        ID3D11UnorderedAccessView* nullUAVs[D3D11_PS_CS_UAV_REGISTER_COUNT] = {};
        UINT                       counts[D3D11_PS_CS_UAV_REGISTER_COUNT]   = {};
        m_Context->CSSetUnorderedAccessViews(0, D3D11_PS_CS_UAV_REGISTER_COUNT, nullUAVs, counts);
    }

    void RenderEngine::ClearShaderResourceViews() const
    {
        if (!m_Context)
            return;

        ID3D11ShaderResourceView* nullSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};

        m_Context->VSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullSRVs);
        m_Context->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullSRVs);
        m_Context->GSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullSRVs);
        m_Context->HSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullSRVs);
        m_Context->DSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullSRVs);
        m_Context->CSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullSRVs);
    }

} // namespace DX

#undef LOG_TAG
