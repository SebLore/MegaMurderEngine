#pragma once

#include "Common/ShaderStage.h"
#include "ICBuffer.h"
#include "Utils/DebugName.h"

namespace DX
{
    /// Abstraction class for constant buffers.
    class ConstantBuffer : public ICBuffer
    {
      public:
        ConstantBuffer()           = default;
        ~ConstantBuffer() override = default;

        /**
         * @brief Move constructor
         * @param other ConstantBuffer to move from
         */
        ConstantBuffer(ConstantBuffer&& other) noexcept;

        /**
         * @brief Move assignment operator
         * @param other ConstantBuffer to move from
         * @return Reference to this
         */
        ConstantBuffer& operator=(ConstantBuffer&& other) noexcept;

        /// Constructor with no data, no description
        ConstantBuffer(
            ID3D11Device* device,
            SHADER_STAGE  pipelineStage,
            bool          dynamic = true)
            : ConstantBuffer(
                  device,
                  pipelineStage,
                  dynamic,
                  DefaultDesc(dynamic, 0))
        {
        }

        /**
         * @brief Create a cbuffer with no initial data from a description
         * @param device valid device
         * @param pipelineStage stage to default bind to
         * @param dynamic if true will be mappable
         * @param desc D3D11_BUFFER_DESC, use DefaultCbufferDesc if you can.
         */
        ConstantBuffer(
            ID3D11Device*            device,
            SHADER_STAGE             pipelineStage,
            bool                     dynamic,
            const D3D11_BUFFER_DESC& desc);

        /**
         * @brief Create cbuffer with initial data
         * @param device valid device
         * @param pipelineStage stage to default bind to
         * @param bytePtr pointer to initial data
         * @param byteWidth size of data in bytes
         * @param dynamic if true will be mappable
         */
        ConstantBuffer(
            ID3D11Device* device,
            SHADER_STAGE  pipelineStage,
            const void*   bytePtr,
            UINT          byteWidth,
            bool          dynamic = true);

        /**
         * @brief Create cbuffer with initial data and custom description
         * @param device valid device
         * @param pipelineStage stage to default bind to
         * @param bytePtr pointer to initial data
         * @param byteWidth size of data in bytes
         * @param dynamic if true will be mappable
         * @param desc D3D11_BUFFER_DESC, use DefaultCbufferDesc if you can.
         */
        ConstantBuffer(
            ID3D11Device*            device,
            SHADER_STAGE             pipelineStage,
            const void*              bytePtr,
            UINT                     byteWidth,
            bool                     dynamic,
            const D3D11_BUFFER_DESC& desc);

        // Member functions
        //
        // INITIALIZERS

        // no data, no description
        void Initialize(ID3D11Device* dev, SHADER_STAGE ps, bool dyn = true)
        {
            Initialize(dev, ps, dyn, DefaultDesc(dyn, 0));
        }

        // no data, with description
        void Initialize(
            ID3D11Device*            device,
            SHADER_STAGE             pipelineStage,
            bool                     dynamic,
            const D3D11_BUFFER_DESC& desc) override;

        // data, no description
        void Initialize(
            ID3D11Device* dev,
            SHADER_STAGE  ps,
            const void*   ptr,
            UINT          bw,
            bool          dyn = true)
        {
            Initialize(dev, ps, ptr, bw, dyn, DefaultDesc(dyn, bw));
        }

        // data and description
        void Initialize(
            ID3D11Device*            device,
            SHADER_STAGE             pipelineStage,
            const void*              bytePtr,
            UINT                     byteWidth,
            bool                     dynamic,
            const D3D11_BUFFER_DESC& desc) override;

        bool IsInitialized() const override;
        void Reset() override;

        /**
         * @brief Update the constant buffer data
         * @param m_context device context for mapping/updating
         * @param data pointer to new data
         * @param size size of data in bytes
         */
        void Update(
            ID3D11DeviceContext* m_context,
            const void*          data,
            UINT                 size) override;

        /**
         * @brief Bind constant buffer to pipeline stage
         * @param m_context device context for binding
         * @param stage shader stage to bind to (VS, PS, etc.)
         * @param startSlot buffer slot to bind to
         */
        void Bind(
            ID3D11DeviceContext* m_context,
            SHADER_STAGE         stage     = VS,
            UINT                 startSlot = 0) const override;

        /**
         * @brief Unbind constant buffer from pipeline stage
         * @param m_context device context for unbinding
         * @param stage shader stage to unbind from (VS, PS, etc.)
         * @param startSlot buffer slot to unbind from
         */
        void UnBind(
            ID3D11DeviceContext* m_context,
            SHADER_STAGE         stage     = VS,
            UINT                 startSlot = 0) const override;

        /**
         * @brief Get the underlying D3D11 buffer
         * @return pointer to ID3D11Buffer
         */
        ID3D11Buffer* GetBuffer() const;

        SHADER_STAGE GetPipelineStage() const noexcept override
        {
            return m_PipelineStage;
        }

        void SetPipelineStage(SHADER_STAGE newStage) override
        {
            m_PipelineStage = newStage;
        }

        /**
         * @brief Set debug object name for graphics debugging tools
         * @param name null-terminated string name
         */
        void SetDebugObjectName(const char* name) override;

        /**
         * @brief Set debug object name for graphics debugging tools (wide string)
         * @param name null-terminated wide string name
         */
        void SetDebugObjectNameW(const wchar_t* name) override;

        static constexpr D3D11_BUFFER_DESC
        DefaultDesc(bool dynamic = true, UINT byteWidth = 0)
        {
            return DefaultCbufferDesc(dynamic, byteWidth);
        }

        void SetDebugName(const std::string& name)
        {
            Debug::SetDebugName(m_Buffer, name.c_str());
        }

        // no copying
        ConstantBuffer(const ConstantBuffer&)            = delete;
        ConstantBuffer& operator=(const ConstantBuffer&) = delete;

      private:
        ComPtr<ID3D11Buffer> m_Buffer; ///< Smart pointer holding the buffer COM object

        /// What pipeline stage to bind to when no argument is passed to Bind()
        SHADER_STAGE m_PipelineStage = VS;
    };
} // namespace DX
