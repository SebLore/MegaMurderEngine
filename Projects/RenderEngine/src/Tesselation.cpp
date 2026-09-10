#include "Techniques/Tesselation.h"

#include <Utility/Logging.h>

#define LOG_TAG "LODTesselation"

namespace DX
{
    namespace
    {
        void LOG_THROW_IF_INVALID(IUnknown* ptr)
        {
            if (!ptr)
            {
                LOG_ERROR("Invalid COM pointer.");
                throw std::invalid_argument("Invalid COM pointer.");
            }
        }
    } // namespace

    void LODTesselation::Initialize(ID3D11Device* device, TShaders* shaders)
    {
        LOG_THROW_IF_INVALID(device);

        if (shaders)
        {
            m_shaders.Reset(); // reset shaders
            m_shaders = *shaders;
        }

        m_ParamCBuffer.Initialize(device, DX::SHADER_STAGE::HS, &m_params, true);

        LOG_INFO("LODTesselation initialized successfully.");
    }

    void LODTesselation::Initialize(
        ID3D11Device*     device,
        DX::Shader const* vs,
        DX::Shader const* ps,
        DX::Shader const* hs,
        DX::Shader const* ds)
    {
        LOG_THROW_IF_INVALID(device);

        m_shaders.vertexShader = vs;
        m_shaders.pixelShader  = ps;
        m_shaders.hullShader   = hs;
        m_shaders.domainShader = ds;

        m_ParamCBuffer.Initialize(device, DX::SHADER_STAGE::HS, &m_params, true);

        LOG_INFO("LODTesselation initialized successfully.");
    }

    void LODTesselation::Bind(ID3D11DeviceContext* context, DX::SHADER_STAGE stage, UINT slot)
    {
        LOG_THROW_IF_INVALID(context);

        if (m_needUpdate)
        {
            LOG_WARN(
                "LODTesselation parameters need update before binding. "
                "Updating now.");
            m_ParamCBuffer.Update(context, &m_params, sizeof(TessParameters));
            m_needUpdate = false;
        }

        m_ParamCBuffer.Bind(context, DX::SHADER_STAGE::HS, R_LOD_PARAMS);
        m_shaders.Bind(context);
        context->IAGetPrimitiveTopology(&m_lastTopology);
        context->IASetPrimitiveTopology(m_topology);
    }

    void LODTesselation::UnBind(ID3D11DeviceContext* context, DX::SHADER_STAGE stage, UINT slot)
    {
        LOG_THROW_IF_INVALID(context);

        m_ParamCBuffer.UnBind(context, DX::SHADER_STAGE::HS, R_LOD_PARAMS);
        m_shaders.UnBind(context);
        context->IASetPrimitiveTopology(m_lastTopology);
    }

    void LODTesselation::Update(ID3D11DeviceContext* context)
    {
        LOG_THROW_IF_INVALID(context);

        if (m_needUpdate)
        {
            m_ParamCBuffer.Update(context, &m_params, sizeof(TessParameters));
            m_needUpdate = false;
        }
    }

    void DX::LODTesselation::SetParameters(const TessParameters& params)
    {
        m_params     = params;
        m_needUpdate = true;
    }

} // namespace DX
