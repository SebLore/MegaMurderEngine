#include "Abstraction/RenderTarget.h"

#include "Abstraction/DepthStencil.h"

#include <Utility/ErrorHandling.h>
#include <Utility/Logging.h>

#define LOG_TAG "RenderTarget"

using namespace DX;

namespace
{
    void ThrowIfFailed(HRESULT hr)
    {
        THROWRE_IF(FAILED(hr), "HRESULT failure: " << hr);
    }
} // namespace

RenderTarget::RenderTarget(
    ID3D11Device* device,
    UINT width,
    UINT height,
    DXGI_FORMAT format,
    UINT flags)
{
    Initialize(device, width, height, format, flags);
}

void RenderTarget::Initialize(
    ID3D11Device* device,
    UINT width,
    UINT height,
    DXGI_FORMAT format,
    UINT flags)
{
    Reset();

    // configure texture description
    auto texDesc = GetTextureDesc();

    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.Format = format;
    texDesc.BindFlags = flags;

    // attempt to create texture
    ThrowIfFailed(device->CreateTexture2D(
        &texDesc,
        nullptr,
        m_Texture.ReleaseAndGetAddressOf()));

    // RTV
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {
            .Format = format,
            .ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D
    };
    rtvDesc.Texture2D.MipSlice = 0;

    ThrowIfFailed(device->CreateRenderTargetView(
        m_Texture.Get(),
        &rtvDesc,
        m_RTV.ReleaseAndGetAddressOf()));

    if (flags & D3D11_BIND_SHADER_RESOURCE)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC
            srvDesc = { .Format = format,
                        .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
                        .Texture2D = {
                                .MostDetailedMip = 0,
                                .MipLevels = texDesc.MipLevels,
                        } };

        ThrowIfFailed(device->CreateShaderResourceView(
            m_Texture.Get(),
            &srvDesc,
            m_SRV.ReleaseAndGetAddressOf()));
    }

    if (flags & D3D11_BIND_UNORDERED_ACCESS)
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc =
        { .Format = format,
            .ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D };
        uavDesc.Texture2D.MipSlice = 0;

        ThrowIfFailed(device->CreateUnorderedAccessView(
            m_Texture.Get(),
            &uavDesc,
            m_UAV.ReleaseAndGetAddressOf()));
    }

    SetValid(true);
}

void RenderTarget::InitializeFromBackBuffer(
    ID3D11Device* device,
    ID3D11Texture2D* backBuffer,
    bool             UAV,
    bool             SRV)
{
    // reset all resources
    Reset();

    // store backbuffer ref
    m_Texture = backBuffer;

    D3D11_TEXTURE2D_DESC texDesc = {};
    backBuffer->GetDesc(&texDesc);

    D3D11_RENDER_TARGET_VIEW_DESC
        rtvDesc = {
            .Format = texDesc.Format,
            .ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D,
    };
    rtvDesc.Texture2D.MipSlice = 0;

    HRESULT hr = device->CreateRenderTargetView(
        backBuffer,
        &rtvDesc,
        m_RTV.ReleaseAndGetAddressOf());

    ThrowIfFailed(hr);

    // create optional views
    if (UAV)
    {
        hr = device->CreateUnorderedAccessView(
            backBuffer,
            nullptr,
            m_UAV.ReleaseAndGetAddressOf());

        if (FAILED(hr))
        {
            m_UAV.Reset();
            LOG_ERROR("Failed to create UAV. HRESULT: 0x" << hr);
        }
    }

    else if (SRV)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC
            srvDesc = { .Format = texDesc.Format,
                        .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
                        .Texture2D = {
                                .MostDetailedMip = 0,
                                .MipLevels = texDesc.MipLevels,
                        } };

        hr = device->CreateShaderResourceView(
            backBuffer,
            nullptr,
            m_SRV.ReleaseAndGetAddressOf());
        if (FAILED(hr))
        {
            m_SRV.Reset();
            LOG_ERROR("Failed to create SRV. HRESULT: 0x" << hr);
        }
    }
}

