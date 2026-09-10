#pragma once

#include <array>
#include <memory>
#include <string>

#include <Abstraction/ConstantBuffer.h>
#include <Abstraction/Shader.h>
#include <Common/RegisterConstants.h>

namespace DX
{
    struct TessParameters
    {
        float minTessellationDistance = 1.0f;
        float maxTessellationDistance = 10.0f;
        float minTesselationFactor    = 1.0f;
        float maxTesselationFactor    = 8.0f;
    };

    class LODTesselation
    {
      public:
        struct TShaders
        {
            std::array<std::string, 4> shaderFilePaths = {};
            const Shader*              hullShader      = nullptr;
            const Shader*              domainShader    = nullptr;
            const Shader*              vertexShader    = nullptr;
            const Shader*              pixelShader     = nullptr;

            constexpr TShaders() = default;

            TShaders(const TShaders& cpy)
                : shaderFilePaths(cpy.shaderFilePaths), hullShader(cpy.hullShader), domainShader(cpy.domainShader),
                  vertexShader(cpy.vertexShader), pixelShader(cpy.pixelShader)
            {
            }

            const TShaders& operator=(const TShaders& cpy)
            {
                if (this != &cpy)
                {
                    shaderFilePaths = cpy.shaderFilePaths;
                    hullShader      = cpy.hullShader;
                    domainShader    = cpy.domainShader;
                    vertexShader    = cpy.vertexShader;
                    pixelShader     = cpy.pixelShader;
                }
                return *this;
            }

            TShaders(TShaders&& mov) noexcept
                : shaderFilePaths(std::move(mov.shaderFilePaths)), hullShader(std::move(mov.hullShader)),
                  domainShader(std::move(mov.domainShader)), vertexShader(std::move(mov.vertexShader)),
                  pixelShader(std::move(mov.pixelShader))
            {
            }

            TShaders& operator=(TShaders&& mov) noexcept
            {
                if (this != &mov)
                {
                    shaderFilePaths = std::move(mov.shaderFilePaths);
                    hullShader      = std::move(mov.hullShader);
                    domainShader    = std::move(mov.domainShader);
                    vertexShader    = std::move(mov.vertexShader);
                    pixelShader     = std::move(mov.pixelShader);
                }
                return *this;
            }

            void Bind(ID3D11DeviceContext* context) const
            {
                if (hullShader)
                    hullShader->Bind(context);
                if (domainShader)
                    domainShader->Bind(context);
                if (vertexShader)
                    vertexShader->Bind(context);
                if (pixelShader)
                    pixelShader->Bind(context);
            }

            void UnBind(ID3D11DeviceContext* context) const
            {
                if (hullShader)
                    hullShader->UnBind(context);
                if (domainShader)
                    domainShader->UnBind(context);
                if (vertexShader)
                    vertexShader->UnBind(context);
                if (pixelShader)
                    pixelShader->UnBind(context);
            }

            void Reset()
            {
                hullShader   = nullptr;
                domainShader = nullptr;
                vertexShader = nullptr;
                pixelShader  = nullptr;
            }
        };

      public:
        LODTesselation()                            = default;
        ~LODTesselation()                           = default;
        LODTesselation(LODTesselation&&)            = default;
        LODTesselation& operator=(LODTesselation&&) = default;
        LODTesselation(ID3D11Device* device, TShaders* shaders = nullptr) { Initialize(device, shaders); }
        LODTesselation(
            ID3D11Device*     device,
            DX::Shader const* vs,
            DX::Shader const* ps,
            DX::Shader const* hs,
            DX::Shader const* ds)
        {
            Initialize(device, vs, ps, hs, ds);
        }

        void Initialize(ID3D11Device* device, TShaders* shaders = nullptr);
        void Initialize(
            ID3D11Device*     device,
            DX::Shader const* vs,
            DX::Shader const* ps,
            DX::Shader const* hs,
            DX::Shader const* ds);
        void Bind(ID3D11DeviceContext* context, DX::SHADER_STAGE stage = DX::HS, UINT slot = R_LOD_PARAMS);
        void UnBind(ID3D11DeviceContext* context, DX::SHADER_STAGE stage = DX::HS, UINT slot = R_LOD_PARAMS);

        void            Update(ID3D11DeviceContext* context);
        void            SetParameters(const TessParameters& params);
        TessParameters& GetParameters()
        {
            m_needUpdate = true;
            return m_params;
        }
        const TessParameters& GetParameters() const { return m_params; }

        TShaders&       GetShaders() { return m_shaders; }
        const TShaders& GetShaders() const { return m_shaders; }

        constexpr D3D11_PRIMITIVE_TOPOLOGY GetTopology() const { return m_topology; }
        void                               SetTopology(D3D11_PRIMITIVE_TOPOLOGY topology) { m_topology = topology; }

        // no copying
        LODTesselation(const LODTesselation&)            = delete;
        LODTesselation& operator=(const LODTesselation&) = delete;

      private:
        TessParameters               m_params;
        DX::CBufferT<TessParameters> m_ParamCBuffer;
        TShaders                     m_shaders;

        // Current topology for tessellation
        D3D11_PRIMITIVE_TOPOLOGY m_topology     = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
        D3D11_PRIMITIVE_TOPOLOGY m_lastTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

        // Flag to indicate if an update is needed after calling SetParameters
        bool m_needUpdate = true;
    };
} // namespace DX
