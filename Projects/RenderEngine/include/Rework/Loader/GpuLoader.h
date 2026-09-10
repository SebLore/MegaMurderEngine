#pragma once

#include <memory>

#include <Common/Bridge/TextureUploadDesc.h>

#include <Abstraction/ShaderResourceView.h>

namespace Murder
{
    class GpuLoader
    {
      public:
        explicit GpuLoader(ID3D11Device* device) : m_Device(device) {}

        std::shared_ptr<DX::TextureSRV> CreateTexture(const TextureUploadDesc& desc) const;

      private:
        ID3D11Device* m_Device = nullptr; // non-owning
    };
} // namespace Murder
