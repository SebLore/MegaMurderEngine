#include "Techniques/ShadowMap.h"

#include <Utility/Logging.h>

#define LOG_TAG "ShadowMap"

namespace DX
{

    ShadowMap::ShadowMap(ID3D11Device* device, UINT shadowCount, UINT width, UINT height)
    {
        Initialize(device, shadowCount, width, height);
    }

    void ShadowMap::Initialize(ID3D11Device* device, UINT shadowCount, UINT width, UINT height)
    {
        THROWIA_IF(!device, "Device is null");

        // create texture array for shadow maps
        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width                = width;
        textureDesc.Height               = height == 0 ? width : height;
        // if height is 0, make a square shadow map
        textureDesc.ArraySize            = shadowCount;
        textureDesc.MipLevels            = 1;
        textureDesc.Format               = DXGI_FORMAT_R32_TYPELESS;
        textureDesc.SampleDesc.Count     = 1;
        textureDesc.BindFlags            = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
        textureDesc.Usage                = D3D11_USAGE_DEFAULT;

        HRESULT hr = device->CreateTexture2D(&textureDesc, nullptr, m_texture.GetAddressOf());
        if (FAILED(hr))
            THROWRE("Failed to create shadow map texture");

        // create DSV desc, mostly the same for each shadow map except for the first array slice
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format                        = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension                 = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Texture2DArray.MipSlice       = 0;
        dsvDesc.Texture2DArray.ArraySize      = 1;

        // create DSVs for each shadow map
        m_DSVs.reserve(shadowCount);
        for (UINT i = 0; i < shadowCount; ++i)
        {
            dsvDesc.Texture2DArray.FirstArraySlice = i;
            ComPtr<ID3D11DepthStencilView> dsv;
            hr = device->CreateDepthStencilView(m_texture.Get(), &dsvDesc, dsv.GetAddressOf());
            if (FAILED(hr))
                THROWRE("Failed to create depth stencil view for shadow map");

            m_DSVs.push_back(std::move(dsv));
        }

        // create shader resource view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                          = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension                   = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        srvDesc.Texture2DArray.MostDetailedMip  = 0;
        srvDesc.Texture2DArray.MipLevels        = 1;
        srvDesc.Texture2DArray.FirstArraySlice  = 0;
        srvDesc.Texture2DArray.ArraySize        = shadowCount;

        hr = device->CreateShaderResourceView(m_texture.Get(), &srvDesc, m_srv.GetAddressOf());
        if (FAILED(hr))
            THROWRE("Failed to create shader resource view for shadow map");

        // make a rect for setting the viewport
        m_viewport.Width    = static_cast<FLOAT>(width);
        m_viewport.Height   = static_cast<FLOAT>(textureDesc.Height);
        m_viewport.MinDepth = 0.0f;
        m_viewport.MaxDepth = 1.0f;
        m_viewport.TopLeftX = 0.0f;
        m_viewport.TopLeftY = 0.0f;

        // release texture (does it automatically but we're being good boys and
        // being explicit)
        m_texture.Reset();

        // create own sampler state for ease of use
        m_samplerState.Initialize(device, R_SHADOW_SAMPLER, GetSamplerStateDesc());

        // create own rasterizer state for ease of use
        m_RasterizerState.Initialize(device, GetRasterizerStateDesc());
    }

    void ShadowMap::Clear(ID3D11DeviceContext* context, UINT index) const
    {
        context->ClearDepthStencilView(m_DSVs[index].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    }

    void ShadowMap::SetViewport(ID3D11DeviceContext* context) const { context->RSSetViewports(1, &m_viewport); }

    void ShadowMap::SetAsDSV(ID3D11DeviceContext* context, UINT index) const
    {
        context->OMSetRenderTargets(0, nullptr, m_DSVs[index].Get());
    }

    void ShadowMap::BindSRV(ID3D11DeviceContext* context, DX::SHADER_STAGE stage, UINT slot) const
    {
        BindShaderSRV(context, slot, stage, m_srv.GetAddressOf());
    }

    void ShadowMap::UnBindSRV(ID3D11DeviceContext* context, SHADER_STAGE stage, UINT slot) const
    {
        UnBindShaderSRV(context, slot, stage);
    }

    void ShadowMap::UnSetDSV(ID3D11DeviceContext* context) const { context->OMSetRenderTargets(0, nullptr, nullptr); }

    void ShadowMap::BindSamplerState(ID3D11DeviceContext* context, SHADER_STAGE stage, UINT slot) const
    {
        m_samplerState.Set(context, stage, slot);
    }

    void ShadowMap::UnBindSamplerState(ID3D11DeviceContext* context, SHADER_STAGE stage, UINT slot) const
    {
        m_samplerState.UnSet(context, stage);
    }

    void ShadowMap::BindRasterizerState(ID3D11DeviceContext* context) const { m_RasterizerState.Set(context); }
    void ShadowMap::UnBindRasterizerState(ID3D11DeviceContext* context) const { m_RasterizerState.Set(context); }

    ID3D11DepthStencilView* ShadowMap::GetDSV(UINT index) const
    {
        THROWOOR_IF(index >= m_DSVs.size(), "Index out of range");
        return m_DSVs[index].Get();
    }

    UINT ShadowMap::GetShadowCount() const { return static_cast<UINT>(m_DSVs.size()); }

    D3D11_VIEWPORT ShadowMap::GetShadowViewport() const { return m_viewport; }

} // namespace DX