void RenderTarget::InitializeTexture2D(
    ID3D11Device* device,
    UINT width,
    UINT height,
    DXGI_FORMAT format,
    UINT bindFlags,
    UINT mipLevels,
    UINT arraySize)
{
    Reset();

    // configure texture description
    auto texDesc = GetTextureDesc();

    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = mipLevels;
    texDesc.ArraySize = arraySize;
    texDesc.Format = format;
    texDesc.BindFlags = bindFlags;

    // attempt to create texture
    ThrowIfFailed(device->CreateTexture2D(
        &texDesc,
        nullptr,
        m_Texture.ReleaseAndGetAddressOf()));

    // RTV
    D3D11_RENDER_TARGET_VIEW_DESC
        rtvDesc = {
            .Format = format,
            .ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D,
    };
    rtvDesc.Texture2D.MipSlice = 0;
    ThrowIfFailed(device->CreateRenderTargetView(
        m_Texture.Get(),
        &rtvDesc,
        m_RTV.ReleaseAndGetAddressOf()));

    SetValid(true);
}

void RenderTarget::InitializeFromTexture(
    ID3D11Device* device,
    ID3D11Texture2D* texture,
    const D3D11_RENDER_TARGET_VIEW_DESC& rtvDesc)
{
    Reset();

    HRESULT hr = device->CreateRenderTargetView(
        texture,
        &rtvDesc,
        m_RTV.ReleaseAndGetAddressOf());

    ThrowIfFailed(hr);

    // texture should now be valid
    SetValid(true);
}

void RenderTarget::Reset()
{
    m_Texture.Reset();
    m_RTV.Reset();
    m_UAV.Reset();
    m_SRV.Reset();
}

void RenderTarget::SetClearColor(float r, float g, float b, float a)
{
    m_ClearColor[0] = r;
    m_ClearColor[1] = g;
    m_ClearColor[2] = b;
    m_ClearColor[3] = a;
}

void RenderTarget::SetClearColor(float* rgba)
{
    for (int i = 0; i < 4; i++)
        m_ClearColor[i] = rgba[i];
}

void RenderTarget::Bind(ID3D11DeviceContext* context, const DepthStencil* dsv)
const
{

    context->OMSetRenderTargets(
        1,
        m_RTV.GetAddressOf(),
        dsv ? dsv->GetDSV() : nullptr);
}

void RenderTarget::BindCSUAV(ID3D11DeviceContext* context, UINT startSlot) const
{

    if (!(Valid()))
    {
        LOG_WARN("Attempting to bind invalid render target as UAV");
        return;
    }

    context->CSSetUnorderedAccessViews(
        startSlot,
        1,
        m_UAV.GetAddressOf(),
        nullptr);
}

void RenderTarget::UnBind(ID3D11DeviceContext* context)
{
    static ID3D11RenderTargetView* nullRTV[] = { nullptr };
    context->OMSetRenderTargets(1, nullRTV, nullptr);
}

void RenderTarget::UnBindCSUAV(ID3D11DeviceContext* context, UINT startSlot)
{
    static ID3D11UnorderedAccessView* nullUAV[] = { nullptr };
    context->CSSetUnorderedAccessViews(startSlot, 1, nullUAV, nullptr);
}

void RenderTarget::Clear(ID3D11DeviceContext* context) const
{
    context->ClearRenderTargetView(m_RTV.Get(), m_ClearColor);
}


void RenderTarget::BindSRV(
    ID3D11DeviceContext* context,
    UINT slot,
    UINT count) const
{
    context->PSSetShaderResources(slot, count, m_SRV.GetAddressOf());
}

void RenderTarget::UnBindSRV(
    ID3D11DeviceContext* context,
    UINT slot,
    UINT count) const
{
    ID3D11ShaderResourceView* nullSRV = nullptr;

    context->PSSetShaderResources(slot, count, &nullSRV);
}

UINT RenderTarget::GetWidth() const
{
    if (m_Texture)
    {
        D3D11_TEXTURE2D_DESC desc;
        m_Texture->GetDesc(&desc);
        return desc.Width;
    }
    return 0;
}

UINT RenderTarget::GetHeight() const
{
    if (m_Texture)
    {
        D3D11_TEXTURE2D_DESC desc;
        m_Texture->GetDesc(&desc);
        return desc.Height;
    }
    return 0;
}

#undef LOG_TAG