#pragma once

#include <DebugRenderer.h>

#include <imgui/ImGuiInterface.h>

#include <RenderEngine.h>

#include <Abstraction/StructuredBuffer.h>

#include <Rework/RenderBackend.h>

#include <Techniques/ShadowMap.h>

#include <Engine2D.h>

#include <Common/Render/CameraGpu.h>
#include <Common/Render/LightCollection.h>
#include <Common/Render/MaterialGpu.h>
#include <Common/Render/ShadowGpu.h>
#include <Common/Render/TransformGpu.h>

#include <Common/Scene/RenderWorld.h>

#include "Common/Assets/ShaderStages.h"
#include "Common/Bridge/ShaderUploadDesc.h"
#include "Common/Render/DrawList.h"

#include <memory>

namespace Murder::Render
{
    struct PerFrameData
    {
        float ambiF          = 0.1f;
        bool  shadowsEnabled = true;
        float pad1;
        float pad2;
    };

    class RenderManager
    {
      public:
        struct Settings
        {
            bool useShadowMapping = true;
        };

      public:
        RenderManager(HWND hwnd, UINT w, UINT h);
        ~RenderManager()
        {
            m_RenderBackend.ClearCaches();
#ifdef _DEBUG
            ReportLiveObjects();
#endif
        }

        void OnResize(UINT w, UINT h);
        UINT BackBufferWidth() const;
        UINT BackBufferHeight() const;

        /// Set at the start of every loop. Updates internal frame buffer and prepares debug draw and imgui frame scopes
        void Clear() const { m_RenderEngine.ClearScreen(); }
        void BeginFrame(float dt = 0);
        void BindBackBuffer() const { m_RenderEngine.BindBackBufferRTVAndDSV(); }
        void UnBindBackBuffer() const { m_RenderEngine.UnBindBackBufferAsRTV(); }

        void Render(const DrawList& drawList);

        /// Presents the back buffer to the screen, with optional vsync
        bool PresentFrame(UINT vsync = 0) const;

        bool ShadowsEnabled() const { return m_Settings.useShadowMapping; }
        void EnableShadows(bool enable = true) { m_Settings.useShadowMapping = enable; }

        void SetClearColor(const float rgba[4]) { m_RenderEngine.SetClearColor(rgba[0], rgba[1], rgba[2], rgba[3]); }

        // TODO: move this elsewhere
        RenderBackend&    Backend() { return m_RenderBackend; }
        DX::RenderEngine& Engine() { return m_RenderEngine; }

        // TODO: don't expose here
        ID3D11Device*        Device() const { return m_RenderEngine.GetDevice(); }
        ID3D11DeviceContext* Context() const { return m_RenderEngine.GetContext(); }

        // debug
        void ReportLiveObjects() const;

        void UploadTestShaders(const ShaderUploadDesc& vsUpload, const ShaderUploadDesc& psUpload);

      private:
        // draw ops
        std::unique_ptr<DX::Shader> LoadManagedShader(
            ShaderUploadDesc                upload,
            const D3D11_INPUT_ELEMENT_DESC* inputLayout       = nullptr,
            UINT                            inputElementCount = 0) const;

        void RenderShadowPasses(const DrawList& drawList);
        void BindDefaults() const;
        void FinalizeUI(const DrawList& drawList);
        void UnBindAll() const;

      private:
        using PerFrameBuffer = DX::CBufferT<PerFrameData>;

        // render engine TODO: make only consume and draw
        DX::RenderEngine m_RenderEngine;
        RenderBackend    m_RenderBackend;

        // 2D rendering engine TODO: potentially change later
        Engine2D m_Engine2D;

        // rendering helpers

        std::unique_ptr<DX::Shader> m_VS;
        std::unique_ptr<DX::Shader> m_PS;

        DX::CBufferT<CameraGpuData>    m_CameraCB;
        DX::CBufferT<TransformGpuData> m_TransformCB;
        DX::CBufferT<MaterialGpuData>  m_MaterialCB;
        DX::CBufferT<ShadowGpuData>    m_ShadowCB;

        DX::CBufferT<LightCollectionBufferData> m_LightCollectionCB;
        DX::StructuredBuffer                    m_PointLightsSB;
        DX::StructuredBuffer                    m_DirLightsSB;
        DX::StructuredBuffer                    m_SpotLightsSB;

        DX::ShadowMap m_ShadowMap;

        DX::RenderTarget m_OffscreenRT;

        PerFrameData   m_FrameData = {};
        PerFrameBuffer m_FrameBuffer;

        // TODO: insert debug and imgui rendering here
        DebugRenderer m_DebugRenderer;

#ifdef USING_IMGUI
        ImGuiInterface m_Imgui;
#endif

        Settings m_Settings;
    };

} // namespace Murder::Render
