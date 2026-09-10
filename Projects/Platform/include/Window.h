#pragma once
#include <functional>
#include <string>

#include <Windows.h>

/**
 * @brief Type definition for a window hook function.
 * @details Create a lambda function or std::function matching this signature to
 * bind it to the window's wndproc chain. Prehooks execute before the window's own message processing.
 *
 * A hook should return true if a message was handled, and optionally set the LRESULT reference to specify a return value for the message.
 */
using Hook = std::function<bool(HWND, UINT, WPARAM, LPARAM, LRESULT&)>;

class Window
{
  public:
    Window(std::wstring title, int width, int height);
    ~Window();

    //set callbacks for various events

    /// Set callback for window resize event, callback receives new client width and height as parameters
    void SetResizeCallback(const std::function<void(UINT, UINT)>& callback) { m_OnResize = callback; }

    /// Set callback for key events, callback receives WPARAM of the key event and a boolean indicating whether the key was pressed (true) or released (false)
    void SetKeyCallback(const std::function<void(WPARAM, bool)>& callback) { m_OnKey = callback; }

    /// Set callback for mouse move events, callback receives delta x and delta y of the mouse movement since the last event
    void SetMouseCallback(const std::function<void(float, float)>& callback) { m_OnMouseMove = callback; }

    /// Set callback for app focus activation state changes (WM_ACTIVATEAPP)
    void SetFocusCallback(const std::function<void(bool)>& callback) { m_OnFocus = callback; }

    // cursor controls

    /// Lock the cursor to the center of the client area and hide it
    void LockCursor();

    /// Unlock the cursor and show it
    void UnlockCursor();

    /// Use when wanting to override mouse callback function for things like ImGui  io.WantCaptureMouse events
    void BlockAppMouse() { m_BlockAppMouse = true; }

    /// Give back mouse to mouse callback
    void UnBlockAppMouse() { m_BlockAppMouse = false; }

    /// Returns true if mouse is blocked for app input
    bool IsMouseAppBlocked() const { return m_BlockAppMouse; }

    /// Returns true if Mouse is currently locked to the center of the window
    bool IsCursorLocked() const { return m_CursorLocked; }

    void Show() const
    {
        ::ShowWindow(m_hWnd, SW_SHOW);
        ::UpdateWindow(m_hWnd);
    }
    // window info
    HWND GetHandle() const { return m_hWnd; }
    UINT GetClientWidth() const { return m_ClientWidth; }
    UINT GetClientHeight() const { return m_ClientHeight; }
    RECT GetClientRect() const;

    /// Get the current cursor position in screen coordinates
    static POINT GetCursorPos();

    /// Get the current cursor position in client coordinates
    POINT GetCursorClientPos() const;

    /// Get window title as wstring
    const std::wstring& GetTitle() const { return m_Title; }
    void                SetTitle(std::wstring title);

    /// @brief Add prehook to window's message queue
    /// @param hook Hook function to add, see Hook for more details.
    /// @details Prehooks are called in the order they were added before the window's default message processing.
    /// If a prehook returns true, it indicates that it has handled the message and no further processing
    /// (including other prehooks and the default window procedure) should occur for that message.
    ///
    /// The LRESULT for a normal message queue should be passed by reference
    void AddPreHook(Hook hook) { m_PreHooks.push_back(std::move(hook)); }

    /// process message loop, returns false if WM_QUIT was received
    static bool ProcessMessages();

  private:
    /// Static window procedure to handle messages for all win ndows of this class.
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /// helper to register window class, called in constructor, takes a WNDPROC to set as the window procedure for the class
    void RegisterWindowClass(WNDPROC proc) const;

    /// Update the stored client size variables
    void UpdateClientSize();

  private:
    HWND      m_hWnd      = nullptr;
    HINSTANCE m_hInstance = nullptr;

    std::wstring m_ClassName = L"D3D11SandboxClass";
    std::wstring m_Title;
    int          m_Width        = 0;
    int          m_Height       = 0;
    UINT         m_ClientWidth  = 0;
    UINT         m_ClientHeight = 0;

    // Callbacks
    std::function<void(UINT, UINT)>   m_OnResize    = nullptr; ///< callback
    std::function<void(WPARAM, bool)> m_OnKey       = nullptr; ///< callback
    std::function<void(float, float)> m_OnMouseMove = nullptr; ///< callback
    std::function<void(bool)>         m_OnFocus     = nullptr; ///< callback

    std::vector<Hook> m_PreHooks = {}; ///< take precedence over window's own message loops

    // cursor state
    bool  m_CursorLocked     = false;
    bool  m_BlockAppMouse    = false; ///< set to true if GUI should override what's going on in the app
    POINT m_MouseCenterPoint = { 0, 0 };
};
