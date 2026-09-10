/*
 * @file Shader.h
 *
 * Shader abstraction class. Allows for loading shaders from file or memory
 * using a single interface.
 */
#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include <Common/D3D11Headers.h>
#include "Common/ShaderStage.h"
#include "Geometry/Vertices.h"
#include "InputLayout.h"
#include "Utils/DebugName.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib") // for WKPID

namespace DX
{
    // ==| ShaderLoadData |================================================
    /**
     * @brief Data to help define a shader on the CPU
     */
    struct ShaderLoadData
    {
        std::string  filePath; ///< path/file to the shader (.cso or .hlsl)
        SHADER_STAGE                    stage             = VS;
        const D3D11_INPUT_ELEMENT_DESC* inputElementPtr   = nullptr;
        UINT                            inputElementCount = 0;
        const char*                     entryPoint        = "main";
        const D3D_SHADER_MACRO*         defines           = nullptr;
        ID3DInclude* include      = D3D_COMPILE_STANDARD_FILE_INCLUDE;
        UINT         compileFlags = 0;
        UINT         effectFlags  = 0;
    };

    class Shader
    {
      public:
        Shader() = default;
        ~Shader();
        Shader(Shader&& other) noexcept
            : m_inputLayout(std::move(other.m_inputLayout)),
              m_shaderBlob(std::move(other.m_shaderBlob)),
              m_shaderUnion(std::move(other.m_shaderUnion)),
              m_shaderType(std::move(other.m_shaderType))
        {
        }
        Shader& operator=(Shader&& right) noexcept
        {
            if (this != &right)
            {
                m_inputLayout = std::move(right.m_inputLayout);
                m_shaderBlob  = std::move(right.m_shaderBlob);
                m_shaderUnion = std::move(right.m_shaderUnion);
                m_shaderType  = std::move(right.m_shaderType);
            }
            return *this;
        }

        Shader(
            ID3D11Device*      device,
            const std::string& filePath,
            SHADER_STAGE       shaderType,
            // optional arguments
            const char*             entryPoint = "main",
            const D3D_SHADER_MACRO* defines    = nullptr,
            ID3DInclude*            include = D3D_COMPILE_STANDARD_FILE_INCLUDE,
            UINT                    compileFlags         = 0,
            UINT                    effectFlags          = 0,
            const D3D11_INPUT_ELEMENT_DESC* inputLayout  = nullptr,
            UINT                            elementCount = 0);

        Shader(
            ID3D11Device* device,
            const void*   byteCode,     // raw byte code
            size_t        byteCodeSize, // size of the byte code
            SHADER_STAGE  shaderType,   // type of the shader
            // optional arguments
            const char*             entrypoint = "main",
            const D3D_SHADER_MACRO* defines    = nullptr,
            ID3DInclude*            include = D3D_COMPILE_STANDARD_FILE_INCLUDE,
            UINT                    compileFLags         = 0,
            UINT                    effectFlags          = 0,
            const D3D11_INPUT_ELEMENT_DESC* layout       = nullptr,
            UINT                            elementCount = 0);

        // bytecode already loaded (as raw string etc) compile id
        void InitializeFromMemory(
            ID3D11Device* device,
            const void*   byteCode,     // raw byte code
            size_t        byteCodeSize, // size of the byte code
            SHADER_STAGE  shaderType,   // type of the shader
            // optional arguments
            const char*             entrypoint = "main",
            const D3D_SHADER_MACRO* defines    = nullptr,
            ID3DInclude*            include = D3D_COMPILE_STANDARD_FILE_INCLUDE,
            UINT                    compileFLags         = 0,
            UINT                    effectFlags          = 0,
            const D3D11_INPUT_ELEMENT_DESC* layout       = nullptr,
            UINT                            elementCount = 0);

        // needs to be compiled from the .hlsl file
        void InitializeFromFile(
            ID3D11Device*      device,
            const std::string& filePath,
            SHADER_STAGE       shaderType,
            // optional arguments
            const char*             entryPoint = "main",
            const D3D_SHADER_MACRO* defines    = nullptr,
            ID3DInclude*            include = D3D_COMPILE_STANDARD_FILE_INCLUDE,
            UINT                    compileFLags         = 0,
            UINT                    effectFlags          = 0,
            const D3D11_INPUT_ELEMENT_DESC* layout       = nullptr,
            UINT                            elementCount = 0);

        void InitializeInputLayout(
            ID3D11Device*                   device,
            const D3D11_INPUT_ELEMENT_DESC* layout,
            UINT                            numElements,
            const void*                     bytecode,
            size_t                          bytecodeSize);

