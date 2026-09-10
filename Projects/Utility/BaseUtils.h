#pragma once

#include <stdexcept>
#include <string>

// determine the C++ version being used
#ifndef UTILS_CXX_VER
#if defined(_MSVC_LANG)
#define UTILS_CXX_VER _MSVC_LANG
#else
#define UTILS_CXX_VER __cplusplus
#endif
#endif

// define include parameters based on C++ version

#if UTILS_CXX_VER >= 201703L
#define UTILS_USE_STRING_VIEW
#else
#define UTILS_USE_STRING
#endif

#if defined(UTILS_USE_STRING_VIEW)
#include <string_view>
#endif

// determine encoding
// Check for unicode (wide char) settings to enable/disable unicode support
#if defined(UNICODE) || defined(_UNICODE)
#ifndef UTILS_UNICODE
#define UTILS_UNICODE
#endif
#else
#ifndef UTILS_ANSI
#define UTILS_ANSI
#endif
#endif

namespace utils
{

    // Aliases for now TODO: implement own exception handling at some point
    using MyExceptionType = std::exception;        // base exception type
    using MyRuntimeErr    = std::runtime_error;    // runtime error
    using MyInvalidArgErr = std::invalid_argument; // invalid argument error
    using MyOutOfRangeErr = std::out_of_range;     // out of range error

} // namespace utils
