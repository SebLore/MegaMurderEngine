#pragma once

#ifdef _WIN32
#define WIN32_CONSOLE_COLOR

#include <fcntl.h> // for _O_U16TEXT
#include <io.h>    // for _setmode
#include <windows.h>

// Windows console color codes
enum class ConsoleColor : WORD
{
    DEFAULT = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
    RED     = FOREGROUND_RED | FOREGROUND_INTENSITY,
    YELLOW  = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
    GREEN   = FOREGROUND_GREEN | FOREGROUND_INTENSITY,
    BLUE    = FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    CYAN    = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    MAGENTA = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    WHITE   = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
};

class ColorGuard
{
  private:
    HANDLE                     m_consoleHandle;
    CONSOLE_SCREEN_BUFFER_INFO m_originalInfo = {};
    bool                       m_validHandle;

  public:
    ColorGuard(ConsoleColor color) : m_validHandle(false)
    {
        m_consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (m_consoleHandle != INVALID_HANDLE_VALUE)
        {
            if (GetConsoleScreenBufferInfo(m_consoleHandle, &m_originalInfo))
            {
                SetConsoleTextAttribute(m_consoleHandle, static_cast<WORD>(color));
                m_validHandle = true;
            }
        }
    }

    ~ColorGuard()
    {
        if (m_validHandle)
            SetConsoleTextAttribute(m_consoleHandle, m_originalInfo.wAttributes);
    }
};

class WideColorGuard
{
  private:
    HANDLE                     m_consoleHandle;
    CONSOLE_SCREEN_BUFFER_INFO m_originalInfo = {};
    bool                       m_validHandle;

  public:
    WideColorGuard(ConsoleColor color) : m_validHandle(false)
    {
        m_consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (m_consoleHandle != INVALID_HANDLE_VALUE)
        {
            if (GetConsoleScreenBufferInfo(m_consoleHandle, &m_originalInfo))
            {
                SetConsoleTextAttribute(m_consoleHandle, static_cast<WORD>(color));
                m_validHandle = true;
            }
        }
    }

    ~WideColorGuard()
    {
        if (m_validHandle)
            SetConsoleTextAttribute(m_consoleHandle, m_originalInfo.wAttributes);
    }
};
#endif
