#pragma once
#include <cstring>
#include <iomanip> // for std::setw so that log levels align
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

// Platform-specific includes
#ifdef _WIN32
#include "include/ConsoleManip.h"
#endif

namespace dbglog
{
    enum class LogLevel
    {
        debug,
        info,
        warn,
        error
    };

    inline const char* level_name(LogLevel L)
    {
        switch (L)
        {
        case LogLevel::info:
            return "";
            break;
        case LogLevel::debug:
            return "[DEBUG]  ";
            break;
        case LogLevel::warn:
            return "[WARNING]";
            break;
        case LogLevel::error:
            return "[ERROR]  ";
            break;
        default:
            return "";
            break;
        }
    }

    // Wide version for wcout/wstring
    inline const wchar_t* level_name_w(LogLevel L)
    {
        switch (L)
        {
        case LogLevel::info:
            return L"";
        case LogLevel::debug:
            return L"[DEBUG]  ";
        case LogLevel::warn:
            return L"[WARNING]";
        case LogLevel::error:
            return L"[ERROR]  ";
        }
        return L"";
    }

#ifdef WIN32_CONSOLE_COLOR
    inline ConsoleColor get_level_color(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::debug:
            return ConsoleColor::CYAN;
        case LogLevel::info:
            return ConsoleColor::DEFAULT;
        case LogLevel::warn:
            return ConsoleColor::YELLOW;
        case LogLevel::error:
            return ConsoleColor::RED;
        }
        return ConsoleColor::DEFAULT;
    }
#endif

    // Enable debug logs unless NDEBUG is set
#if defined(_DEBUG)
    inline bool debug_enabled = true;
#else
    inline bool debug_enabled = false;
#endif

    // Shared mutex for thread-safe console output
    inline std::mutex& io_mutex()
    {
        static std::mutex m;
        return m;
    }

    /// @brief Log a message to the console with the specified log level
    /// @param level as defined in dbglog::LogLevel
    /// @param tag unique tag for the module, like "ResourceManager"
    /// @param func function name where the log is generated
    /// @param message the log message
    inline void log(dbglog::LogLevel level, const char* tag, const char* func, const std::string& message)
    {
        if (level == dbglog::LogLevel::debug && !debug_enabled)
            return;

        // ensure thread-safe output
        std::lock_guard<std::mutex> lock(io_mutex());
#ifdef WIN32_CONSOLE_COLOR
        // Apply color for the entire log message
        ColorGuard colorGuard(get_level_color(level));
#endif
        std::cout << dbglog::level_name(level) << tag << "::" << func << ": " << message << '\n';
    }

    /// @brief Log a message to the console with the specified log level,
    /// without function name
    inline void log_nofunc(dbglog::LogLevel level, const char* tag, const std::string& message)
    {
        if (level == dbglog::LogLevel::debug && !debug_enabled)
            return;

        // ensure thread-safe output
        std::lock_guard<std::mutex> lock(io_mutex());
#ifdef WIN32_CONSOLE_COLOR
        // Apply color for the entire log message
        ColorGuard colorGuard(get_level_color(level));
#endif
        std::cout << dbglog::level_name(level) << tag << ": " << message << '\n';
    }

    template <typename Func, typename Type>
    void log_once_impl(Func f, Type t)
    {
        static bool first = true;
        if (first)
        {
            f();
            first = false;
        }
    }

    template <typename Func>
    void log_once(Func f)
    {
        log_once_impl(f, []() {});
    }

    // Helpers for widening narrow strings (for __func__ / LOG_TAG)
    inline std::wstring widen(const char* s) { return std::wstring{ s, s + std::char_traits<char>::length(s) }; }
    inline std::wstring widen(const std::string& s) { return std::wstring{ s.begin(), s.end() }; }

    // Wide logging overload (wstring / wchar_t)
    inline void
    log(dbglog::LogLevel level, const std::wstring& tag, const std::wstring& func, const std::wstring& message)
    {
        if (level == dbglog::LogLevel::debug && !debug_enabled)
            return;

        std::lock_guard<std::mutex> lock(io_mutex());
#ifdef WIN32_CONSOLE_COLOR
        WideColorGuard colorGuard(get_level_color(level));
#endif
        std::wcout << dbglog::level_name_w(level) << tag << L"::" << func << L": " << message << L'\n';
    }

    class LogClock
    {
      public:
        explicit LogClock(size_t lifetime) : m_Lifetime(lifetime) {}

        void Write(dbglog::LogLevel level, const char* tag, const char* func, const std::string& message)
        {
            if (m_Lifetime == 0)
                return;

            log(level, tag, func, message);
            --m_Lifetime;
        }

      private:
        size_t m_Lifetime;
    };

} // namespace dbglog

