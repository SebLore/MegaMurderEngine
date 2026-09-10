#pragma once

#include <Windows.h> // Must be included before Mouse.h

#include <memory>

// DirectXTK includes
#include <Keyboard.h>
#include <Mouse.h>

class Input
{
  public:
    struct Control
    {
        enum class Type
        {
            KEYBOARD,
            MOUSE,
            UNDEFINED
        } type;

        enum MouseButton : unsigned char
        {
            LEFT,
            MIDDLE,
            RIGHT,
            BUTTON_3,
            BUTTON_4
        };

      private:
        union
        {
            DirectX::Keyboard::Keys key;
            MouseButton             mouseButton;
            char                    unused;
        };

      public:
        Control() : type(Type::UNDEFINED), unused(0) {}
        Control(DirectX::Keyboard::Keys key) : type(Type::KEYBOARD), key(key) {}
        Control(MouseButton button) : type(Type::MOUSE), mouseButton(button) {}

        friend Input;
    };
    Input(const HWND& windowHandle);
    ~Input()                       = default;
    Input(const Input&)            = delete;
    Input& operator=(const Input&) = delete;

    /**
    * @brief Updates the states of the keyboard and mouse
    * @note Should be called every frame.
    */
    void Update();

    /**
    * @brief Resets the state trackers.
    * @note Should be called when the application loses and gains focus as
    * well as when eventual pause menus are toggled in-game.
    */
    void Reset();

    /// @returns true if the control is being held down
    bool IsDown(const Control& control) const;

    /// @returns true if the control is up
    bool IsUp(const Control& control) const;

    /// @returns true if the control has just been pressed
    bool IsPressed(const Control& control) const;

    /// @returns true if the control has just been released
    bool IsReleased(const Control& control) const;

    /// @returns The absolute horizontal screen position of the mouse cursor
    int GetMouseX() const;

    /// @returns The absolute vertical screen position of the mouse cursor
    int GetMouseY() const;

    /// @returns The horizontal mouse movement since last @ref Update()
    int GetMouseDeltaX() const;

    /// @returns The vertical mouse movement since last @ref Update()
    int GetMouseDeltaY() const;

    /// @returns The absolute scroll value since start of application
    int GetMouseScrollValue() const;

    /// @returns The scroll value since last @ref Update()
    int GetMouseScrollDelta() const;

    DirectX::Mouse::Mode GetMouseMode() const { return m_MouseState.positionMode; }

    void SetMouseMode(DirectX::Mouse::Mode mode) const { m_Mouse->SetMode(mode); }

    /// Hook the input messaging queue up to a window
    static bool WindowHook(HWND, UINT, WPARAM, LPARAM, LRESULT&);

  private:
    DirectX::Mouse::ButtonStateTracker::ButtonState GetMouseButtonState(Control::MouseButton button) const;

  private:
    std::unique_ptr<DirectX::Keyboard>      m_Keyboard;
    DirectX::Keyboard::State                m_KeyboardState{};
    DirectX::Keyboard::KeyboardStateTracker m_KeyboardStateTracker{};

    std::unique_ptr<DirectX::Mouse>    m_Mouse;
    DirectX::Mouse::State              m_MouseState{};
    DirectX::Mouse::ButtonStateTracker m_MouseStateTracker{};

    int m_LastMouseX           = 0;
    int m_LastMouseY           = 0;
    int m_LastMouseScrollValue = 0;
};
