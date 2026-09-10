/*
* InputLayout.h
*
* Abstraction class for D3D11 input layout.
*/
#pragma once

#include <stdexcept>
#include <string>

#include <d3d11.h>
#include <wrl/client.h>

#include <Utility/Errorhandling.h>
#include "Utils/DebugName.h"

using Microsoft::WRL::ComPtr;

namespace DX
{
    /// Abstraction class for ID3D11InputLayout. Has features to pair with a specific vertex shader
    class InputLayout
    {
      public:
        InputLayout()  = default;
        ~InputLayout() = default;
        /// move constructor
        InputLayout(InputLayout&& right) noexcept
        {
            m_layout       = std::move(right.m_layout);
            m_layoutDesc   = right.m_layoutDesc;
            m_elementCount = right.m_elementCount;
            m_vshaderName  = std::move(right.m_vshaderName);

            right.m_layout       = nullptr;
            right.m_layoutDesc   = nullptr;
            right.m_elementCount = 0;
            right.m_vshaderName  = L"";
        }
        /// move operator
        InputLayout& operator=(InputLayout&& right) noexcept
        {
            if (this != &right)
            {
                m_layout       = std::move(right.m_layout);
                m_layoutDesc   = right.m_layoutDesc;
                m_elementCount = right.m_elementCount;
                m_vshaderName  = std::move(right.m_vshaderName);

                right.m_layout       = nullptr;
                right.m_layoutDesc   = nullptr;
                right.m_elementCount = 0;
                right.m_vshaderName  = L"";
            }
            return *this;
        }

        /**
         * @brief Constructor for Input Layout.
         * @param device valid device
         * @param layout pointer to input element desc (array)
         * @param numElements number of elements in layout
         * @param bytecode pointer to vertex shader's byte code generated at compilation time.
         * @param bytecodeSize size of the shader byte code
         * @param vshaderName name of the shader this is being paired with
         */
        InputLayout(
            ID3D11Device*                   device,
            const D3D11_INPUT_ELEMENT_DESC* layout,
            UINT                            numElements,
            const void*                     bytecode,
            size_t                          bytecodeSize,
            const wchar_t*                  vshaderName)
            : m_layoutDesc(layout), m_elementCount(numElements),
              m_vshaderName(vshaderName)
        {
            Initialize(
                device,
                m_layoutDesc,
                m_elementCount,
                bytecode,
                bytecodeSize,
                m_vshaderName.c_str());
        }

        /**
         * @brief Initializes default constructed InputLayout.
         * @param device valid device
         * @param layout pointer to input element desc (array)
         * @param numElements number of elements in layout
         * @param bytecode pointer to vertex shader's byte code generated at compilation time.
         * @param bytecodeSize size of the shader byte code
         * @param vshaderName name of the shader this is being paired with, optional
         * @throws invalid_argument if device, layout and/or bytecode is nullptr
         * @throws invalid_argument if number of elements or bytecodeSize is 0.
         * @throws runtime_error if object creation failed
         */
        void Initialize(
            ID3D11Device*                   device,
            const D3D11_INPUT_ELEMENT_DESC* layout,
            UINT                            numElements,
            const void*                     bytecode,
            size_t                          bytecodeSize,
            const wchar_t*                  vshaderName = nullptr)
        {
            THROWIA_IF(
                !device || !layout || !bytecode,
                "An argument was nullptr");
            THROWIA_IF(
                numElements == 0 || bytecodeSize == 0,
                "A numeric argument was 0");

// suppress warning about possible arithmetic overflow
#pragma warning(suppress : 6387)

            // create layout
            HRESULT hr = device->CreateInputLayout(
                layout,
                numElements,
                bytecode,
                bytecodeSize,
                m_layout.GetAddressOf());

            // store name of shader if it was given
            if (vshaderName && m_vshaderName.empty())
                m_vshaderName = std::wstring(vshaderName);

            // throw if failed
            if (FAILED(hr))
                THROWRE("Failed to create input layout");
        }

        /// set input layout
        void Set(ID3D11DeviceContext* context) const
        {
            if (m_layout)
                context->IASetInputLayout(m_layout.Get());
        }
        /// set name of underlying shader
        void SetShaderName(const wchar_t* name)
        {
            m_vshaderName = std::wstring(name);
        }

        /// Get pointer to input layout object
        const ID3D11InputLayout* GetLayout() const { return m_layout.Get(); }

        /// get number of elements in element desc
        constexpr UINT GetElementCount() const noexcept
        {
            return m_elementCount;
        }

        /// get pointer to element desc
        const D3D11_INPUT_ELEMENT_DESC* GetElementDesc() const
        {
            return m_layoutDesc;
        }

        void SetDebugName(const std::string& name)
        {
            Debug::SetDebugName(m_layout, name.c_str());
        }

        // no copying
        InputLayout(const InputLayout&)            = delete;
        InputLayout& operator=(const InputLayout&) = delete;

      private:
        ComPtr<ID3D11InputLayout>       m_layout;                   ///< COM object
        const D3D11_INPUT_ELEMENT_DESC* m_layoutDesc   = nullptr; ///< pointer to external input element desc array
        UINT                            m_elementCount = 0;       ///< number of entries in input element desc array
        std::wstring                    m_vshaderName  = L"";     ///< file name of paired vertex shader (optional)
    };
} // namespace DX
