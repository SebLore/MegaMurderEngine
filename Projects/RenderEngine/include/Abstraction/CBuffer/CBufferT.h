#pragma once

#include "ICBuffer.h"

#include <Utility/ErrorHandling.h>
#include <Utility/Logging.h>
#include "Utils/DebugName.h"
#include <cassert>

#define LOG_TAG "CBufferT"

namespace DX
{

    /// @brief Template class for a typed constant buffer. Automatically sets
    /// the size of the buffer based on the type T.
    /// @tparam T 16-byte-aligned data struct up to 256 bytes
    template <typename T>
    class CBufferT : public ICBuffer
    {
        static_assert(
            (sizeof(T) % 16) == 0,
            "CBufferT<T>: sizeof(T) must be multiple of 16 for D3D11 constant buffers");

      public:
        CBufferT()  = default;
        ~CBufferT() = default;

        /**
         * @brief Move constructor
         * @param other CBufferT to move from
         */
        CBufferT(CBufferT&& other) noexcept;

        /**
         * @brief Move assignment operator
         * @param other CBufferT to move from
         * @return Reference to this
         */
        CBufferT& operator=(CBufferT&& other) noexcept;

        /**
         * @brief Initializes an empty cbuffer with default stages as either dynamic or default
         * @param device valid device pointer
         * @param stage Which stage to bind to on default Bind command
         * @param dynamic if true sets D3D11_USAGE_DYNAMIC and CPU_ACCESS_WRITE_FLAGS, else D3D11_USAGE_DEFAULT and 0, respectively
         */
        CBufferT(ID3D11Device* device, SHADER_STAGE stage, bool dynamic = true);

        /**
         * @brief Initialize an empty cbuffer from a description
         * @param device valid device pointer
         * @param stage shader stage to bind to on default Bind command
         * @param dynamic if true will be mappable
         * @param desc D3D11_BUFFER_DESC
         */
        CBufferT(ID3D11Device* device, SHADER_STAGE stage, bool dynamic, const D3D11_BUFFER_DESC& desc);

        /**
         * @brief Initialize constant buffer with initial data and default settings
         * @param device valid device pointer
         * @param stage shader stage to bind to on default Bind command
         * @param bytePtr const void * to data
         * @param dynamic if true will be mappable
         */
        CBufferT(ID3D11Device* device, SHADER_STAGE stage, const void* bytePtr, bool dynamic = true);

        /**
         * @brief Initialize constant buffer with some initial data from a buffer description
         * @param device valid device pointer
         * @param stage shader stage to bind to on default Bind command
         * @param bytePtr pointer to initial data
         * @param dynamic if true will be mappable
         * @param desc D3D11_BUFFER_DESC
         */
        CBufferT(
            ID3D11Device*            device,
            SHADER_STAGE             stage,
            const void*              bytePtr,
            bool                     dynamic,
            const D3D11_BUFFER_DESC& desc);

        void Initialize(ID3D11Device* dev, SHADER_STAGE ps, bool dyn = true);
        void Initialize(ID3D11Device* device, SHADER_STAGE stage, bool dynamic, const D3D11_BUFFER_DESC& desc) override;
        void Initialize(ID3D11Device* dev, SHADER_STAGE ps, const void* ptr, bool dyn = true);

        /**
         * @brief Initializes buffer with some initial data and a user-set description
         * @param device valid device pointer
         * @param stage shader stage to bind to
         * @param bytePtr pointer to initial data
         * @param dynamic if true will be mappable
         * @param desc D3D11_BUFFER_DESC
         */
        void Initialize(
            ID3D11Device* device,
            SHADER_STAGE  stage,
            const void*   bytePtr,
            UINT,
            bool                     dynamic,
            const D3D11_BUFFER_DESC& desc) override;

        /**
         * @brief Returns true if the internal ComPtr is not nullptr
         * @return true if initialized, false otherwise
         */
        bool IsInitialized() const override;

        /**
         * @brief Reset the buffer, requiring it to be initialized again to be used
         */
        void Reset() override;

        /**
         * @brief Updates the data on the GPU side, using Map/Unmap if dynamic and UpdateSubresource if not
         * @param context device context for updating
         * @param data pointer to new data
         * @param size size of data in bytes (default is BUFFER_SIZE)
         */
        void Update(ID3D11DeviceContext* context, const void* data, UINT size = BUFFER_SIZE) override;

