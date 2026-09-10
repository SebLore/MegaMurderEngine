#include "GpuLoader.h"

#include <ResourceManagement/Loaders/TextureLoader.h>

#include <Utility/Logging.h>

#define LOG_TAG "GpuLoader"

namespace Murder
{
    std::shared_ptr<DX::TextureSRV> GpuLoader::CreateTexture(const TextureUploadDesc& desc) const
    {
        if (!m_Device)
            return nullptr;

        ComPtr<ID3D11ShaderResourceView> srv;

        if (!desc.bytes.empty() && desc.isEncoded)
        {
            try
            {
                srv = TextureLoader::CreateTextureFromMemory(
                    m_Device,
                    reinterpret_cast<const uint8_t*>(desc.bytes.data()),
                    desc.bytes.size_bytes());
            }
            catch (const std::exception& e)
            {
                LOG_WARN("CreateTextureFromMemory failed for '" << desc.debugName << "' reason=" << e.what());
            }
        }

        if (!srv && !desc.bytes.empty() && !desc.isEncoded)
        {
            const UINT width    = desc.width;
            const UINT height   = desc.height;
            const UINT rowPitch = desc.rowPitch ? desc.rowPitch : (width * 4);

            if (width > 0 && height > 0)
            {
                D3D11_TEXTURE2D_DESC textureDesc{};
                textureDesc.Width            = width;
                textureDesc.Height           = height;
                textureDesc.MipLevels        = 1;
                textureDesc.ArraySize        = 1;
                textureDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
                textureDesc.SampleDesc.Count = 1;
                textureDesc.Usage            = D3D11_USAGE_DEFAULT;
                textureDesc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

                D3D11_SUBRESOURCE_DATA init{};
                init.pSysMem     = desc.bytes.data();
                init.SysMemPitch = rowPitch;

                ComPtr<ID3D11Texture2D> texture;
                if (SUCCEEDED(m_Device->CreateTexture2D(&textureDesc, &init, texture.GetAddressOf())) && texture)
                {
                    ComPtr<ID3D11ShaderResourceView> rawSrv;
                    if (SUCCEEDED(m_Device->CreateShaderResourceView(texture.Get(), nullptr, rawSrv.GetAddressOf())) &&
                        rawSrv)
                        srv = std::move(rawSrv);
                }
            }
        }

        if (!srv && !desc.sourcePath.empty())
        {
            try
            {
                srv = TextureLoader::LoadTextureFromFile(m_Device, desc.sourcePath);
            }
            catch (const std::exception& e)
            {
                LOG_WARN("LoadTextureFromFile failed for '" << desc.sourcePath << "' reason=" << e.what());
            }
        }

        if (!srv)
            return nullptr;

        return std::make_shared<DX::TextureSRV>(std::move(srv));
    }
} // namespace Murder

#undef LOG_TAG
