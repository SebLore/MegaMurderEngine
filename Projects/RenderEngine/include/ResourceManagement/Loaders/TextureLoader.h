#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include <d3d11.h>
#include <wrl/client.h> // comptr

#include <WICTextureLoader.h>

#include <Utility/ErrorHandling.h>
#include <Utility/Logging.h>

#include "Abstraction/ShaderResourceView.h"

#define LOG_TAG "TextureLoader"

// silence C6387 warning for WICTextureLoader functions
#pragma warning(push)
#pragma warning(disable : 6387)

using Microsoft::WRL::ComPtr;

class TextureLoader
{
  public:
    TextureLoader()  = default;
    ~TextureLoader() = default;

    /// @brief Load a texture from a file using WICTextureLoader and returns a shader resource view
    /// @param device valid D3D11 device
    /// @param filePath path to the texture file
    /// @return returns a ComPtr to the shader resource view of the texture, texture component is released
    static ComPtr<ID3D11ShaderResourceView> LoadTextureFromFile(ID3D11Device* device, const std::string& filePath)
    {
        THROWIA_IF(!device, "Device is nullptr");
        THROWIA_IF(filePath.empty(), "File path is empty");

        ComPtr<ID3D11ShaderResourceView> textureView;
        std::filesystem::path            path(filePath);

        // print path as absolute path to verify it's loading from the right place
        LOG_DEBUG("Loading texture from file: " + std::filesystem::absolute(path).string());

        HRESULT hr = DirectX::CreateWICTextureFromFile(device, path.c_str(), nullptr, textureView.GetAddressOf());
        if (FAILED(hr))
            THROWRE("Failed to load texture from file");

        return textureView;
    }

    /// @brief Create a texture from memory using WICTextureLoader and returns a shader resource view
    /// @param device valid D3D11 device
    /// @param data pointer to the texture data in memory
    /// @param size of the texture data in bytes
    /// @return returns a ComPtr to the shader resource view of the texture, texture component is released
    static ComPtr<ID3D11ShaderResourceView>
    CreateTextureFromMemory(ID3D11Device* device, const uint8_t* data, size_t size)
    {
        THROWIA_IF(!device || (data == nullptr), "An argument was nullptr");
        THROWIA_IF(size == 0, "Size is 0");

        ComPtr<ID3D11ShaderResourceView> textureView;
        // Load the texture from memory using WICTextureLoader
        HRESULT hr = DirectX::CreateWICTextureFromMemory(
            device,
            data ? data : nullptr,
            size,
            nullptr,
            textureView.GetAddressOf());
        if (FAILED(hr))
            THROWRE("Failed to create texture from memory");
        return textureView;
    }

    /// @brief Create a simple color texture of specified width and height, filled with the given RGBA color
    /// @param device valid D3D11 device
    /// @param rgba array of 4 uint8_t values representing the color in RGBA format
    /// @param width width of the texture, default is 1
    /// @param height height of the texture, default is 1
    /// @return returns a ComPtr to the shader resource view of the texture
    static ComPtr<ID3D11ShaderResourceView>
    CreateColorTexture(ID3D11Device* device, uint8_t rgba[4], UINT m_width = 1, UINT m_height = 1)
    {
        THROWIA_IF(!device, "Device is nullptr");

        std::vector<uint8_t> pixelData(m_width * m_height * 4);

        // create pixel data, incrementing by 4 for the r, g, b and a channels
        for (size_t i = 0; i < pixelData.size(); i += 4)
        {
            pixelData[i + 0] = rgba[0];
            pixelData[i + 1] = rgba[1];
            pixelData[i + 2] = rgba[2];
            pixelData[i + 3] = rgba[3];
        }

        // create a texture description
        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width                = m_width;
        textureDesc.Height               = m_height;
        textureDesc.MipLevels            = 1;
        textureDesc.ArraySize            = 1;
        textureDesc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count     = 1;
        textureDesc.Usage                = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags            = D3D11_BIND_SHADER_RESOURCE;
        textureDesc.CPUAccessFlags       = 0;
        textureDesc.MiscFlags            = 0;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem                = pixelData.data();
        initData.SysMemPitch            = m_width * 4;

        // create the texture
        ComPtr<ID3D11Texture2D> texture;
        HRESULT                 hr = device->CreateTexture2D(&textureDesc, &initData, texture.GetAddressOf());
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to create color texture");
            return nullptr;
        }

        // create the shader resource view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                          = textureDesc.Format;
        srvDesc.ViewDimension                   = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip       = 0;
        srvDesc.Texture2D.MipLevels             = 1;

        // create the shader resource view
        ComPtr<ID3D11ShaderResourceView> textureView;
        hr = device->CreateShaderResourceView(texture.Get(), &srvDesc, textureView.GetAddressOf());
        if (FAILED(hr))
            THROWRE("Failed to create shader resource view");

        return textureView;
    }

    // no copying or moving
    TextureLoader(const TextureLoader&)             = delete;
    TextureLoader& operator=(const TextureLoader&)  = delete;
    TextureLoader(TextureLoader&& other)            = delete;
    TextureLoader& operator=(TextureLoader&& other) = delete;
};

#undef LOG_TAG
