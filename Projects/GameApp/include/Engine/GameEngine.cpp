#include "pch.h"

#include "GameEngine.h"
#define LOG_TAG "GameEngine"

#include "Game/Game.h"
#include "Config/GameConfig.h"

#include "Game/Components.h"
#include "Rendering/RenderPrep.h"

namespace Murder
{
    GameEngine::GameEngine(HWND handle, UINT cWidth, UINT cHeight, Input& input, unsigned flags)
        : m_Renderer(handle, cWidth, cHeight)
    {
        m_UiViewport.SetSize(static_cast<float>(cWidth), static_cast<float>(cHeight));

        // plug input into ECS, call with ecs.ContextRef<Input>()
        m_ECS.SetContextRef(input);
        input.SetMouseMode(DirectX::Mouse::MODE_ABSOLUTE);

        // keep for now: sprite/HUD workflows in team code still rely on these contexts
        m_ECS.SetContextRef(m_AssetManager);
        m_ECS.SetContextRef(m_RenderWorld);
        m_ECS.SetContextRef(m_InputMode);
        m_ECS.SetContextRef(m_Physics);
        m_ECS.SetContextRef(m_UiViewport);
        m_ECS.SetContextRef(m_GameState);

        auto& cfg = m_ECS.GetOrEmplaceContext<Config::GameConfig>();
        cfg.SetDefaults(); // TODO: load from file later
    }

    GameEngine::~GameEngine()
    {
        //m_Renderer.Cleanup();
        //m_AssetManager.Cleanup();
    }

    bool GameEngine::Initialize()
    {
        m_GameState.stage = Game::GameState::Stage::Loading;
        Game::Configure(m_ECS);

        // update ecs once to run the load system and add the rest of the systems
        m_ECS.Update(0.0f);

        return true;
    }

    void GameEngine::Tick() { m_Clock.Tick(); }

    void GameEngine::Update()
    {
        float dt = Delta();
        m_Renderer.BeginFrame(m_Clock.GetDeltaTime());

        m_ECS.Update(dt);
    }

    void GameEngine::Draw()
    {
        if (!Ready())
            return;

        ShaderUploadDesc vsUpload{};
        ShaderUploadDesc psUpload{};

        const ShaderId vsId = m_AssetManager.RegisterShader("VS_Default.cso");
        const ShaderId psId = m_AssetManager.RegisterShader("PS_Default.cso");

        m_AssetManager.TryGetShaderUpload(vsId, ShaderStage::Vertex, vsUpload);
        m_AssetManager.TryGetShaderUpload(psId, ShaderStage::Pixel, psUpload);

        m_Renderer.UploadTestShaders(vsUpload, psUpload);

        // prepare the frame by making sure data exists
        Render::ResolveSceneResources(m_RenderWorld, m_AssetManager, m_Renderer.Backend());

        // build the draw list by fetching data from the render world and preparing draw calls
        Render::BuildDrawList(m_RenderWorld, m_AssetManager, m_Renderer.Backend(), m_DrawList);

        m_Renderer.Render(m_DrawList);

        if (!m_Renderer.PresentFrame())
            THROWRE("Present back buffer failed");
    }

    void GameEngine::OnResize(UINT width, UINT height)
    {
        m_Renderer.OnResize(width, height);

        m_UiViewport.SetSize(static_cast<float>(width), static_cast<float>(height));
    }

    void GameEngine::SetAppFocused(bool focused)
    {
        m_InputMode.appFocused = focused;
        if (focused)
            SuppressLookThisFrame(); // prevent big jump of the camera
    }

    void GameEngine::SuppressLookThisFrame() { m_InputMode.suppressLookOnce = true; }

} // namespace Murder

#undef LOG_TAG
