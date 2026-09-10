#include "Input.h"

#include <Utility/ErrorHandling.h>

#ifdef DEBUG_INPUT
#include <Utility/Logging.h>
#define LOG_TAG "Input"
#endif // DEBUG_INPUT

Input::Input(const HWND& windowHandle)
    : m_Keyboard(std::make_unique<DirectX::Keyboard>()), m_Mouse(std::make_unique<DirectX::Mouse>())
{
    m_Mouse->SetWindow(windowHandle);
}

void Input::Update()
{
    m_KeyboardState = m_Keyboard->GetState();
    m_KeyboardStateTracker.Update(m_KeyboardState);

    if (m_MouseState.positionMode == DirectX::Mouse::MODE_ABSOLUTE)
    {
        m_LastMouseX = m_MouseState.x;
        m_LastMouseY = m_MouseState.y;
    }
    m_LastMouseScrollValue = m_MouseState.scrollWheelValue;

    m_MouseState = m_Mouse->GetState();
    m_MouseStateTracker.Update(m_MouseState);

#ifdef DEBUG_INPUT
    m_Mouse->SetMode(
        m_MouseState.leftButton ? DirectX::Mouse::Mode::MODE_RELATIVE : DirectX::Mouse::Mode::MODE_ABSOLUTE);

    for (int i = 0; i < 256; i++)
    {
        Control control(static_cast<DirectX::Keyboard::Keys>(i));

        if (IsPressed(control))
            LOG_DEBUG("Key " << i << " is pressed");
    }

    for (int i = 0; i < 5; i++)
    {
        Control control(static_cast<Control::MouseButton>(i));

        if (IsPressed(control))
            LOG_DEBUG("Mouse Button " << i << " is pressed at (" << GetMouseX() << ", " << GetMouseY() << ")");

        if (IsDown(control))
            LOG_DEBUG("Delta (" << GetMouseDeltaX() << ", " << GetMouseDeltaY() << ")");

        if (IsReleased(control))
            LOG_DEBUG("Mouse Button " << i << " is released at (" << GetMouseX() << ", " << GetMouseY() << ")");
    }
#endif // DEBUG_INPUT
}

void Input::Reset()
{
    m_KeyboardStateTracker.Reset();
    m_MouseStateTracker.Reset();
}

DirectX::Mouse::ButtonStateTracker::ButtonState Input::GetMouseButtonState(Control::MouseButton button) const
{
    switch (button)
    {
    case Control::MouseButton::LEFT:
        return m_MouseStateTracker.leftButton;
    case Control::MouseButton::MIDDLE:
        return m_MouseStateTracker.middleButton;
    case Control::MouseButton::RIGHT:
        return m_MouseStateTracker.rightButton;
    case Control::MouseButton::BUTTON_3:
        return m_MouseStateTracker.xButton1;
    case Control::MouseButton::BUTTON_4:
        return m_MouseStateTracker.xButton2;
    default:
        break;
    }
    THROWRE("Invalid mouse button " << button);
}

bool Input::IsDown(const Control& control) const
{
    if (control.type == Control::Type::UNDEFINED)
        return false;

    if (control.type == Control::Type::KEYBOARD)
        return m_KeyboardState.IsKeyDown(control.key);
    return GetMouseButtonState(control.mouseButton) == DirectX::Mouse::ButtonStateTracker::HELD;
}

bool Input::IsUp(const Control& control) const
{
    if (control.type == Control::Type::UNDEFINED)
        return false;

    if (control.type == Control::Type::KEYBOARD)
        return m_KeyboardState.IsKeyUp(control.key);
    return GetMouseButtonState(control.mouseButton) == DirectX::Mouse::ButtonStateTracker::UP;
}

bool Input::IsPressed(const Control& control) const
{
    if (control.type == Control::Type::UNDEFINED)
        return false;

    if (control.type == Control::Type::KEYBOARD)
        return m_KeyboardStateTracker.IsKeyPressed(control.key);
    return GetMouseButtonState(control.mouseButton) == DirectX::Mouse::ButtonStateTracker::PRESSED;
}

bool Input::IsReleased(const Control& control) const
{
    if (control.type == Control::Type::UNDEFINED)
        return false;

    if (control.type == Control::Type::KEYBOARD)
        return m_KeyboardStateTracker.IsKeyReleased(control.key);
    return GetMouseButtonState(control.mouseButton) == DirectX::Mouse::ButtonStateTracker::RELEASED;
}

int Input::GetMouseX() const
{
    if (m_MouseState.positionMode == DirectX::Mouse::MODE_ABSOLUTE)
        return m_MouseState.x;
    return m_LastMouseX;
}

int Input::GetMouseY() const
{
    if (m_MouseState.positionMode == DirectX::Mouse::MODE_ABSOLUTE)
        return m_MouseState.y;
    return m_LastMouseY;
}

int Input::GetMouseDeltaX() const
{
    if (m_MouseState.positionMode == DirectX::Mouse::MODE_ABSOLUTE)
        return m_MouseState.x - m_LastMouseX;
    return m_MouseState.x;
}

int Input::GetMouseDeltaY() const
{
    if (m_MouseState.positionMode == DirectX::Mouse::MODE_ABSOLUTE)
        return m_MouseState.y - m_LastMouseY;
    return m_MouseState.y;
}

int Input::GetMouseScrollValue() const { return m_MouseState.scrollWheelValue; }

int Input::GetMouseScrollDelta() const { return m_MouseState.scrollWheelValue - m_LastMouseScrollValue; }

bool Input::WindowHook(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    switch (msg)
    {
    case WM_ACTIVATEAPP:
        DirectX::Keyboard::ProcessMessage(msg, wParam, lParam);
        DirectX::Mouse::ProcessMessage(msg, wParam, lParam);
        break;
    case WM_ACTIVATE:
    case WM_INPUT:
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEWHEEL:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_MOUSEHOVER:
        DirectX::Mouse::ProcessMessage(msg, wParam, lParam);
        break;
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
        DirectX::Keyboard::ProcessMessage(msg, wParam, lParam);
        break;
    default:
        break;
    }

    // return false when we don't eat the message
    return false;
}

#ifdef DEBUG_INPUT
#undef LOG_TAG
#endif // DEBUG_INPUT
