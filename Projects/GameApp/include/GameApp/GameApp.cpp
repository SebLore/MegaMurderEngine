#include "pch.h"

#include "GameApp.h"

#include <fstream>

#include <Utility/Logging.h>

#define LOG_TAG "GameApp"

bool GameApp::Config::Load(const char* path)
{
    std::ifstream file(path, std::ios::binary);

    if (!file.is_open())
        return false;

    // load config from file
    const std::streamsize size = file.tellg();
    if (size <= 0)
        return false;

    Config out{};
    return true;
}

void GameApp::Run()
{
    const bool initiallyFocused = (::GetForegroundWindow() == m_Window.GetHandle());
    m_State.appFocused          = initiallyFocused;
    m_Game.SetAppFocused(initiallyFocused);

    if (!m_Game.Initialize())
    {
        LOG_ERROR("Failed to initialize game");
        return;
    }

    LOG_DEBUG("Running game...");

    // game loop
    while (Window::ProcessMessages() && !m_Game.Exiting())
    {
        const bool gameCapture = m_State.appFocused && !m_Game.IsUiMode();

        if (gameCapture)
        {
            m_Window.LockCursor();
            m_Input.SetMouseMode(DirectX::Mouse::MODE_RELATIVE);
        }
        else
        {
            m_Window.UnlockCursor();
            m_Input.SetMouseMode(DirectX::Mouse::MODE_ABSOLUTE);
        }

        if (gameCapture != m_State.wasCapturingGameMouse)
            m_Game.SuppressLookThisFrame();

        m_State.wasCapturingGameMouse = gameCapture;

        // set game/ui input policy before sampling inputs
        // update input first
        m_Input.Update();

        // update internal clock
        m_Game.Tick();

        // update game system based on delta time
        m_Game.Update();

        // render the game
        m_Game.Draw();
    }

    // shut down
    // ShutDown();

    LOG_DEBUG("Game loop exited, shutting down.");
}

#undef LOG_TAG
