#include "Abstraction/DepthStencil.h"
#include "Abstraction/RenderTarget.h"

#include <Utility/ErrorHandling.h>

namespace DX
{
    DepthStencil::DepthStencil(
        ID3D11Device*                        device,
        UINT                                 width,
        UINT                                 height,
        const D3D11_DEPTH_STENCIL_VIEW_DESC& viewDesc)
    {
        D3D11_TEXTURE2D_DESC desc = GetTextureDesc(width, height);
        Initialize(device, viewDesc, desc);
    }

    DepthStencil::DepthStencil(
        ID3D11Device*                        device,
        const D3D11_DEPTH_STENCIL_VIEW_DESC& viewDesc,
        const D3D11_TEXTURE2D_DESC&          depthStencilDesc)
    {
        Initialize(device, viewDesc, depthStencilDesc);
    }

    void DepthStencil::Initialize(
        ID3D11Device*                        device,
        const D3D11_DEPTH_STENCIL_VIEW_DESC& viewDesc,
        const D3D11_TEXTURE2D_DESC&          depthStencilDesc)
    {
        THROWIA_IF(!device, "Device is nullptr");

        HRESULT hr = device->CreateTexture2D(
            &depthStencilDesc,
            nullptr,
            m_depthStencilBuffer.GetAddressOf());
        if (FAILED(hr))
            THROWRE("failed to create depth stencil buffer.");

        hr = device->CreateDepthStencilView(
            m_depthStencilBuffer.Get(),
            &viewDesc,
            m_depthStencilView.GetAddressOf());
        if (FAILED(hr))
            THROWRE("failed to create depth stencil view.");
    }

    void DepthStencil::Initialize(
        ID3D11Device*                        device,
        UINT                                 width,
        UINT                                 height,
        const D3D11_DEPTH_STENCIL_VIEW_DESC& viewDesc)
    {
        D3D11_TEXTURE2D_DESC texDesc = GetTextureDesc(width, height);

        Initialize(device, viewDesc, texDesc);
    }

    void DepthStencil::Bind(
        ID3D11DeviceContext* context,
        const RenderTarget*  rt,
        UINT                 viewCount) const
    {
        static ID3D11RenderTargetView* nullRT = nullptr;

        context->OMSetRenderTargets(
            viewCount,
            rt ? rt->GetRTVAddress() : &nullRT,
            m_depthStencilView.Get());
    }

    void DepthStencil::UnBind(ID3D11DeviceContext* context, UINT viewCount)
    {
        static ID3D11RenderTargetView* nullRTVs[8] = { nullptr };
        context->OMSetRenderTargets(viewCount, nullRTVs, nullptr);
    }

    void DepthStencil::Clear(ID3D11DeviceContext* context) const
    {
        context->ClearDepthStencilView(
            m_depthStencilView.Get(),
            D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
            1.0f,
            0);
    }

    void DepthStencil::Reset()
    {
        m_depthStencilBuffer.Reset();
        m_depthStencilView.Reset();
    }
} // namespace DX
