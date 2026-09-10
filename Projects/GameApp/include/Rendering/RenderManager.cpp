#include "RenderManager.h"

#include "Rework/VertexLayout.h"
#include "Utils/DebugName.h"

#include "Core/Math/Math.h"

#include <Utility/Logging.h>

#define LOG_TAG "RenderManager"

// STL
#include <array>
#include <cmath>
#include <unordered_map>
#include <vector>

#include "Common/Render/DrawList.h"

namespace Murder::Render
{
    namespace
    {
        using namespace Murder;

        DX::SHADER_STAGE ToDxShaderStage(ShaderStage stage)
        {
            switch (stage)
            {
            case ShaderStage::Vertex:
                return DX::SHADER_STAGE::VS;
            case ShaderStage::Pixel:
                return DX::SHADER_STAGE::PS;
            case ShaderStage::Geometry:
                return DX::SHADER_STAGE::GS;
            case ShaderStage::Hull:
                return DX::SHADER_STAGE::HS;
            case ShaderStage::Domain:
                return DX::SHADER_STAGE::DS;
            case ShaderStage::Compute:
                return DX::SHADER_STAGE::CS;
            default:
                LOG_ERROR("Unknown shader stage" << static_cast<int>(stage));
                return DX::SHADER_STAGE::NA;
            }
        }
    } // namespace

    RenderManager::RenderManager(HWND hwnd, UINT w, UINT h)
        : m_RenderEngine(hwnd, w, h), m_RenderBackend(Device(), Context()), m_Engine2D(m_RenderEngine),
          m_DebugRenderer(Device(), Context())
#ifdef USING_IMGUI
          ,
          m_Imgui(hwnd, Device(), Context())
#endif
    {

        // frame buffer for settings
        m_FrameBuffer.Initialize(Device(), DX::PS, &m_FrameData, true);

        // initialize buffers TODO: move these into a FrameResources or something
        m_CameraCB.Initialize(Device(), DX::VS);
        m_TransformCB.Initialize(Device(), DX::VS);
        m_MaterialCB.Initialize(Device(), DX::PS);
        m_ShadowCB.Initialize(Device(), DX::PS);

        m_LightCollectionCB.Initialize(Device(), DX::PS);

        m_PointLightsSB
            .Initialize(Device(), sizeof(PointLightGpuData), MAX_POINT_LIGHTS_GPU, true, false, true, nullptr);

        m_DirLightsSB.Initialize(
            Device(),
            sizeof(DirectionalLightGpuData),
            MAX_DIRECTIONAL_LIGHTS_GPU,
            true,
            false,
            true,
            nullptr);

        m_SpotLightsSB.Initialize(Device(), sizeof(SpotLightGpuData), MAX_SPOT_LIGHTS_GPU, true, false, true, nullptr);

        m_ShadowMap.Initialize(Device(), 8, 2048, 2048);

        // Debug names
#ifdef _DEBUG
        m_FrameBuffer.SetDebugName("m_FrameBuffer");
        m_CameraCB.SetDebugName("m_RwCameraCB");
        m_TransformCB.SetDebugName("m_RwTransformCB");
        m_MaterialCB.SetDebugName("m_RwMaterialCB");
        m_ShadowCB.SetDebugName("m_RwShadowCB");
        m_LightCollectionCB.SetDebugName("m_RwLightCollectionCB");
        m_PointLightsSB.SetDebugName("m_RwPointLightsSB");
        m_DirLightsSB.SetDebugName("m_RwDirectionalLightsSB");
        m_SpotLightsSB.SetDebugName("m_RwSpotLightsSB");
        m_ShadowMap.SetDebugName("m_RwShadowMap");
#endif
    }

    void RenderManager::ReportLiveObjects() const
    {
#ifdef _DEBUG
        auto* ctx = Context();
        auto* dev = Device();

        if (!ctx || !dev)
            return;

        // Release pipeline-held refs first
        m_RenderEngine.ClearRenderTargets();
        m_RenderEngine.ClearUnorderedAccessViews();
        m_RenderEngine.ClearShaderResourceViews();

        ctx->ClearState();
        ctx->Flush();

        DX::Debug::ReportLiveD3DObjects(dev, true);
        DX::Debug::ReportLiveDXGIObjects();
#endif
    }

