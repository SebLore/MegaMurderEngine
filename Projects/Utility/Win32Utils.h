/// @file Win32Utils.h
/// @brief Utilities for Win32 to make some stuff easier.
///
/// @date 2025-11-15
/// @author Sebastian L

#pragma once

#include "BaseUtils.h"

#ifndef _WIN32
#error "Win32Utils.h requires Windows (_WIN32 defined). Are you building on Linux/macOS?"
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <string>
#include <sstream>
#include <stdexcept>

namespace utils
{
    namespace win32
    {
#if defined(WIN32UTILS_USE_STRING_VIEW)
        using string_param = std::string_view;
#else
        using string_param = const std::string&;
#endif

        /// @brief Sets the text color of the console.
        /// @param color The color to set.
        inline void SetConsoleTextColor(WORD color) noexcept
        {
            HANDLE hConsole = ::GetStdHandle(STD_OUTPUT_HANDLE);
            if (hConsole != INVALID_HANDLE_VALUE)
                ::SetConsoleTextAttribute(hConsole, color);
        }

        /// @brief Throws a runtime error with the last Win32 error message.
        /// @param msg The message to include with the error.
        inline void ThrowLastError(string_param msg)
        {
            DWORD err      = ::GetLastError();
            char  buf[256] = {};

            ::FormatMessageA(
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,
                err,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default language
                buf,
                static_cast<DWORD>(sizeof(buf)),
                nullptr);
            std::stringstream ss;
            ss << msg << " (GetLastError: " << err << "): " << buf;
            throw MyRuntimeErr(ss.str());
        }

#if defined(UTILS_UNICODE) // using UNICODE, wide chars
        // ========================================================================================
        // UNICODE Window Class
        // ========================================================================================
        // Registers a window class and returns the ATOM.
        struct WindowClassDescW
        {
            HINSTANCE      hInstance = ::GetModuleHandleW(nullptr); // current module (exe) handle
            const wchar_t* className = L"WindowClassName";          // default class name
            WNDPROC        wndProc   = ::DefWindowProcW;            // default window proc, defaults to the standard one
            UINT           style     = CS_HREDRAW | CS_VREDRAW; // default style, redraw on horizontal/vertical resize
            HICON          hIcon     = ::LoadIconW(nullptr, IDI_APPLICATION);          // default icon
            HICON          hIconSm   = ::LoadIconW(nullptr, IDI_APPLICATION);          // default small icon
            HCURSOR        hCursor   = ::LoadCursorW(nullptr, IDC_ARROW);              // default arrow cursor
            HBRUSH         hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1); // default window color

            const wchar_t* menuName = nullptr; // no menu by default
        };

        /// @brief Registers a window class and returns the ATOM structure
        /// @param desc the WindowClassDescW structure describing the window class
        /// @return the ATOM of the registered window class
        inline ATOM RegisterWindowClassW(const WindowClassDescW& desc)
        {
            WNDCLASSEXW wc{};
            wc.cbSize        = sizeof(WNDCLASSEXW);
            wc.style         = desc.style;
            wc.lpfnWndProc   = desc.wndProc;
            wc.cbClsExtra    = 0;
            wc.cbWndExtra    = 0;
            wc.hInstance     = desc.hInstance;
            wc.hIcon         = desc.hIcon;
            wc.hCursor       = desc.hCursor;
            wc.hbrBackground = desc.hbrBackground;
            wc.lpszMenuName  = desc.menuName;
            wc.lpszClassName = desc.className;
            wc.hIconSm       = desc.hIconSm;

            ::ATOM atom = ::RegisterClassExW(&wc);
            if (!atom)
                ThrowLastError("Failed to register window class");
            return atom;
        }
        // Aliases for WindowClassDescW and RegisterWindowClassW
        using WindowClassDesc = WindowClassDescW;
        inline ATOM RegisterWindowClass(const WindowClassDesc& desc) { return RegisterWindowClassW(desc); }
#else   // not using UNICODE, using narrow chars
        // ========================================================================================
        // ANSI Window Class
        // ========================================================================================
        // Window class description struct for ANSI
        struct WindowClassDescA
        {
            HINSTANCE   hInstance     = ::GetModuleHandleA(nullptr); // current module (exe) handle
            const char* className     = "WindowClassName";           // default class name
            WNDPROC     wndProc       = ::DefWindowProcA;        // default window proc, defaults to the standard one
            UINT        style         = CS_HREDRAW | CS_VREDRAW; // default style, redraw on horizontal/vertical resize
            HICON       hIcon         = ::LoadIconA(nullptr, IDI_APPLICATION);      // default icon
            HICON       hIconSm       = ::LoadIconA(nullptr, IDI_APPLICATION);      // default small icon
            HCURSOR     hCursor       = ::LoadCursorA(nullptr, IDC_ARROW);          // default arrow cursor
            HBRUSH      hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1); // default window color
            const char* menuName      = nullptr;                                    // no menu by default
        };

        /// @brief Registers a window class and returns the ATOM structure
        /// @param desc the WindowClassDescA structure describing the window class
        /// @return the ATOM of the registered window class
        inline ATOM RegisterWindowClassA(const WindowClassDescA& desc)
        {
            WNDCLASSEXA wc{};
            wc.cbSize        = sizeof(WNDCLASSEXA);
            wc.style         = desc.style;
            wc.lpfnWndProc   = desc.wndProc;
            wc.cbClsExtra    = 0;
            wc.cbWndExtra    = 0;
            wc.hInstance     = desc.hInstance;
            wc.hIcon         = desc.hIcon;
            wc.hCursor       = desc.hCursor;
            wc.hbrBackground = desc.hbrBackground;
            wc.lpszMenuName  = desc.menuName;
            wc.lpszClassName = desc.className;
            wc.hIconSm       = desc.hIconSm;

            ATOM atom = ::RegisterClassExA(&wc);
            if (!atom)
                ThrowLastError("Failed to register window class");
            return atom;
        }
        using WindowClassDesc = WindowClassDescA;
        inline ATOM RegisterWindowClass(const WindowClassDesc& desc) { return RegisterWindowClassA(desc); }