        /**
         * @brief Binds the buffer to a shader and slot
         * @param context Valid device context ptr
         * @param stage Stage to bind to. Default is NA which uses own held m_PipelineStage
         * @param startSlot cbuffer (b#) slot to bind to in the chosen shader stage
         */
        void Bind(ID3D11DeviceContext* context, SHADER_STAGE stage = NA, UINT startSlot = 0) const override;

        /**
         * @brief Sets the selected cbuffer slot in the selected shader stage to nullptr
         * @param context device context for unbinding
         * @param stage shader stage to unbind from. Default is NA which uses own held m_PipelineStage
         * @param startSlot cbuffer slot to unbind from
         */
        void UnBind(ID3D11DeviceContext* context, SHADER_STAGE stage = NA, UINT startSlot = 0) const override;

        /**
         * @brief Sets the buffer's debug name for debugging with things like RenderDoc, using regular width string
         * @param name null-terminated string name
         */
        void SetDebugObjectName(const char* name) override;

        /**
         * @brief Sets the buffer's name for debugging with things like RenderDoc, using wide string
         * @param name null-terminated wide string name
         */
        void SetDebugObjectNameW(const wchar_t* name) override;

        SHADER_STAGE GetPipelineStage() const noexcept override { return m_PipelineStage; }
        void         SetPipelineStage(SHADER_STAGE newStage) override { m_PipelineStage = newStage; }

        /**
         * @brief Get the byte size of the buffer
         * @return size in bytes (sizeof(T))
         */
        constexpr UINT GetByteSize() const noexcept { return BUFFER_SIZE; }

        void SetDebugName(const std::string& name)
        {
            Debug::SetDebugName(m_Buffer, name.c_str());
        }

      public:
        CBufferT(const CBufferT&)            = delete;
        CBufferT& operator=(const CBufferT&) = delete;

        /**
         * @brief Returns a D3D11_BUFFER_DESC with default settings, ByteWidth = sizeof(T), and either DYNAMIC + CPU_WRITE or DEFAULT + NULL flags if dynamic
         * @param dynamic if true sets DYNAMIC usage and CPU_WRITE access, else DEFAULT usage
         * @return D3D11_BUFFER_DESC configured for constant buffer usage
         */
        static constexpr D3D11_BUFFER_DESC DefaultDesc(bool dynamic = true);

      private:
        ComPtr<ID3D11Buffer>  m_Buffer;
        SHADER_STAGE          m_PipelineStage = VS;
        UINT                  m_Slot          = 0;
        static constexpr UINT BUFFER_SIZE     = sizeof(T);
        bool                  m_IsDynamic     = false;
    };

    // ============================================================================================
    // INLINE DEFINES
    // ============================================================================================

    // -- constructors & operators
    // ----------------------------------------------------------------

    template <typename T>
    CBufferT<T>::CBufferT(CBufferT&& other) noexcept
        : m_Buffer(std::move(other.m_Buffer)), m_PipelineStage(std::move(other.m_PipelineStage))
    {
    }

    template <typename T>
    CBufferT<T>& CBufferT<T>::operator=(CBufferT<T>&& other) noexcept
    {
        if (this != &other)
        {
            m_Buffer              = std::move(other.m_Buffer);
            m_PipelineStage       = other.m_PipelineStage;
            other.m_PipelineStage = VS;
        }
        return *this;
    }

    template <typename T>
    CBufferT<T>::CBufferT(ID3D11Device* device, SHADER_STAGE stage, bool dynamic)
        : CBufferT(device, stage, dynamic, DefaultDesc(dynamic))
    {
    }

    template <typename T>
    CBufferT<T>::CBufferT(ID3D11Device* device, SHADER_STAGE stage, bool dynamic, const D3D11_BUFFER_DESC& desc)
    {
        CBufferT<T>::Initialize(device, stage, dynamic, desc);
    }

    template <typename T>
    CBufferT<T>::CBufferT(ID3D11Device* device, SHADER_STAGE stage, const void* bytePtr, bool dynamic)
    {
        Initialize(device, stage, bytePtr, dynamic);
    }

    template <typename T>
    CBufferT<T>::CBufferT(
        ID3D11Device*            device,
        SHADER_STAGE             stage,
        const void*              bytePtr,
        bool                     dynamic,
        const D3D11_BUFFER_DESC& desc)
    {
        Initialize(device, stage, bytePtr, dynamic, desc);
    }

    template <typename T>
    void CBufferT<T>::Initialize(ID3D11Device* dev, SHADER_STAGE ps, bool dyn)
    {
        Initialize(dev, ps, dyn, DefaultDesc(dyn));
    }

