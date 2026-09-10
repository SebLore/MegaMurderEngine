#include "Window.h"

#include <ShellScalingApi.h> // for SetProcessDpiAwareness and related DPI functions
#include <Windowsx.h>        // for GET_X_LPARAM, GET_Y_LPARAM etc.

#include <Utility/Logging.h>

#define LOG_TAG "Window"

namespace
{
    constexpr int kMaxCursorAdjustAttempts = 128;

    //void SetCursorVisible(bool visible)
    //{
    //    CURSORINFO ci{};
    //    ci.cbSize = sizeof(ci);

    //    for (int i = 0; i < kMaxCursorAdjustAttempts; ++i)
    //    {
    //        if (!::GetCursorInfo(&ci))
    //            break;

    //        const bool isVisible = (ci.flags & CURSOR_SHOWING) != 0;
    //        if (isVisible == visible)
    //            return;

    //        ::ShowCursor(visible);
    //    }
    //}
} // namespace

#pragma comment(lib, "Shcore.lib") // needed to define ShellScalingApi

Window::Window(std::wstring title, int width, int height) : m_Title(std::move(title)), m_Width(width), m_Height(height)
{
    m_hInstance = GetModuleHandle(nullptr);

    // register own window class as the base
    RegisterWindowClass(Window::WndProc);

    DWORD style = WS_OVERLAPPEDWINDOW;

    // for high-DPI (Dots Per Inch) displays TODO: make this a flag, not default
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-adjustwindowrectexfordpi
    // https://learn.microsoft.com/en-us/windows/win32/api/shellscalingapi/nf-shellscalingapi-setprocessdpiawareness

    HRESULT hr = ::SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);

    if (FAILED(hr))
        LOG_WARN("Failed to set process DPI awareness.");

    UINT dpi = ::GetDpiForSystem();

    // divide pixelwidth and height by dpi
    int pixelWidth  = ::MulDiv(width, dpi, 96);
    int pixelHeight = ::MulDiv(height, dpi, 96);

    // set the window dimensions to account for DPI
    RECT rc = { 0, 0, pixelWidth, pixelHeight };
    ::AdjustWindowRectExForDpi(&rc, style, FALSE, 0, dpi);

    // store new size
    int winWidth  = rc.right - rc.left;
    int winHeight = rc.bottom - rc.top;
    m_Width       = winWidth;
    m_Height      = winHeight;

    // create the window
    m_hWnd = CreateWindowEx(
        0,
        m_ClassName.c_str(),
        m_Title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        m_Width,
        m_Height,
        nullptr,
        nullptr,
        m_hInstance,
        this);

    // More precise controls, no virtual mouse/keyboard but reading the input directly
    // Read more: https://learn.microsoft.com/en-us/windows/win32/inputdev/raw-input
    RAWINPUTDEVICE rid{
        .usUsagePage = 0x01, // Generic desktop controls
        .usUsage     = 0x02, // Mouse
        .dwFlags     = 0,    // Default behavior
        .hwndTarget  = m_hWnd,
    };
    ::RegisterRawInputDevices(&rid, 1, sizeof(rid));

    UpdateClientSize();
}

Window::~Window()
{
    if (m_hWnd)
    {
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }

    ::UnregisterClassW(m_ClassName.c_str(), m_hInstance);
}

void Window::RegisterWindowClass(WNDPROC proc) const
{
    WNDCLASSEX wc    = {};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_CLASSDC;
    wc.lpfnWndProc   = proc;
    wc.hInstance     = m_hInstance;
    wc.lpszClassName = m_ClassName.c_str();
    wc.hCursor       = ::LoadCursor(nullptr, IDC_ARROW);

    RegisterClassEx(&wc);
}

void Window::UpdateClientSize()
{
    RECT rect;
    ::GetClientRect(m_hWnd, &rect);
    m_ClientWidth  = rect.right - rect.left;
    m_ClientHeight = rect.bottom - rect.top;
}

/// Process window messages and return false if a quit message was received
bool Window::ProcessMessages()
{
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            return false;

        ::TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return true;
}

void Window::LockCursor()
{
    // early return if already locked
    if (m_CursorLocked)
        return;

    // Get client rect in screen space
    RECT client;
    ::GetClientRect(m_hWnd, &client);

    // store top-left and bottom-right points
    POINT ul = { client.left, client.top };
    POINT lr = { client.right, client.bottom };

    // Then convert to screen space
    ::MapWindowPoints(m_hWnd, nullptr, &ul, 1);
    ::MapWindowPoints(m_hWnd, nullptr, &lr, 1);

    // Clip cursor to client area
    RECT clipRect = { ul.x, ul.y, lr.x, lr.y };
    ::ClipCursor(&clipRect);

    // Hide cursor and recenter
    //SetCursorVisible(false);

    // center cursor on the screen
    ::GetClientRect(m_hWnd, &client);
    POINT center = { (client.right - client.left) / 2, (client.bottom - client.top) / 2 };
    ::ClientToScreen(m_hWnd, &center);
    ::SetCursorPos(center.x, center.y);

    // remember center point and flag as locked
    m_MouseCenterPoint = center;
    m_CursorLocked     = true;
}

