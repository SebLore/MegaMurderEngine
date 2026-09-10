#pragma once

#pragma once

#include <Common/D3D11Headers.h>
#include <Utility/ErrorHandling.h>
#include <Utility/Logging.h>
#include "Utils/DebugName.h"

#include "Common/RegisterConstants.h"
#include "Common/ShaderStage.h"

#define LOG_TAG "States"

namespace DX
{
    class SamplerState
    {
      public:
        SamplerState()  = default;
        ~SamplerState() = default;
        /**
         * @brief Constructs a new sampler state with default arguments. Calls Initialize() internally.
         * @param device A valid ID3D11Device pointer
         * @param slot What slot to bind the state to in the pipeline. Defaults to 0.
         * @param desc A D3D11_SAMPLER_DESC structure that describes the sampler state. If not provided, a default description is used.
         */
        SamplerState(ID3D11Device* device, UINT slot = 0, const D3D11_SAMPLER_DESC& desc = DefaultDesc())
        {
            Initialize(device, slot, desc);
        }

        /**
         * @brief Initializes the sampler state. The state can be created without being initialized using the default constructor, but needs
         * to be initialized before it can be used.
         * @param device A valid ID3D11Device pointer
         * @param slot What slot to bind the state to in the pipeline. Defaults to 0.
         * @param desc A D3D11_SAMPLER_DESC structure that describes the sampler state. If not provided, a default description is used.
         */
        void Initialize(ID3D11Device* device, UINT slot = 0, const D3D11_SAMPLER_DESC& desc = DefaultDesc())
        {
            THROWIA_IF(!device, "Device is nullptr");

            HRESULT hr = device->CreateSamplerState(&desc, m_state.GetAddressOf());
            if (FAILED(hr))
                THROWRE("Failed to create sampler state");
            m_slot = slot;
        }

        /**
         * @brief returns a pointer to the raw ID3D11SamplerState object.
         * @return The raw ID3D11SamplerState pointer.
         */
        ID3D11SamplerState* GetState() const { return m_state.Get(); };

        /**
         * @brief Sets the sampler to a pipeline state using its default slot.
         * @param context A valid ID3D11DeviceContext pointer.
         * @param stage The shader stage to set the sampler for. Defaults to PIXEL_SHADER.
         */
        void Set(ID3D11DeviceContext* context, DX::SHADER_STAGE stage = PS, int slot = -1) const
        {
            THROWIA_IF(!context, "Context is nullptr");
            THROWRE_IF(!m_state, "Sampler m_state is nullptr");

            if (slot == -1)
                slot = m_slot;

            switch (stage)
            {
            case VS:
                context->VSSetSamplers(slot, 1, m_state.GetAddressOf());
                break;
            case PS:
                context->PSSetSamplers(slot, 1, m_state.GetAddressOf());
                break;
            case HS:
                context->HSSetSamplers(slot, 1, m_state.GetAddressOf());
                break;
            case DS:
                context->DSSetSamplers(slot, 1, m_state.GetAddressOf());
                break;
            case GS:
                context->GSSetSamplers(slot, 1, m_state.GetAddressOf());
                break;
            case CS:
                context->CSSetSamplers(slot, 1, m_state.GetAddressOf());
                break;
            default:
                LOG_ERROR("SamplerState::Set called with invalid shader stage: " << static_cast<int>(stage));
                break;
            }
        }

        /**
         * @brief Removes the sampler state from the pipeline by setting its slot on the pipeline to a nulled object pointer.
         * @param context A valid ID3D11DeviceContext pointer.
         * @param stage The shader stage to unset the sampler for. Defaults to PIXEL_SHADER.
         */
        void UnSet(ID3D11DeviceContext* context, DX::SHADER_STAGE stage = NA, int slot = -1) const
        {
            THROWIA_IF(!context, "Context is nullptr");

            ID3D11SamplerState* nullSampler = nullptr;
            if (stage == NA)
                stage = m_stage;

            if (slot == -1)
                slot = m_slot;

            switch (stage)
            {
            case VS:
                context->VSSetSamplers(static_cast<UINT>(slot), 1, &nullSampler);
                break;
            case PS:
                context->PSSetSamplers(static_cast<UINT>(slot), 1, &nullSampler);
                break;
            case GS:
                context->GSSetSamplers(static_cast<UINT>(slot), 1, &nullSampler);
                break;
            case HS:
                context->HSSetSamplers(static_cast<UINT>(slot), 1, &nullSampler);
                break;
            case DS:
                context->DSSetSamplers(static_cast<UINT>(slot), 1, &nullSampler);
                break;
            case CS:
                context->CSSetSamplers(static_cast<UINT>(slot), 1, &nullSampler);
                break;
            case NA:
                LOG_ERROR(
                    "SamplerState::UnSet called with NA shader stage, this is "
                    "not allowed.");
                break;
            default:
                LOG_ERROR("SamplerState::UnSet called with invalid shader stage: " << static_cast<int>(stage));
                break;
            }
        }

