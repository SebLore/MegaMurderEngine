#pragma once

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

// namespace so it doesn't conflict with other libraries
namespace err
{
    enum class ErrorType
    {
        runtime,
        invarg,
        oor
    };

    inline void Error(ErrorType e, const std::string& msg)
    {
        switch (e)
        {
        case ErrorType::runtime:
            throw std::runtime_error(msg);
        case ErrorType::invarg:
            throw std::invalid_argument(msg);
        case ErrorType::oor:
            throw std::out_of_range(msg);
        default:
            throw msg;
        }
    }
    inline const char* ErrorName(ErrorType e)
    {
        switch (e)
        {
        case ErrorType::runtime:
            return "Runtime Error   ";
        case ErrorType::invarg:
            return "Invalid Argument";
        case ErrorType::oor:
            return "Out-of-range    ";
        default:
            return "Unknown Error   ";
        }
    }

    inline void ThrowError(ErrorType type, const char* func, const std::string& msg)
    {
        std::string errMsg = std::string(func) + ": " + msg;
        Error(type, errMsg);
    }

    // helper macro to build string from expressions, e.g. THROWRE("Value: " <<
    // value);
#define BUILD_ERROR_MSG(expr)                                                                                          \
    (                                                                                                                  \
        [&]                                                                                                            \
        {                                                                                                              \
            std::ostringstream oss;                                                                                    \
            oss << expr;                                                                                               \
            return oss.str();                                                                                          \
        }())

#define THROWRE(msg)  err::ThrowError(err::ErrorType::runtime, __func__, BUILD_ERROR_MSG(msg))
#define THROWIA(msg)  err::ThrowError(err::ErrorType::invarg, __func__, BUILD_ERROR_MSG(msg))
#define THROWOOR(msg) err::ThrowError(err::ErrorType::oor, __func__, BUILD_ERROR_MSG(msg))

#define THROWRE_IF(expr, msg)                                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        if (expr)                                                                                                      \
            err::ThrowError(err::ErrorType::runtime, __func__, BUILD_ERROR_MSG(msg));                                  \
    } while (0)
#define THROWIA_IF(expr, msg)                                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        if (expr)                                                                                                      \
            err::ThrowError(err::ErrorType::invarg, __func__, BUILD_ERROR_MSG(msg));                                   \
    } while (0)
#define THROWOOR_IF(expr, msg)                                                                                         \
    do                                                                                                                 \
    {                                                                                                                  \
        if (expr)                                                                                                      \
            err::ThrowError(err::ErrorType::oor, __func__, BUILD_ERROR_MSG(msg));                                      \
    } while (0)

} // namespace err

//#undef BUILD_ERROR_MSG
