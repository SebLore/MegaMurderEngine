#pragma once

#include <d3d11.h>
#include <functional>
#include <string>
#include <unordered_map>

#include "imgui.hpp"



class ImGuiInterface
{
public:
    struct Config {};
public:
    ImGuiInterface() = default;
    ~ImGuiInterface() { CleanUp(); }

    ImGuiInterface(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context, const Config& config = {})
    {
        Initialize(hwnd, device, context, config);
    }

    void Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context, const Config& config = {})
    {
        IMGUI_CHECKVERSION();
        ImGui_ImplWin32_EnableDpiAwareness(); // Enable DPI awareness for better scaling on high-DPI displays
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        ImGui::StyleColorsDark();

        // scale to dpi
        UINT  dpi = ::GetDpiForSystem();
        float scaling = static_cast<float>(dpi) / static_cast<float>(USER_DEFAULT_SCREEN_DPI);

        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(scaling); // Adjust scaling factor as needed

        {
            // either or
            //io.FontGlobalScale = scaling;

            io.Fonts->Clear();
            io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\cour.ttf", 16.0f * scaling);
            io.Fonts->Build();
        }

        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX11_Init(device, context);
        m_hwnd = hwnd;
        m_device = device;
        m_context = context;

        m_Initialized = true;
    }

    void NewFrame() const
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void Render() const
    {
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    void CleanUp() const
    {
        if (m_Initialized)
        {
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
        }
    }

#if 0
    // Register a window callback with a unique name and optional parent
    template <typename ...Args>
    inline
        void AddWindow(std::function<void(Args...)> fn, const std::string& name, const std::string& parent = "")
    {
        m_windows[name] = fn;
        if (!parent.empty())
            m_windowParents[name] = parent;
    }
#endif

    // Register a window callback with a unique name and optional parent
    void AddWindow(std::function<void()> fn, const std::string& name, const std::string& parent = "")
    {
        m_windows[name] = fn;
        if (!parent.empty())
            m_windowParents[name] = parent;
    }

    // Draw all registered windows (call this each frame)
    void DrawWindows()
    {

        for (const auto& [name, fn] : m_windows)
        {
            bool is_parent = (m_windowParents.find(name) == m_windowParents.end()); // Check if window has no parent
            bool has_parent_drawn =
                (!is_parent && m_windows.count(m_windowParents[name])); // Check if parent window is drawn
            // Only draw top-level windows or those whose parent is also being drawn
            if (is_parent || has_parent_drawn)
            {
                fn();
            }
            else
            {
                // Parent window not drawn; skip this window
            }
        }
    }

    void DrawWindow(const std::string& name)
    {
        auto it = m_windows.find(name);
        if (it != m_windows.end())
            it->second();
    }

    void Draw()
    {
        if (!m_enabled)
            return;

        NewFrame();
        DrawWindows();
        Render();
    }

    bool IsEnabled() const { return m_enabled; }
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    void ToggleEnabled() { m_enabled = !m_enabled; }

    bool IsInitialized() const { return m_Initialized; }

private:
    HWND                                                   m_hwnd = nullptr;
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    std::unordered_map<std::string, std::function<void()>> m_windows;
    std::unordered_map<std::string, std::string>           m_windowParents;
    bool                                                   m_enabled = true;
    bool                                                   m_Initialized = false;
};