        /**
         * @brief Sets this sampler's starting slot for every pipeline stage.
         * @param slot a UINT representing the StartSlot parameter in ID3D11DeviceContext::*SSetSamplers
         */
        void SetStartSlot(UINT slot) { m_slot = slot; }

        /**
         * @brief Get the start slot for this sampler state in the target pipeline stage.
         * @return An index of the starting slot.
         */
        UINT GetStartSlot() const noexcept { return m_slot; }

        void SetDebugName(const std::string& name)
        {
            Debug::SetDebugName(m_state, name.c_str());
        }

        /**
         * @brief Gets the sampler description for this sampler state
         * @return a D3D11_SAMPLER_DESC structure with this sampler's description/settings.
         */
        D3D11_SAMPLER_DESC GetDesc() const
        {
            D3D11_SAMPLER_DESC desc = {};
            m_state->GetDesc(&desc);
            return desc;
        }

        /**
         * @brief Creates a sampler description with set default values.
         * @return a D3D11_SAMPLER_DESC structure with default values.
         */
        static D3D11_SAMPLER_DESC DefaultDesc()
        {
            D3D11_SAMPLER_DESC samplerDesc = {};
            samplerDesc.Filter             = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            samplerDesc.AddressU           = D3D11_TEXTURE_ADDRESS_WRAP;
            samplerDesc.AddressV           = D3D11_TEXTURE_ADDRESS_WRAP;
            samplerDesc.AddressW           = D3D11_TEXTURE_ADDRESS_WRAP;
            samplerDesc.ComparisonFunc     = D3D11_COMPARISON_NEVER;
            samplerDesc.MinLOD             = 0;
            samplerDesc.MaxLOD             = D3D11_FLOAT32_MAX;
            return samplerDesc;
        }

      private:
        ComPtr<ID3D11SamplerState> m_state;      // the actual sampler state COM object
        UINT                       m_slot  = 0;  // what slot to bind to
        DX::SHADER_STAGE           m_stage = PS; // default stage to bind to
    };

    /*
    * RasterizerState
    *
    *
    */
    class RasterizerState
    {
      public:
        RasterizerState()  = default;
        ~RasterizerState() = default;

        RasterizerState(ID3D11Device* device, const D3D11_RASTERIZER_DESC& desc = DefaultDesc())
        {
            Initialize(device, desc);
        }

        bool Initialize(ID3D11Device* device, const D3D11_RASTERIZER_DESC& desc = DefaultDesc())
        {
            THROWIA_IF(!device, "Device is nullptr");

            HRESULT hr = device->CreateRasterizerState(&desc, m_state.GetAddressOf());
            return SUCCEEDED(hr);
        }

        ID3D11RasterizerState* GetState() const noexcept { return m_state.Get(); };

        void Set(ID3D11DeviceContext* context) const
        {
            THROWIA_IF(!context, "Context is nullptr");

            THROWRE_IF(!m_state, "Rasterizer m_state is nullptr");

            context->RSSetState(m_state.Get());
        }

        D3D11_RASTERIZER_DESC GetDesc()
        {
            D3D11_RASTERIZER_DESC desc = {};
            m_state->GetDesc(&desc);
            return desc;
        }

        static D3D11_RASTERIZER_DESC DefaultDesc() noexcept
        {
            D3D11_RASTERIZER_DESC rasterizerDesc = {};
            rasterizerDesc.FillMode              = D3D11_FILL_SOLID;
            rasterizerDesc.CullMode              = D3D11_CULL_BACK;
            rasterizerDesc.FrontCounterClockwise = FALSE;
            rasterizerDesc.DepthClipEnable       = TRUE;
            rasterizerDesc.MultisampleEnable     = TRUE;
            rasterizerDesc.AntialiasedLineEnable = TRUE;
            rasterizerDesc.DepthBias             = 0;
            rasterizerDesc.DepthBiasClamp        = 0.0f;
            rasterizerDesc.SlopeScaledDepthBias  = 0.0f;
            return rasterizerDesc;
        }

        void SetDebugName(const std::string& name)
        {
            Debug::SetDebugName(m_state, name.c_str());
        }

      private:
        ComPtr<ID3D11RasterizerState> m_state;
    };