    void RenderManager::UploadTestShaders(const ShaderUploadDesc& vsUpload, const ShaderUploadDesc& psUpload)
    {
        if (!m_VS && !vsUpload.byteCode.empty())
        {
            m_VS = LoadManagedShader(vsUpload, nullptr, 0);
            if (m_VS)
                m_VS->SetDebugName("m_RwVS");
        }

        if (!m_PS && !psUpload.byteCode.empty())
        {
            m_PS = LoadManagedShader(psUpload, nullptr, 0);
            if (m_PS)
                m_PS->SetDebugName("m_RwPS");
        }
    }

    std::unique_ptr<DX::Shader> RenderManager::LoadManagedShader(
        ShaderUploadDesc                upload,
        const D3D11_INPUT_ELEMENT_DESC* inputLayout,
        UINT                            inputElementCount) const
    {
        auto shader = std::make_unique<DX::Shader>();
        shader->InitializeFromMemory(
            Device(),
            upload.byteCode.data(),
            upload.byteCode.size_bytes(),
            ToDxShaderStage(upload.shaderStage),
            "main",
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            0,
            0,
            inputLayout,
            inputElementCount);
        return shader;
    }

    // TODO: make this work! Right now shadows are not rendering properly
    void RenderManager::RenderShadowPasses(const DrawList& drawList)
    {
        if (!ShadowsEnabled() || !m_VS || drawList.shadows.empty())
            return;

        auto* ctx = Context();

        m_ShadowMap.UnBindSRV(ctx, DX::PS, R_SHADOW_MAPS);
        m_RenderEngine.SetBlendMode(DX::RenderEngine::BLEND_DISABLED);
        m_RenderEngine.SetDepthMode(DX::RenderEngine::DEPTH_DEFAULT);
        m_RenderEngine.SetTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        D3D11_VIEWPORT oldVp{};
        UINT           oldVpCount = 1;
        ctx->RSGetViewports(&oldVpCount, &oldVp);

        m_ShadowMap.SetViewport(ctx);

        m_VS->Bind(ctx);

        // clear pixel shader for the shadow pass
        ID3D11PixelShader* nullPs = nullptr;
        ctx->PSSetShader(nullPs, nullptr, 0);

        // iterate over all the shadows
        for (const auto& shadow : drawList.shadows)
        {
            m_ShadowMap.Clear(ctx, shadow.shadowSlice);
            m_ShadowMap.SetAsDSV(ctx, shadow.shadowSlice);

            m_CameraCB.Update(ctx, &shadow.camera);
            m_CameraCB.Bind(ctx, DX::VS, R_CAMERA);

            for (const auto& item : drawList.items)
            {
                if (item.transparent || item.viewModel)
                    continue;

                const MeshGpu* mesh = m_RenderBackend.FindMesh(item.meshId);
                if (!mesh)
                    continue;

                ID3D11InputLayout* il = m_RenderBackend.GetOrCreateInputLayout(item.meshId, *mesh, *m_VS);
                if (!il)
                    continue;

                ctx->IASetInputLayout(il);

                m_TransformCB.Update(ctx, &item.transform);
                m_TransformCB.Bind(ctx, DX::VS, R_TRANSFORM);

                m_RenderBackend.BindMesh(*mesh);
                m_RenderBackend.DrawSubMesh(*mesh, item.submeshIndex);
            }

            m_ShadowMap.UnSetDSV(ctx);
        }

        ctx->RSSetState(nullptr);
        ctx->RSSetViewports(1, &oldVp);
    }

    void RenderManager::OnResize(UINT w, UINT h)
    {
        m_RenderEngine.ResizeBackBuffer(w, h);
        m_Engine2D.SetViewportSize(w, h);
    }

    UINT RenderManager::BackBufferWidth() const { return m_RenderEngine.GetBackBufferWidth(); }

    UINT RenderManager::BackBufferHeight() const { return m_RenderEngine.GetBackBufferHeight(); }

