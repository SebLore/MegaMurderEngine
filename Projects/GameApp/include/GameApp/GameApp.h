#pragma once

#include <Input.h>
#include <Window.h>

#include <imgui/imgui.hpp>

#include "Engine/GameEngine.h"

#include <string>

#ifdef USING_IMGUI
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

inline bool ImGui_WindowHook(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    {
        result = 1;
        return true;
    }
    return false;
}
#endif

class GameApp
{
  public:
    struct Config
    {
        std::wstring Title = L"GameApp";
        struct Window
        {
            int Width  = 800;
            int Height = 600;
        } Window;
        unsigned int flags = 0;

        std::string defaultPath = "Game.cfg";

        bool Load(const char* path);
        bool Save(const char* path);
    };

    struct State
    {
        bool appFocused            = true;
        bool wasCapturingGameMouse = false;
    };

  public:
    // constructor
    explicit GameApp(const wchar_t* name, int wWidth, int wHeight, unsigned flags = 0)
        : m_Window(name, wWidth, wHeight), m_Input(m_Window.GetHandle()),
          m_Game(m_Window.GetHandle(), m_Window.GetClientWidth(), m_Window.GetClientHeight(), m_Input, flags)
    {
        m_Window.SetFocusCallback(
            [&](bool focused)
            {
                m_State.appFocused = focused;
                m_Game.SetAppFocused(focused);
            });
        m_Window.SetResizeCallback([&](UINT w, UINT h) { m_Game.OnResize(w, h); });

#ifdef USING_IMGUI
        m_Window.AddPreHook(ImGui_WindowHook);
#endif

        m_Window.AddPreHook(Input::WindowHook);

        m_Window.Show();
    }

    /// Call other constructor using a Config struct
    explicit GameApp(const Config& c) : GameApp(c.Title.c_str(), c.Window.Width, c.Window.Height, c.flags) {}

    /// Run game until WM_QUIT is received or error occurs
    void Run();

  private:
    // Platform
    Window m_Window;
    Input  m_Input;

    // Game
    Murder::GameEngine m_Game;

    // State flags
    State m_State;
};
