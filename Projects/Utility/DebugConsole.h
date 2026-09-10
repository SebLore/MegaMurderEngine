/**********************************************************************
 * @file   DebugConsole.h
 * @brief  Creates an output console for win32. Requires 
 * ::ShowWindow() to be called on the main window in order for
 * COM to be initialized, otherwise printing to the console
 * will fail.
 * 
 * @author Sebastian
 * @date   October 2024
 *********************************************************************/
#pragma once

#include <Windows.h> // for AllocConsole, FreeConsole, GetStdHandle

#include <iostream> // for output/input streams
#include <cassert>  // for assert
//#include <fstream>
//#include <string>

// DebugConsole creates a debug console window, redirecting cout, cerr and cin stream to the console window.
class DebugConsole
{
  public:
    // constructor allocates a new console and redirects the standard streams
    explicit DebugConsole()
    {
        if (::AllocConsole()) // windows API call to allocate a new console
        {
            // redirect the streams to the console
            freopen_s(&stream_in, "CONIN$", "r", stdin);    // redirect	cin stream
            freopen_s(&stream_out, "CONOUT$", "w", stdout); // redirect cout stream
            freopen_s(&stream_err, "CONOUT$", "w", stderr); // redirect cerr

            // clear the streams
            std::cout.clear();
            std::cerr.clear();
            std::cin.clear();
            std::wcout.clear();
            std::wcerr.clear();
            std::wcin.clear();

            m_hConsole = ::GetStdHandle(STD_OUTPUT_HANDLE);
        }
        assert(m_hConsole != nullptr && "Failed to create debug console window");
    }
    // destructor frees the console and closes the streams automatically when it coes out of scope
    ~DebugConsole()
    {
        if (stream_in)
            ::fclose(stream_in);
        if (stream_out)
            ::fclose(stream_out);
        if (stream_err)
            ::fclose(stream_err);
        ::FreeConsole();
    }

    // get the console handle for advanced operations (not used)
    HANDLE handle() const noexcept { return m_hConsole; }

    void SetTextColor(WORD color) const noexcept { SetConsoleTextAttribute(m_hConsole, color); }

    // no copying
    DebugConsole(const DebugConsole&)            = delete;
    DebugConsole& operator=(const DebugConsole&) = delete;

  private:
    HANDLE m_hConsole = nullptr;
    FILE*  stream_in  = nullptr;
    FILE*  stream_out = nullptr;
    FILE*  stream_err = nullptr;
};