    void RenderManager::BeginFrame(float dt)
    {
        m_RenderEngine.BeginFrame();
        m_FrameBuffer.Update(Context(), &m_FrameData);

#ifdef _DEBUG
        DBG_DRAW_BEGIN(dt);
#ifdef USING_IMGUI
        m_Imgui.NewFrame();
#endif
#endif
    }

    void RenderManager::Render(const DrawList& drawList)
    {
        auto* ctx = Context();
        if (!ctx)
            return;

        if (m_VS == nullptr || m_PS == nullptr)
        {
            FinalizeUI(drawList);
            return;
        }

        if (ShadowsEnabled())
            RenderShadowPasses(drawList);

        m_RenderEngine.BindBackBufferRTVAndDSV();
        BindDefaults();

        // Camera
        m_CameraCB.Update(ctx, &drawList.camera);
        m_CameraCB.Bind(ctx, DX::VS, R_CAMERA);

        // Lights
        m_LightCollectionCB.Update(ctx, &drawList.lightCollection);
        m_LightCollectionCB.Bind(ctx, DX::PS, R_LIGHT_COUNT);

        if (!drawList.pointLights.empty())
        {
            m_PointLightsSB.UpdateData(
                ctx,
                drawList.pointLights.data(),
                static_cast<UINT>(drawList.pointLights.size() * sizeof(PointLightGpuData)));
        }
        else
        {
            std::array<PointLightGpuData, 1> dummy{};
            m_PointLightsSB.UpdateData(ctx, dummy.data(), sizeof(PointLightGpuData));
        }
        m_PointLightsSB.BindAsSRV(ctx, DX::PS, R_POINT_LIGHTS);

        if (!drawList.dirLights.empty())
        {
            m_DirLightsSB.UpdateData(
                ctx,
                drawList.dirLights.data(),
                static_cast<UINT>(drawList.dirLights.size() * sizeof(DirectionalLightGpuData)));
        }
        else
        {
            std::array<DirectionalLightGpuData, 1> dummy{};
            m_DirLightsSB.UpdateData(ctx, dummy.data(), sizeof(DirectionalLightGpuData));
        }
        m_DirLightsSB.BindAsSRV(ctx, DX::PS, R_DIR_LIGHTS);

        if (!drawList.spotLights.empty())
        {
            m_SpotLightsSB.UpdateData(
                ctx,
                drawList.spotLights.data(),
                static_cast<UINT>(drawList.spotLights.size() * sizeof(SpotLightGpuData)));
        }
        else
        {
            std::array<SpotLightGpuData, 1> dummy{};
            m_SpotLightsSB.UpdateData(ctx, dummy.data(), sizeof(SpotLightGpuData));
        }
        m_SpotLightsSB.BindAsSRV(ctx, DX::PS, R_SPOT_LIGHTS);

        // Shadow data
        ShadowGpuData shadowData{};
        shadowData.enabled = false;

        for (const auto& shadow : drawList.shadows)
        {
            DirectX::XMMATRIX vp = DirectX::XMLoadFloat4x4(&shadow.camera.viewProj);
            DirectX::XMStoreFloat4x4(&shadowData.lightViewProj, vp);
            shadowData.shadowBias     = shadow.depthBias;
            shadowData.shadowStrength = shadow.strength;

            const float size           = m_ShadowMap.GetShadowViewport().Width;
            shadowData.shadowTexelSize = { 1.0f / std::max(1.0f, size), 1.0f / std::max(1.0f, size) };
            shadowData.enabled         = true;
            break;
        }

        m_ShadowCB.Update(ctx, &shadowData);
        m_ShadowCB.Bind(ctx, DX::PS, R_FRUSTUM);

        m_ShadowMap.BindSRV(ctx, DX::PS, R_SHADOW_MAPS);
        m_ShadowMap.BindSamplerState(ctx, DX::PS, R_SHADOW_SAMPLER);

        // Shaders
        m_VS->Bind(ctx);
        m_PS->Bind(ctx);

        auto drawItems = [&](bool drawViewModel) -> void
        {
            for (const auto& item : drawList.items)
            {

                // leave till last
                if (item.viewModel != drawViewModel)
                    continue;

                const MeshGpu* mesh = m_RenderBackend.FindMesh(item.meshId);
                if (!mesh)
                    continue;

                ID3D11InputLayout* il = m_RenderBackend.GetOrCreateInputLayout(item.meshId, *mesh, *m_VS);
                if (!il)
                    continue;

                ctx->IASetInputLayout(il);

                m_TransformCB.Update(ctx, &item.transform);
                m_TransformCB.Bind(ctx, DX::VS, R_TRANSFORM);

                m_MaterialCB.Update(ctx, &item.materialData);
                m_MaterialCB.Bind(ctx, DX::PS, R_MATERIAL);

                std::shared_ptr<const DX::TextureSRV> tex = m_RenderBackend.GetDefaultTexture();
                if (item.textureId)
                {
                    if (const auto* cached = m_RenderBackend.FindTexture(item.textureId))
                        tex = *cached;
                }
                if (tex)
                    tex->Bind(ctx, DX::PS, R_DIFFUSE_MAP);

                m_RenderBackend.BindMesh(*mesh);
                m_RenderBackend.DrawSubMesh(*mesh, item.submeshIndex);
            }
        };

        drawItems(false);
        m_RenderEngine.ClearDSV();
        drawItems(true);

        UnBindAll();

        // Finish draw list and imgui frame
        FinalizeUI(drawList);
    }