    /*
    * DepthStencilState
    *
    * Abstracts the ID3D11DepthStencilState COM object to make it simpler to create
    * and set. The user can pass a description or it will use a default mode.
    */
    class DepthStencilState
    {
      public:
        DepthStencilState()  = default;
        ~DepthStencilState() = default;
        DepthStencilState(ID3D11Device* device, const D3D11_DEPTH_STENCIL_DESC& desc = DefaultDesc())
        {
            Initialize(device, desc);
        }
        bool Initialize(ID3D11Device* device, const D3D11_DEPTH_STENCIL_DESC& desc = DefaultDesc())
        {
            THROWIA_IF(!device, "Device is nullptr");

            HRESULT hr = device->CreateDepthStencilState(&desc, m_state.GetAddressOf());
            if (FAILED(hr))
                return false;
            return true;
        }
        ID3D11DepthStencilState* GetState() const { return m_state.Get(); };
        void                     Set(ID3D11DeviceContext* context) const
        {
            THROWIA_IF(!context, "Context is nullptr");
            THROWRE_IF(!m_state, "Depth stencil m_state is nullptr");

            context->OMSetDepthStencilState(m_state.Get(), 1);
        }
        static D3D11_DEPTH_STENCIL_DESC DefaultDesc()
        {
            D3D11_DEPTH_STENCIL_DESC desc = {};
            desc.DepthEnable              = TRUE;
            desc.DepthWriteMask           = D3D11_DEPTH_WRITE_MASK_ALL;
            desc.DepthFunc                = D3D11_COMPARISON_LESS;

            desc.StencilEnable                = FALSE; // no stencil by default, enable to use
            desc.StencilReadMask              = 0xFF;
            desc.StencilWriteMask             = 0xFF;
            desc.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
            desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
            desc.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
            desc.FrontFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;
            desc.BackFace                     = desc.FrontFace;
            return desc;
        }

        void SetDebugName(const std::string& name)
        {
            Debug::SetDebugName(m_state, name.c_str());
        }

      private:
        ComPtr<ID3D11DepthStencilState> m_state;
    };

    /*
    * BlendState
    *
    * Abstracts the ID3D11BlendState COM object to make it simpler to create
    * and set. The user can pass a description or it will use a default mode.
    */

    class BlendState
    {
      public:
        BlendState()  = default;
        ~BlendState() = default;
        BlendState(
            ID3D11Device*           device,
            const FLOAT             blendFactor[4] = nullptr,
            UINT                    sampleMask     = 0xFFFFFFFF,
            const D3D11_BLEND_DESC& desc           = DefaultDesc())
        {
            Initialize(device, blendFactor, sampleMask, desc);
        }
        bool Initialize(
            ID3D11Device*           device,
            const FLOAT             blendFactor[4] = nullptr,
            UINT                    sampleMask     = 0xFFFFFFFF,
            const D3D11_BLEND_DESC& desc           = DefaultDesc())
        {
            THROWIA_IF(!device, "Device is nullptr");

            if (blendFactor == nullptr)
                for (int i = 0; i < 4; ++i)
                    m_blendFactor[i] = 1.0f;
            else
                for (int i = 0; i < 4; ++i)
                    m_blendFactor[i] = blendFactor[i];

            HRESULT hr = device->CreateBlendState(&desc, m_state.GetAddressOf());
            if (FAILED(hr))
                return false;

            return true;
        }
        ID3D11BlendState* GetState() const { return m_state.Get(); };

        void Set(ID3D11DeviceContext* context) const
        {
            THROWIA_IF(!context, "Context is nullptr");
            THROWRE_IF(!m_state, "Blend m_state is nullptr");

            context->OMSetBlendState(m_state.Get(), m_blendFactor, m_sampleMask);
        }

        void UnSet(ID3D11DeviceContext* context) const
        {
            THROWIA_IF(!context, "Context is nullptr");
            THROWRE_IF(!m_state, "Blend m_state is nullptr");

            // unbind the blend state
            ID3D11BlendState* nullState = nullptr;
            context->OMSetBlendState(nullState, nullptr, 0xFFFFFFFF);
        }

        void SetBlendFactor(const FLOAT blendFactor[4])
        {
            for (int i = 0; i < 4; ++i)
                m_blendFactor[i] = blendFactor[i];
        }

        void SetSampleMask(UINT sampleMask) { m_sampleMask = sampleMask; }

        UINT GetSampleMask() const { return m_sampleMask; }

        const FLOAT* GetBlendFactor() const { return m_blendFactor; }

        void SetDebugName(const std::string& name)
        {
            Debug::SetDebugName(m_state, name.c_str());
        }

        static constexpr D3D11_BLEND_DESC DefaultDesc()
        {
            D3D11_BLEND_DESC blendDesc                      = {};
            blendDesc.AlphaToCoverageEnable                 = FALSE;
            blendDesc.IndependentBlendEnable                = FALSE;
            blendDesc.RenderTarget[0].BlendEnable           = TRUE;
            blendDesc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
            blendDesc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
            blendDesc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
            blendDesc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ZERO;
            blendDesc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            return blendDesc;
        }

      private:
        ComPtr<ID3D11BlendState> m_state;
        FLOAT                    m_blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        UINT                     m_sampleMask     = 0xFFFFFFFF;
    };
} // namespace DX
#undef LOG_TAG
