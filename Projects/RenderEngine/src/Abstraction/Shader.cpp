#include "Abstraction/Shader.h"

#include <d3dcompiler.h>
#include <filesystem>
#include <cstring>

#include <Utility/ErrorHandling.h>
#include <Utility/Logging.h>

#define LOG_TAG "Shader"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib") // for WKPID

namespace DX
{
    namespace
    {
        void NameDeviceChild(ID3D11DeviceChild* childPtr, UINT dataSize, const char* name)
        {
            childPtr->SetPrivateData(WKPDID_D3DDebugObjectName, dataSize, name);
        }
    } // namespace

    Shader::~Shader()
    {
        switch (m_shaderType)
        {
        case DX::VS:
            m_shaderUnion.vertexShader.Reset();
            break;
        case DX::PS:
            m_shaderUnion.pixelShader.Reset();
            break;
        case DX::GS:
            m_shaderUnion.geometryShader.Reset();
            break;
        case DX::HS:
            m_shaderUnion.hullShader.Reset();
            break;
        case DX::DS:
            m_shaderUnion.domainShader.Reset();
            break;
        case DX::CS:
            m_shaderUnion.computeShader.Reset();
            break;
        default:
            break;
        }
    }
    Shader::Shader(
        ID3D11Device*                   device,
        const std::string&              filePath,
        SHADER_STAGE                    shaderType,
        const char*                     entryPoint,
        const D3D_SHADER_MACRO*         defines,
        ID3DInclude*                    include,
        UINT                            compileFlags,
        UINT                            effectFlags,
        const D3D11_INPUT_ELEMENT_DESC* inputLayout,
        UINT                            elementCount)
    {
        InitializeFromFile(
            device,
            filePath,
            shaderType,
            entryPoint,
            defines,
            include,
            compileFlags,
            effectFlags,
            inputLayout,
            elementCount);
    }

    Shader::Shader(
        ID3D11Device*                   device,
        const void*                     byteCode,
        size_t                          byteCodeSize,
        SHADER_STAGE                    shaderType,
        const char*                     entrypoint,
        const D3D_SHADER_MACRO*         defines,
        ID3DInclude*                    include,
        UINT                            compileFLags,
        UINT                            effectFlags,
        const D3D11_INPUT_ELEMENT_DESC* layout,
        UINT                            elementCount)
    {
        InitializeFromMemory(
            device,
            byteCode,
            byteCodeSize,
            shaderType,
            entrypoint,
            defines,
            include,
            compileFLags,
            effectFlags,
            layout,
            elementCount);
    }

    /*
    * InitializeFromMemory
     *
     * Initializes the shader from memory. The byte code is passed in as a raw byte array.
     * The shader type is specified and the input layout is created if needed.
     */
    void Shader::InitializeFromMemory(
        ID3D11Device* device,
        const void*   byteCode,     // raw byte code
        size_t        byteCodeSize, // size of the byte code
        SHADER_STAGE  shaderType,   // type of the shader
        // optional arguments
        const char*                     entrypoint,   // entry point of the shader
        const D3D_SHADER_MACRO*         defines,      // defines for the shader
        ID3DInclude*                    include,      // include handler
        UINT                            compileFLags, // compilation flags
        UINT                            effectFlags,  // effect flags
        const D3D11_INPUT_ELEMENT_DESC* layout,       // input layout
        UINT                            elementCount)                            // number of elements in the input layout
    {
        THROWIA_IF(!device || !byteCode, "Device or byteCode is nullptr");
        THROWIA_IF(byteCodeSize == 0, "ByteCode size is 0");

        // Keep an internal blob copy so GetByteCode()/GetByteCodeSize() are valid,
        // matching file-loaded shader behavior.
        {
            ComPtr<ID3DBlob> blob;
            HRESULT hr = D3DCreateBlob(byteCodeSize, blob.GetAddressOf());
            THROWRE_IF(FAILED(hr) || !blob, "Failed to allocate shader bytecode blob");

            std::memcpy(blob->GetBufferPointer(), byteCode, byteCodeSize);
            m_shaderBlob = std::move(blob);
        }

        CreateShader(device, shaderType, byteCode, byteCodeSize);

        if (layout && elementCount > 0)
        {
            m_inputLayout = std::make_unique<InputLayout>();
            m_inputLayout->Initialize(device, layout, elementCount, byteCode, byteCodeSize);
        }

        // defer the shader type until after successful creation
        m_shaderType = shaderType;
        m_Valid      = true;
    }