void Window::UnlockCursor()
{
    if (!m_CursorLocked)
        return;

    // toggle cursor lock off and show cursor again
    ::ClipCursor(nullptr);
    //SetCursorVisible(true);
    m_CursorLocked = false;
}

RECT Window::GetClientRect() const
{
    RECT rect;
    ::GetClientRect(m_hWnd, &rect);
    return rect;
}

POINT Window::GetCursorPos()
{
    POINT p;
    ::GetCursorPos(&p);
    return p;
}

/// Get the current cursor position in client coordinates
POINT Window::GetCursorClientPos() const
{
    POINT p = GetCursorPos();
    ::ScreenToClient(m_hWnd, &p);
    return p;
}

void Window::SetTitle(std::wstring title)
{
    m_Title = std::move(title);
    if (m_hWnd)
        SetWindowText(m_hWnd, m_Title.c_str());
}

LRESULT Window::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto* self = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd,
                                                            GWLP_USERDATA)); // NOLINT(performance-no-int-to-ptr)

    // hook self if not done
    if (msg == WM_NCCREATE)
    {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam); // NOLINT(performance-no-int-to-ptr)

        self = static_cast<Window*>(cs->lpCreateParams);

        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));

        // keep track of the window handle in the instance
        self->m_hWnd = hWnd;

        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    // if no instance, pass to windows default proc
    if (!self)
        return DefWindowProc(hWnd, msg, wParam, lParam);

    // go through pre-hooks, if any return true return the result
    for (auto& hook : self->m_PreHooks)
    {
        LRESULT hookResult = 0;

        if (hook && hook(hWnd, msg, wParam, lParam, hookResult))
            return hookResult;
    }

    // unlock cursor if blocked and locked
    if (self->m_CursorLocked && self->m_BlockAppMouse)
        self->UnlockCursor();

    // if nothing was caught by pre-hooks, handle default messages
    switch (msg)
    {
    case WM_ACTIVATEAPP:
    {

        if (self->m_OnFocus)
            self->m_OnFocus(wParam != 0);
        return 0;
    }
    case WM_SETCURSOR:
    {
        // LOWORD(lParam) is the hit-test code, tells us where the cursor is at
        if (self->m_CursorLocked && LOWORD(lParam) == HTCLIENT)
        {
            ::SetCursor(nullptr);
            return TRUE;
        }
        break;
    }

    case WM_SIZE:
    {
        if (self->m_OnResize)
        {
            const UINT newWidth  = LOWORD(lParam);
            const UINT newHeight = HIWORD(lParam);

            if (newWidth && newHeight)
                self->m_OnResize(newWidth, newHeight);
        }
        return 0;
    }

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;

    case WM_MOUSEACTIVATE:
        // When clicking to activate the window, we want Mouse to ignore it.
        return MA_ACTIVATEANDEAT;

    case WM_MENUCHAR:
        // A menu is active and the user presses a key that does not correspond
        // to any mnemonic or accelerator key. Ignore so we don't produce an error beep.
        return MAKELRESULT(0, MNC_CLOSE);
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        if (self->m_OnKey)
        {
            const bool pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
            self->m_OnKey(wParam, pressed);
        }
        return 0;
    }

    case WM_KILLFOCUS:
        return 0;
    case WM_INPUT:
        if (self->m_CursorLocked && !self->IsMouseAppBlocked())
        {
            // store buffer to receive raw input data
            UINT dwSize = 0;

            // first get size of input data
            ::GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));

            // early out if size is invalid
            if (dwSize == 0)
                return 0;

            // allocate buffer and get raw input data
            std::vector<BYTE> lpb(dwSize);

            if (::GetRawInputData(
                    reinterpret_cast<HRAWINPUT>(lParam),
                    RID_INPUT,
                    lpb.data(),
                    &dwSize,
                    sizeof(RAWINPUTHEADER)) != dwSize)
                return 0;

            // interpret as RAWINPUT structure
            RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(lpb.data());

            // early out if we couldn't get the data
            if (raw->header.dwType != RIM_TYPEMOUSE)
                return 0;

            // get relative movement
            LONG dx = raw->data.mouse.lLastX;
            LONG dy = raw->data.mouse.lLastY;

            // call mouse move callback if set
            if (self->m_OnMouseMove)
                self->m_OnMouseMove(static_cast<float>(dx), static_cast<float>(dy));
        }
        return 0;
    default:
        break;
    }

    // pass unhandled messages to default window proc
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
