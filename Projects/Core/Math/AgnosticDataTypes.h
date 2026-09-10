/**
* AgnosticDataTypes.h
*
* Data types used in the ObjLoader class for loading .obj files. Used to keep the data
* type agnostic and reusable across different frameworks (Direct3D, OpenGL) that use
* different maths libraries (DirectXMath, GLM, etc.).
*/
#pragma once

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

#include "AgnosticMath.h"

// stop min/max macros from messing things up
#undef min
#undef max

// ---------------------------------------------------------------------------------------------------------------------
static constexpr float FLOAT_MAX = std::numeric_limits<float>::max();
static constexpr float FLOAT_MIN = std::numeric_limits<float>::lowest();

// ---------------------------------------------------------------------------------------------------------------------
// Vector Data Types
// ---------------------------------------------------------------------------------------------------------------------
using index_16t = uint16_t;
using index_t   = uint32_t;
using index_t64 = uint64_t;

/**
 * @struct vec2f
 * 2D vector with float components
 */
struct vec2f
{
    float x, y;

    constexpr vec2f(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

    constexpr bool operator==(const vec2f& other) const
    {
        return mutil::float_equals(x, other.x) && mutil::float_equals(y, other.y);
    }

    static constexpr vec2f max() { return vec2f(FLOAT_MAX, FLOAT_MAX); }

    static constexpr vec2f min() { return vec2f(FLOAT_MIN, FLOAT_MIN); }
};

struct vec3f
{
    float x, y, z;

    constexpr vec3f(float x = 0.0f, float y = 0.0f, float z = 0.0f) : x(x), y(y), z(z) {}

    constexpr bool operator==(const vec3f& other) const
    {
        return mutil::float_equals(x, other.x) && mutil::float_equals(y, other.y) && mutil::float_equals(z, other.z);
    }

    // assignment
    constexpr vec3f& operator=(const vec3f& other)
    {
        if (this != &other)
        {
            x = other.x;
            y = other.y;
            z = other.z;
        }
        return *this;
    }

    static constexpr vec3f max() { return vec3f(FLOAT_MAX, FLOAT_MAX, FLOAT_MAX); }
    static constexpr vec3f min() { return vec3f(FLOAT_MIN, FLOAT_MIN, FLOAT_MIN); }

    constexpr vec3f operator+(const vec3f& other) const { return vec3f(x + other.x, y + other.y, z + other.z); }

    constexpr vec3f operator-(const vec3f& other) const { return vec3f(x - other.x, y - other.y, z - other.z); }

    constexpr vec3f operator*(float scalar) const { return vec3f(x * scalar, y * scalar, z * scalar); }

    constexpr vec3f operator/(float scalar) const
    {
        if (scalar == 0.0f)
            throw std::runtime_error("Division by zero in vec3f division");
        return vec3f(x / scalar, y / scalar, z / scalar);
    }

    constexpr vec3f operator-() const { return vec3f(-x, -y, -z); }

    constexpr float dot(const vec3f& other) const { return x * other.x + y * other.y + z * other.z; }

    constexpr vec3f cross(const vec3f& other) const
    {
        return vec3f(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x);
    }

    float lensq() const { return dot(*this); }

    float length() const { return mutil::sqrtf(lensq()); }

    vec3f normalized() const
    {
        const float len = length();
        if (mutil::float_equals(len, 0.0f))
            return vec3f(0.0f, 0.0f, 0.0f);
        return (*this) / len;
    }
};
struct vec4f
{
    float x, y, z, w;

    constexpr vec4f(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 1.0f) : x(x), y(y), z(z), w(w) {}

    constexpr bool operator==(const vec4f& other) const
    {
        return mutil::float_equals(x, other.x) && mutil::float_equals(y, other.y) && mutil::float_equals(z, other.z) &&
               mutil::float_equals(w, other.w);
    }

    static constexpr vec4f max() { return vec4f(FLOAT_MAX, FLOAT_MAX, FLOAT_MAX, FLOAT_MAX); }

    static constexpr vec4f min()
    {
        return vec4f(
            std::numeric_limits<float>::lowest(),
            std::numeric_limits<float>::lowest(),
            std::numeric_limits<float>::lowest(),
            std::numeric_limits<float>::lowest());
    }
};

inline std::string to_string(const vec2f& v) { return std::to_string(v.x) + std::to_string(v.y); }

inline std::string to_string(const vec3f& v)
{
    return std::to_string(v.x) + " " + std::to_string(v.y) + " " + std::to_string(v.z);
}

inline std::string to_string(const vec4f& v)
{
    return std::to_string(v.x) + " " + std::to_string(v.y) + " " + std::to_string(v.z) + " " + std::to_string(v.w);
}

namespace mathutil
{
    template <typename T>
    constexpr T clamp(T value, T min, T max)
    {
        return (value < min) ? min : (value > max) ? max : value;
    }

    constexpr vec2f clamp(const vec2f& value, const vec2f& min, const vec2f& max)
    {
        return vec2f(clamp(value.x, min.x, max.x), clamp(value.y, min.y, max.y));
    }

    constexpr vec3f clamp(const vec3f& value, const vec3f& min, const vec3f& max)
    {
        return vec3f(clamp(value.x, min.x, max.x), clamp(value.y, min.y, max.y), clamp(value.z, min.z, max.z));
    }

    constexpr vec4f clamp(const vec4f& value, const vec4f& min, const vec4f& max)
    {
        return vec4f(
            clamp(value.x, min.x, max.x),
            clamp(value.y, min.y, max.y),
            clamp(value.z, min.z, max.z),
            clamp(value.w, min.w, max.w));
    }
} // namespace mathutil