    /*
    * InitializeFromFile
     *
     * Initializes the shader from a file. The file can be either a .cso or .hlsl file.
     * If the file is a .cso file, it will be loaded directly. If the file is a .hlsl file,
     * it will be compiled using D3DCompileFromFile.
     */
    void Shader::InitializeFromFile(
        ID3D11Device*                   device,
        const std::string&              filePath,
        SHADER_STAGE                    shaderType,
        const char*                     entryPoint,
        const D3D_SHADER_MACRO*         defines,
        ID3DInclude*                    include,
        UINT                            compileFLags,
        UINT                            effectFlags,
        const D3D11_INPUT_ELEMENT_DESC* inputLayout,
        UINT                            elementCount)
    {
        THROWIA_IF(!device, "Device is nullptr");

        // determine the file type
        if (filePath.substr(filePath.find_last_of(".")) == ".cso")
            LoadFromCSO(filePath);
        else if (filePath.substr(filePath.find_last_of(".")) == ".hlsl")
            LoadFromHLSL(
                filePath,
                shaderType,
                entryPoint,
                defines,
                include,
                compileFLags,
                effectFlags,
                inputLayout,
                elementCount);
        else
            THROWRE("Invalid file stage. Only .cso and .hlsl files are supported.");
        // create the shader
        CreateShader(device, shaderType, m_shaderBlob->GetBufferPointer(), m_shaderBlob->GetBufferSize());

        // vertex shaders need an input layout
        if (shaderType & DX::VS && inputLayout && elementCount)
        {
            InitializeInputLayout(
                device,
                inputLayout,
                elementCount,
                m_shaderBlob->GetBufferPointer(),
                m_shaderBlob->GetBufferSize());
        }

        m_shaderType = shaderType;
        NameShader(filePath);
    }

    /// @brief Initializes the input layout for a vertex shader, otherwise this shouldn't be touched
    void Shader::InitializeInputLayout(
        ID3D11Device*                   device,
        const D3D11_INPUT_ELEMENT_DESC* layout,
        UINT                            elementCount,
        const void*                     byteCode,
        size_t                          byteCodeSize)
    {
        THROWIA_IF(!device || !layout || !byteCode, "An argument was nullptr");
        THROWIA_IF(elementCount == 0 || byteCodeSize == 0, "A numeric argument was 0");

        m_inputLayout = std::make_unique<InputLayout>();
        m_inputLayout->Initialize(device, layout, elementCount, byteCode, byteCodeSize);
    }

    /*
    * Bind
     *
     * Binds the shader to the device context. This is used to set the shader for rendering.
     * The input layout is also bound if it is set.
     */
    void Shader::Bind(ID3D11DeviceContext* context) const
    {
        THROWIA_IF(!context, "Context is nullptr");

        switch (m_shaderType)
        {
        case DX::VS:
            context->VSSetShader(m_shaderUnion.vertexShader.Get(), nullptr, 0);
            if (m_inputLayout)
                m_inputLayout->Set(context);
            break;
        case DX::PS:
            context->PSSetShader(m_shaderUnion.pixelShader.Get(), nullptr, 0);
            break;
        case DX::GS:
            context->GSSetShader(m_shaderUnion.geometryShader.Get(), nullptr, 0);
            break;
        case DX::HS:
            context->HSSetShader(m_shaderUnion.hullShader.Get(), nullptr, 0);
            break;
        case DX::DS:
            context->DSSetShader(m_shaderUnion.domainShader.Get(), nullptr, 0);
            break;
        case DX::CS:
            context->CSSetShader(m_shaderUnion.computeShader.Get(), nullptr, 0);
            break;
        default:
            THROWRE("Invalid shader stage");
            break;
        }
    }

    void Shader::UnBind(ID3D11DeviceContext* pcontext) const
    {
        switch (m_shaderType)
        {
        case DX::VS:
            pcontext->VSSetShader(nullptr, nullptr, 0);
            break;
        case DX::PS:
            pcontext->PSSetShader(nullptr, nullptr, 0);
            break;
        case DX::GS:
            pcontext->GSSetShader(nullptr, nullptr, 0);
            break;
        case DX::CS:
            pcontext->CSSetShader(nullptr, nullptr, 0);
            break;
        case DX::DS:
            pcontext->DSSetShader(nullptr, nullptr, 0);
            break;
        case DX::HS:
            pcontext->HSSetShader(nullptr, nullptr, 0);
            break;
        default:
            THROWRE("Shader stage was invalid or NA");
            break;
        }
    }

    // ================================================================================
    // Private functions
    // ================================================================================
    /*
    * LoadFromCSO
     *
     * Loads the shader from a compiled shader (.cso) file. The file is read into a blob and then the shader
     * is created from the blob.
     */
    void Shader::LoadFromCSO(const std::string& filePath)
    {
        // convert to path
        std::filesystem::path path(filePath);
        HRESULT               hr = D3DReadFileToBlob(path.c_str(), m_shaderBlob.GetAddressOf());

        if (FAILED(hr))
        {
            std::string errMsg = "Failed to read .cso shader file: " + std::filesystem::absolute(filePath).string();
            LOG_ERROR(errMsg);
            THROWRE("Failed to read .cso shader file");
        }
    }