        void Bind(ID3D11DeviceContext* context) const;
        void UnBind(ID3D11DeviceContext* context) const;
        // Get the shader type
        constexpr SHADER_STAGE GetShaderType() const { return m_shaderType; }

        // Get the shader byte code
        const void* GetByteCode() const
        {
            if (m_shaderBlob)
                return m_shaderBlob->GetBufferPointer();
            return nullptr;
        }

        // Get the size of the shader byte code
        size_t GetByteCodeSize() const
        {
            if (m_shaderBlob)
                return m_shaderBlob->GetBufferSize();
            return 0;
        }

        // Get the input layout
        InputLayout* GetInputLayout() const
        {
            if (m_inputLayout)
                return m_inputLayout.get();
            return nullptr;
        }

        bool Valid()const { return m_Valid; }

        void SetDebugName(const std::string& name)
        {
            switch (m_shaderType)
            {
            case VS: Debug::SetDebugName(m_shaderUnion.vertexShader, name.c_str()); break;
            case PS: Debug::SetDebugName(m_shaderUnion.pixelShader, name.c_str()); break;
            case GS: Debug::SetDebugName(m_shaderUnion.geometryShader, name.c_str()); break;
            case HS: Debug::SetDebugName(m_shaderUnion.hullShader, name.c_str()); break;
            case DS: Debug::SetDebugName(m_shaderUnion.domainShader, name.c_str()); break;
            case CS: Debug::SetDebugName(m_shaderUnion.computeShader, name.c_str()); break;
            default: break;
            }
            if (m_inputLayout)
                m_inputLayout->SetDebugName(name + ".inputLayout");
        }

        // no copying
        Shader(const Shader&)            = delete;
        Shader& operator=(const Shader&) = delete;

      private:
        void LoadFromCSO(const std::string& filePath);
        void LoadFromHLSL(
            const std::string&      filePath,
            SHADER_STAGE            shaderType,
            const char*             entryPoint = "main",
            const D3D_SHADER_MACRO* defines    = nullptr,
            ID3DInclude*            include = D3D_COMPILE_STANDARD_FILE_INCLUDE,
            UINT                    compileFLags         = 0,
            UINT                    effectFlags          = 0,
            const D3D11_INPUT_ELEMENT_DESC* layout       = nullptr,
            UINT                            elementCount = 0);

        /*
         * Creates the shader from the byte code. The shader is created using the appropriate
        * function based on the shader type.
        */
        void CreateShader(
            ID3D11Device* device,
            SHADER_STAGE  type,
            const void* byteCode,
            SIZE_T        byteCodeLength);
        void NameShader(const std::string& filepath) const;

      private:
        // ==| member variables |==
        union ShaderU
        {
            ShaderU() noexcept = default;
            ~ShaderU() {

            };
            //#if 0
            //            {
            //                //vertexShader.Reset();
            //                //pixelShader.Reset();
            //                //geometryShader.Reset();
            //                //hullShader.Reset();
            //                //domainShader.Reset();
            //                //computeShader.Reset();
            //            }
            //#endif
            ShaderU(ShaderU&& other) noexcept
            {
                if (other.vertexShader)
                    vertexShader = std::move(other.vertexShader);
                else if (other.pixelShader)
                    pixelShader = std::move(other.pixelShader);
                else if (other.geometryShader)
                    geometryShader = std::move(other.geometryShader);
                else if (other.hullShader)
                    hullShader = std::move(other.hullShader);
                else if (other.domainShader)
                    domainShader = std::move(other.domainShader);
                else if (other.computeShader)
                    computeShader = std::move(other.computeShader);
            }
            ShaderU& operator=(ShaderU&& other) noexcept
            {
                if (this != &other)
                {
                    vertexShader   = std::move(other.vertexShader);
                    pixelShader    = std::move(other.pixelShader);
                    geometryShader = std::move(other.geometryShader);
                    hullShader     = std::move(other.hullShader);
                    domainShader   = std::move(other.domainShader);
                    computeShader  = std::move(other.computeShader);
                }
                return *this;
            };

            ComPtr<ID3D11VertexShader>   vertexShader = nullptr;
            ComPtr<ID3D11PixelShader>    pixelShader;
            ComPtr<ID3D11GeometryShader> geometryShader;
            ComPtr<ID3D11HullShader>     hullShader;
            ComPtr<ID3D11DomainShader>   domainShader;
            ComPtr<ID3D11ComputeShader>  computeShader;
        } m_shaderUnion;

        std::unique_ptr<InputLayout>
                         m_inputLayout; // input layout for vertex shaders
        ComPtr<ID3DBlob> m_shaderBlob;  // save the compiled shader blob
        SHADER_STAGE     m_shaderType = DX::VS;

        bool m_Valid = false; ///< flag to see if shader has been created and is valid
    };
} // namespace DX