#ifndef LOG_TAG
#define LOG_TAG "Unknown"
#endif

#ifndef LOG_WTAG
#define LOG_WTAG L"Unknown"
#endif

// helper macro to build string from expressions, e.g. LOG_INFO("Value: " <<
// value) or LOG_ERROR("Error code: " + errorCode);
#define BUILD_MSG(expr)                                                                                                \
    (                                                                                                                  \
        [&]                                                                                                            \
        {                                                                                                              \
            std::ostringstream oss;                                                                                    \
            oss << expr;                                                                                               \
            return oss.str();                                                                                          \
        }())
#define BUILD_WMSG(expr)                                                                                               \
    (                                                                                                                  \
        [&]                                                                                                            \
        {                                                                                                              \
            std::wostringstream woss;                                                                                  \
            woss << expr;                                                                                              \
            return woss.str();                                                                                         \
        }())

#define LOG_INFO(msg)  dbglog::log(dbglog::LogLevel::info, LOG_TAG, __func__, BUILD_MSG(msg))
#define LOG_DEBUG(msg) dbglog::log(dbglog::LogLevel::debug, LOG_TAG, __func__, BUILD_MSG(msg))
#define LOG_WARN(msg)  dbglog::log(dbglog::LogLevel::warn, LOG_TAG, __func__, BUILD_MSG(msg))
#define LOG_ERROR(msg) dbglog::log(dbglog::LogLevel::error, LOG_TAG, __func__, BUILD_MSG(msg))

#define LOG_INFO_NOFUNC(msg)  dbglog::log_nofunc(dbglog::LogLevel::info, LOG_TAG, BUILD_MSG(msg))
#define LOG_DEBUG_NOFUNC(msg) dbglog::log_nofunc(dbglog::LogLevel::debug, LOG_TAG, BUILD_MSG(msg))
#define LOG_WARN_NOFUNC(msg)  dbglog::log_nofunc(dbglog::LogLevel::warn, LOG_TAG, BUILD_MSG(msg))
#define LOG_ERROR_NOFUNC(msg) dbglog::log_nofunc(dbglog::LogLevel::error, LOG_TAG, BUILD_MSG(msg))

#define LOG_INFO_W(msg)                                                                                                \
    dbglog::log(dbglog::LogLevel::info, std::wstring(LOG_WTAG), dbglog::widen(__func__), BUILD_WMSG(msg))
#define LOG_DEBUG_W(msg)                                                                                               \
    dbglog::log(dbglog::LogLevel::debug, std::wstring(LOG_WTAG), dbglog::widen(__func__), BUILD_WMSG(msg))
#define LOG_WARN_W(msg)                                                                                                \
    dbglog::log(dbglog::LogLevel::warn, std::wstring(LOG_WTAG), dbglog::widen(__func__), BUILD_WMSG(msg))
#define LOG_ERROR_W(msg)                                                                                               \
    dbglog::log(dbglog::LogLevel::error, std::wstring(LOG_WTAG), dbglog::widen(__func__), BUILD_WMSG(msg))

// helper macro to print a message N times using macro
// https://gcc.gnu.org/onlinedocs/cpp/Standard-Predefined-Macros.html,
// https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
#define LOG_WARN_N(msg, N)                                                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        static dbglog::LogClock warn##__LINE__((N));                                                                   \
        warn##__LINE__.Write(dbglog::LogLevel::warn, LOG_TAG, __func__, BUILD_MSG(msg));                               \
    } while (0)

#undef LOG_TAG
#undef LOG_WTAG