    bool RenderManager::PresentFrame(UINT vsync) const { return !(FAILED(m_RenderEngine.PresentFrame(vsync))); }

    void RenderManager::BindDefaults() const
    {
        m_RenderEngine.SetRasterizerState(DX::RenderEngine::RASTERIZER_DEFAULT);
        m_RenderEngine.SetBlendMode(DX::RenderEngine::BLEND_DISABLED);
        m_RenderEngine.SetDepthMode(DX::RenderEngine::DEPTH_DEFAULT);
        m_RenderEngine.SetTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_RenderEngine.GetDefaultSamplerState().Set(Context(), DX::PS, R_DEFAULT_SAMPLER);
    }

    void RenderManager::UnBindAll() const
    {
        auto ctx = Context();
        // Unbind
        m_TransformCB.UnBind(ctx, DX::VS, R_TRANSFORM);
        m_CameraCB.UnBind(ctx, DX::VS, R_CAMERA);
        m_MaterialCB.UnBind(ctx, DX::PS, R_MATERIAL);
        m_LightCollectionCB.UnBind(ctx, DX::PS, R_LIGHT_COUNT);
        m_ShadowCB.UnBind(ctx, DX::PS, R_FRUSTUM);

        m_PointLightsSB.UnBindSRV(ctx, DX::PS, R_POINT_LIGHTS);
        m_DirLightsSB.UnBindSRV(ctx, DX::PS, R_DIR_LIGHTS);
        m_SpotLightsSB.UnBindSRV(ctx, DX::PS, R_SPOT_LIGHTS);

        m_ShadowMap.UnBindSRV(ctx, DX::PS, R_SHADOW_MAPS);
        m_ShadowMap.UnBindSamplerState(ctx, DX::PS, R_SHADOW_SAMPLER);

        {
            ID3D11ShaderResourceView* nullSrv = nullptr;
            ctx->PSSetShaderResources(R_DIFFUSE_MAP, 1, &nullSrv);
        }

        m_VS->UnBind(ctx);
        m_PS->UnBind(ctx);
    }

    void RenderManager::FinalizeUI(const DrawList& drawList)
    {
        m_Engine2D.BeginBatch();
        if (!drawList.sprites.empty())
            m_Engine2D.DrawSprites(drawList.sprites);
        if (!drawList.textSprites.empty())
            m_Engine2D.DrawSpriteTexts(drawList.textSprites);
        m_Engine2D.EndBatch();

#ifdef _DEBUG
        m_DebugRenderer.EndFrame();
        m_DebugRenderer.Flush(
            DirectX::XMLoadFloat4x4(&drawList.camera.view),
            DirectX::XMLoadFloat4x4(&drawList.camera.proj));
#ifdef USING_IMGUI
        m_Imgui.Render();
#endif
#endif
    }
} // namespace Murder::Render
#undef LOG_TAG