    template <typename T>
    void CBufferT<T>::Initialize(ID3D11Device* dev, SHADER_STAGE ps, const void* ptr, bool dyn)
    {
        Initialize(dev, ps, ptr, 0, dyn, DefaultDesc(dyn));
    }

    template <typename T>
    void CBufferT<T>::Initialize(
        ID3D11Device* device,
        SHADER_STAGE  stage,
        const void*   bytePtr,
        UINT,
        bool                     dynamic,
        const D3D11_BUFFER_DESC& desc)
    {
        THROWIA_IF(!device, "Device is nullptr");

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem                = bytePtr;
        initData.SysMemPitch            = 0;

        HRESULT hr = device->CreateBuffer(&desc, &initData, m_Buffer.GetAddressOf());
        THROWRE_IF(FAILED(hr), "Failed to create constant buffer. HR:" << hr);

        m_PipelineStage = stage;
        m_IsDynamic     = dynamic;
    }

    template <typename T>
    void CBufferT<T>::Initialize(ID3D11Device* device, SHADER_STAGE stage, bool dynamic, const D3D11_BUFFER_DESC& desc)
    {
        THROWIA_IF(!device, "Device is nullptr");

        HRESULT hr = device->CreateBuffer(&desc, nullptr, m_Buffer.GetAddressOf());
        THROWRE_IF(FAILED(hr), "Failed to create constant buffer. HR:" << hr);

        m_PipelineStage = stage;
        m_IsDynamic     = dynamic;
    }

    template <typename T>
    bool CBufferT<T>::IsInitialized() const
    {
        return m_Buffer != nullptr;
    }

    template <typename T>
    void CBufferT<T>::Reset()
    {
        m_Buffer.Reset();
        m_PipelineStage = VS;
    }

    template <typename T>
    void CBufferT<T>::Bind(ID3D11DeviceContext* context, SHADER_STAGE stage, UINT startSlot) const
    {
        THROWIA_IF(!context, "Context is nullptr");
        THROWRE_IF(!m_Buffer, "ID3D11Buffer m_Buffer is nullptr");

        if (stage == NA)
            stage = m_PipelineStage;

        auto buf = m_Buffer.GetAddressOf();
        BindConstantBuffer(context, startSlot, stage, buf, 1);
    }

    template <typename T>
    void CBufferT<T>::UnBind(ID3D11DeviceContext* context, SHADER_STAGE stage, UINT startSlot) const
    {
        THROWIA_IF(!context, "Context is nullptr");
        if (stage == NA)
            stage = m_PipelineStage;
        UnBindConstantBuffer(context, startSlot, stage, 1);
    }

    template <typename T>
    void CBufferT<T>::Update(ID3D11DeviceContext* context, const void* data, UINT size)
    {
        THROWIA_IF(!context, "Context is nullptr");
        if (!IsInitialized())
        {
            LOG_WARN_N("Attempting to update data of uninitialized buffer", 5);
            return;
        }

        if (size > BUFFER_SIZE)
        {
            LOG_ERROR("Size was " << size << " but buffer size is " << BUFFER_SIZE);
            THROWRE("Incompatible size");
        }

        if (m_IsDynamic)
        {
            D3D11_MAPPED_SUBRESOURCE mappedResource = {};
            HRESULT                  hr = context->Map(m_Buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
            if (FAILED(hr))
                THROWRE("Failed to map constant buffer");

            std::memcpy(mappedResource.pData, data, size);
            context->Unmap(m_Buffer.Get(), 0);
        }
        else
        {
            // with default usage we expect not to update every frame and
            // mapping is not allowed, so we use UpdateSubresource
            context->UpdateSubresource(m_Buffer.Get(), 0, nullptr, data, 0, 0);
        }
    }

    template <typename T>
    void CBufferT<T>::SetDebugObjectName(const char* name)
    {
        if (name && IsInitialized())
            m_Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)) * sizeof(char), name);
    }

    template <typename T>
    void CBufferT<T>::SetDebugObjectNameW(const wchar_t* name)
    {
        if (name && IsInitialized())
            m_Buffer->SetPrivateData(
                WKPDID_D3DDebugObjectNameW,
                static_cast<UINT>(wcslen(name) * sizeof(wchar_t)),
                name);
    }

    template <typename T>
    constexpr D3D11_BUFFER_DESC CBufferT<T>::DefaultDesc(bool dynamic)
    {
        return DefaultCbufferDesc(dynamic, sizeof(T));
    }

} // namespace DX

#undef LOG_TAG