    /*
    * LoadFromHLSL
     *
     * Loads the shader from a .hlsl file. The file is compiled using D3DCompileFromFile and then the shader
     * is created from the blob.
     */
    void Shader::LoadFromHLSL(
        const std::string&              filePath,
        SHADER_STAGE                    shaderType,
        const char*                     entryPoint,
        const D3D_SHADER_MACRO*         defines,
        ID3DInclude*                    include,
        UINT                            compileFLags,
        UINT                            effectFlags,
        const D3D11_INPUT_ELEMENT_DESC* layout,
        UINT                            elementCount)
    {
        std::string target;

        switch (shaderType)
        {
        case DX::VS:
            target = "vs_5_0";
            break;
        case DX::PS:
            target = "ps_5_0";
            break;
        case DX::GS:
            target = "gs_5_0";
            break;
        case DX::HS:
            target = "hs_5_0";
            break;
        case DX::DS:
            target = "ds_5_0";
            break;
        case DX::CS:
            target = "cs_5_0";
            break;
        default:
            throw std::runtime_error("Invalid shader stage");
        }
        ComPtr<ID3DBlob> errorBlob;

        std::filesystem::path path(filePath);

        HRESULT hr = D3DCompileFromFile(
            path.c_str(),
            defines,
            include,
            entryPoint,
            target.c_str(),
            compileFLags,
            effectFlags,
            m_shaderBlob.GetAddressOf(),
            errorBlob.GetAddressOf());

        if (FAILED(hr))
            THROWRE("Failed to compile shader from .hlsl file" << errorBlob.Get());

        NameShader(filePath);
        m_shaderType = shaderType;
    }

    void Shader::CreateShader(ID3D11Device* device, SHADER_STAGE type, const void* byteCode, SIZE_T byteCodeLength)
    {
        THROWIA_IF(device == nullptr || byteCode == nullptr, "Device or byteCode is nullptr");
        THROWIA_IF(byteCodeLength == 0, "ByteCode length is 0");

        HRESULT hr = S_OK;
        switch (type)
        {
        case DX::VS:
            hr = device
                     ->CreateVertexShader(byteCode, byteCodeLength, nullptr, m_shaderUnion.vertexShader.GetAddressOf());
            break;
        case DX::PS:
            hr = device->CreatePixelShader(byteCode, byteCodeLength, nullptr, m_shaderUnion.pixelShader.GetAddressOf());
            break;
        case DX::GS:
            hr = device->CreateGeometryShader(
                byteCode,
                byteCodeLength,
                nullptr,
                m_shaderUnion.geometryShader.GetAddressOf());
            break;
        case DX::HS:
            hr = device->CreateHullShader(byteCode, byteCodeLength, nullptr, m_shaderUnion.hullShader.GetAddressOf());
            break;
        case DX::DS:
            hr = device
                     ->CreateDomainShader(byteCode, byteCodeLength, nullptr, m_shaderUnion.domainShader.GetAddressOf());
            break;
        case DX::CS:
            hr = device->CreateComputeShader(
                byteCode,
                byteCodeLength,
                nullptr,
                m_shaderUnion.computeShader.GetAddressOf());
            break;
        default:
            THROWRE("Invalid shader stage");
            break;
        }
        if (FAILED(hr))
            THROWRE("Failed to create shader from bytecode");

        m_shaderType = type;
        m_Valid      = true;
    }

    // set the private data for object viewing
    void Shader::NameShader(const std::string& filePath) const
    {
        const UINT dataSize = static_cast<UINT>(filePath.length()) * sizeof(char);

        auto& s = m_shaderUnion;

        switch (m_shaderType)
        {
        case DX::VS:
            NameDeviceChild(s.vertexShader.Get(), dataSize, filePath.c_str());
            break;
        case DX::PS:
            NameDeviceChild(s.pixelShader.Get(), dataSize, filePath.c_str());
            break;
        case DX::GS:
            NameDeviceChild(s.geometryShader.Get(), dataSize, filePath.c_str());
            break;
        case DX::HS:
            NameDeviceChild(s.hullShader.Get(), dataSize, filePath.c_str());
            break;
        case DX::DS:
            NameDeviceChild(s.domainShader.Get(), dataSize, filePath.c_str());
            break;
        case DX::CS:
            NameDeviceChild(s.computeShader.Get(), dataSize, filePath.c_str());
            break;
        default:
            LOG_ERROR("Failed to name shader from file path: " << filePath.c_str());
            THROWRE("Invalid shader stage");
            break;
        }
    }

} // namespace DX

#undef LOG_TAG
