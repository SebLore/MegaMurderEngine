#pragma once

#include <Input.h>

#ifdef USING_IMGUI
#include <imgui/ImGuiInterface.h>
#endif

#include <Utility/Clock.h>

#include <AssetManager.h>

#include <Common/Scene/RenderWorld.h>

#include "Game/Components.h"

#include <ECS.hpp>

#include "Common/Render/DrawList.h"
#include "Game/DebugSettings.h"
#include "Game/State/GameState.h"
#include "Rendering/RenderManager.h"

#include "Physics/Physics.h"

namespace Murder
{
    class GameEngine
    {
      public:
        enum class Flags : unsigned
        {
            NONE         = 0,
            START_PAUSED = 1 << 0,
            // add more as needed
        };

      public:
        /// Create an instance of Game Engine, store a reference to input in ECS, set context refs
        explicit GameEngine(HWND handle, UINT cWidth, UINT cHeight, Input& input, unsigned flags = 0);
        ~GameEngine();

        // no copying or moving for now, can add later if needed
        GameEngine(const GameEngine&)            = delete;
        GameEngine& operator=(const GameEngine&) = delete;
        GameEngine(GameEngine&&)                 = delete;
        GameEngine& operator=(GameEngine&&)      = delete;

        /// Initialize the game, ideally from file, systems, ecs, resources etc
        bool Initialize();

        // -- run-time operations --

        /// Update the clock
        void Tick();

        /// Update ECS with delta time
        void Update();

        /// Render the scene
        void Draw();

        /// Decide what happens when window resizes
        void OnResize(UINT width, UINT height);

        void SetAppFocused(bool focused);

        bool IsUiMode() const { return m_InputMode.uiMode; }

        /// Don't adjust camera based on mouse look for 1 frame. Used for when game focuses in after tabbing out and we don't want to spin the camera around by a massive delta
        void SuppressLookThisFrame();

        // -- state queries --
        /// Returns false to stop the game from running
        bool Exiting() const { return m_GameState.IsExiting(); }
        bool Running() const { return m_GameState.IsRunning(); }
        bool Ready() const { return m_GameState.IsReady(); }
        bool Paused() const { return m_GameState.IsPaused(); }

      private:
        // -- timing --
        /// Returns delta time from when Tick was called
        float Delta() const { return m_Clock.GetDeltaTime(); }
        /// Return total time since the clock started ticking
        float Elapsed() const { return m_Clock.GetElapsedTime(); }

      private:
        // TODO: temp for now, move into ECS and Game later

        ECS::ECSManager m_ECS; ///< GameEngine owns ECS

        // AssetManager
        AssetManager m_AssetManager;

        /// Hold
        RenderWorld      m_RenderWorld{};
        Render::DrawList m_DrawList{};
        /// Handle rendering
        Render::RenderManager m_Renderer;

        /// Physics
        Physics m_Physics;

        // Utility, debug, flags
        Clock m_Clock; ///< ticks once per frame and gives out delta time

        Game::InputMode  m_InputMode{};
        Game::UiViewport m_UiViewport{};
        Game::GameState  m_GameState{};
    };
} // namespace Murder
