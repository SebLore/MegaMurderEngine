#pragma once

#include <cmath>
#include <limits>

/**
 * @brief Safe compare function for floats
 * @param a float
 * @param b float
 * @return bool if the difference is smaller than epsilon (very small number)
 */

namespace mutil
{
	constexpr float FLOAT_EPSILON = std::numeric_limits<float>::epsilon();
	constexpr float PI            = 3.14159265358979323846f;
	constexpr float TWO_PI        = 6.28318530717958647692f;

	constexpr float fabsf(float a)
	{
		return (a < 0.0f) ? -a : a;
	}

	inline float sqrtf(float a) { return std::sqrt(a); }
	inline float sinf(float a) { return std::sin(a); }
	inline float cosf(float a) { return std::cos(a); }

	/// @brief returns true if a and b are equal, taking into account floating point precision issues
        constexpr bool float_equals(float a, float b) { return fabsf(a - b) < std::numeric_limits<float>::epsilon(); }

        /// @brief returns true if a is greater than b, taking into account floating point precision issues
	constexpr bool float_greaterthan(float a, float b)
	{
		return (a - b) > std::numeric_limits<float>::epsilon();
	}
	/// @brief returns true if a is less than b, taking into account floating point precision issues
	constexpr bool float_lessthan(float a, float b)
	{
		return (b - a) > std::numeric_limits<float>::epsilon();
	}

	/// @brief returns a clamped float value between min and max
	static constexpr float clamp(float n, float min, float max)
	{
		if (n < min)
			return min;
		if (n > max)
			return max;
		return n;
	}

	/**
	 * @brief Performs linear interpolation between two floating-point values.
	 * @param a The start value.
	 * @param b The end value.
	 * @param t The interpolation factor, typically in the range [0, 1]
	 * @return The interpolated value between a and b based on t.
	 */
	constexpr float lerp(float a, float b, float t)
	{
		return a + (b - a) * t;
	}


}