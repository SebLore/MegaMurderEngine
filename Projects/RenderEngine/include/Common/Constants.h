/*
 * Constants.h
 *
 * Global constants for the entire proejct.
 */
#pragma once

#include <limits>
#undef max
#undef min

// constants
namespace constants
{
    // default window settings
    constexpr unsigned int WINDOW_DEFAULT_WIDTH    = 800;
    constexpr unsigned int WINDOW_DEFAULT_HEIGHT   = 600;
    constexpr float        WINDOW_DEFAULT_WIDTH_F  = 800.0f;
    constexpr float        WINDOW_DEFAULT_HEIGHT_F = 600.0f;
    constexpr unsigned int WINDOW_DEFAULT_X        = 100;
    constexpr unsigned int WINDOW_DEFAULT_Y        = 100;

    constexpr float FLOAT_MAX = std::numeric_limits<float>::max();
    constexpr float FLOAT_MIN = std::numeric_limits<float>::lowest();
} // namespace constants

// constants is long to type
namespace cts = constants;