#endif
        inline LRESULT CALLBACK DefaultWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            switch (msg)
            {
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
            default:
                return DefWindowProc(hwnd, msg, wParam, lParam); // default handling
            }
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }

#if defined(UTILS_UNICODE)
        // Description of a window to create
        struct WindowDescW
        {
            HINSTANCE      hInstance   = GetModuleHandleW(nullptr);
            const wchar_t* className   = L"Win32UtilsWindowClass";
            const wchar_t* windowTitle = L"Win32Utils Window";

            DWORD style   = WS_OVERLAPPEDWINDOW;
            DWORD exStyle = 0;

            int x      = CW_USEDEFAULT;
            int y      = CW_USEDEFAULT;
            int width  = 800;
            int height = 600;

            HWND  hWndParent = nullptr;
            HMENU hMenu      = nullptr;
            void* lpParam    = nullptr; // passed as CREATESTRUCT::lpCreateParams
        };
        using WindowDesc = WindowDescW;

        inline HWND MakeWindow(const WindowDesc& desc)
        {
            HWND hwnd = CreateWindowExW(
                desc.exStyle,
                desc.className,
                desc.windowTitle,
                desc.style,
                desc.x,
                desc.y,
                desc.width,
                desc.height,
                desc.hWndParent,
                desc.hMenu,
                desc.hInstance,
                desc.lpParam);
#else
        HWND hwnd = CreateWindowExA(
            desc.exStyle,
            reinterpret_cast<LPCSTR>(desc.className),
            reinterpret_cast<LPCSTR>(desc.windowTitle),
            desc.style,
            desc.x,
            desc.y,
            desc.width,
            desc.height,
            desc.hWndParent,
            desc.hMenu,
            desc.hInstance,
            desc.lpParam);
#endif
            if (!hwnd)
                ThrowLastError("Failed to create window");
            return hwnd;
        }

        // Shows and updates window
        inline void DisplayWindow(HWND hwnd, int nCmdShow = SW_SHOWDEFAULT)
        {
            ::ShowWindow(hwnd, nCmdShow);
            ::UpdateWindow(hwnd);
        }

        // Basic message loop: runs until WM_QUIT
        inline int RunMessageLoop()
        {
            MSG msg;
            while (GetMessageW(&msg, nullptr, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            return static_cast<int>(msg.wParam);
        }

        // Peek message loop: processes all pending messages and returns
        inline int PeekMessageLoop()
        {
            MSG msg;
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            return static_cast<int>(msg.wParam);
        }

        // Miscellaneous utility functions
        // Convert narrow string (e.what()) to wide
        inline std::wstring ToWide(string_param s, UINT codePage = CP_ACP)
        {
            if (s.empty())
                return {};

            // Query required buffer size (in wchar_t)
            int len = ::MultiByteToWideChar(codePage, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);

            if (len <= 0)
                return {}; // or throw, depending on how strict you want to be

            std::wstring result(len, L'\0');

            ::MultiByteToWideChar(codePage, 0, s.data(), static_cast<int>(s.size()), result.data(), len);

            return result;
        }

        inline std::wstring ToWide(const std::exception& e, UINT codePage = CP_ACP)
        {
            return ToWide(string_param(e.what()), codePage);
        }
    } // namespace win32
} // namespace utils
