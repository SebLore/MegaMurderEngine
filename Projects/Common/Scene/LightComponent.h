#pragma once

#include <cstdint>
#include <variant>

#include <Core/MurderCore.hpp>          // Vector3, etc.
#include <Core/Math/CameraProjection.h> // CameraProjection, Ortho/Perspective projections

namespace Murder
{

    enum class LightType : uint8_t
    {
        DIRECTIONAL = 0,
        POINT,
        SPOT
    };

    static constexpr const char* ToString(LightType t)
    {
        switch (t)
        {
        case LightType::DIRECTIONAL:
            return "Directional Light";
        case LightType::POINT:
            return "Point Light";
        case LightType::SPOT:
            return "Spot Light";
        default:
            return "Unknown Light Type";
        }
    }

    /// Common shadow metadata (CPU-side only, no GPU resources)
    struct ShadowSettings
    {
        bool     enabled    = true;
        uint32_t mapSize    = 2048; // shadow map resolution, size x size
        float    depthBias  = 0.0f; ///< depth bias to reduce shadow acne, in world units (e.g. meters)
        float    normalBias = 0.0f; ///< normal bias to reduce peter panning, in world units (e.g. meters)

        // Directional lights generally use ortho/off-center.
        // Spot lights generally use perspective.
        // Point lights may ignore this for cubemap shadows later.
        CameraProjection projection = OrthographicProjection{};
    };

    struct DirectionalLight
    {
        Vector3 color     = { 1.0f, 1.0f, 1.0f };
        Vector3 direction = { 0.0f, -1.0f, 0.0f }; // normalized preferred

        float intensity = 1.0f;

        bool dynamic = false; // whether the light can be expected to move and update, possibly unnecessary

        ShadowSettings shadow{};
    };

    struct PointLight
    {
        Vector3 color    = { 1.0f, 1.0f, 1.0f };
        Vector3 position = { 0.0f, 0.0f, 0.0f };

        float intensity = 1.0f;
        float minRange  = 0.1f;
        float maxRange  = 10.0f;

        bool dynamic     = false;
        bool castsShadow = false; // point shadows later (cubemap)
    };

    struct SpotLight
    {
        Vector3 color     = { 1.0f, 1.0f, 1.0f };
        Vector3 position  = { 0.0f, 0.0f, 0.0f };
        Vector3 direction = { 0.0f, -1.0f, 0.0f }; // normalized preferred

        float intensity = 1.0f;
        float minRange  = 0.1f;
        float maxRange  = 20.0f;

        float innerAngleRadians = ToRadians(15.0f);
        float outerAngleRadians = ToRadians(30.0f); ///< fov for the light's camera should be x2 this to match the cone

        bool dynamic = false;

        ShadowSettings shadow{};
    };

    using Light = std::variant<DirectionalLight, PointLight, SpotLight>;

    namespace LightMath
    {
        inline Vector3 NormalizeSafe(Vector3 v, Vector3 fallback = { 0.0f, -1.0f, 0.0f })
        {
            if (v.LengthSquared() <= EPS)
                return fallback;
            v.Normalize();
            return v;
        }

        inline void SetDirection(DirectionalLight& l, const Vector3& dir) { l.direction = NormalizeSafe(dir); }

        inline void SetDirection(SpotLight& l, const Vector3& dir) { l.direction = NormalizeSafe(dir); }

        inline void LookAt(DirectionalLight& l, const Vector3& fromPosition, const Vector3& target)
        {
            l.direction = NormalizeSafe(target - fromPosition);
        }

        inline void LookAt(SpotLight& l, const Vector3& target) { l.direction = NormalizeSafe(target - l.position); }

        inline void EnsureValid(PointLight& l)
        {
            l.minRange = std::max(EPS, l.minRange);
            l.maxRange = std::max(l.minRange + EPS, l.maxRange);
        }

        inline void EnsureValid(DirectionalLight& l)
        {
            l.direction         = NormalizeSafe(l.direction);
            l.intensity         = std::max(0.0f, l.intensity);
            l.shadow.depthBias  = std::max(0.0f, l.shadow.depthBias);
            l.shadow.normalBias = std::max(0.0f, l.shadow.normalBias);
            l.shadow.mapSize    = std::max<uint32_t>(1, l.shadow.mapSize);
        }

        /// Sets the spotlight's parameters to be valid
        inline void EnsureValid(SpotLight& l)
        {
            l.direction = NormalizeSafe(l.direction);
            l.intensity = std::max(0.0f, l.intensity);
            l.minRange  = std::max(EPS, l.minRange);
            l.maxRange  = std::max(l.minRange + EPS, l.maxRange);

            l.innerAngleRadians = std::clamp(l.innerAngleRadians, BIG_EPS, ToRadians(179.0f));
            l.outerAngleRadians = std::clamp(l.outerAngleRadians, l.innerAngleRadians + BIG_EPS, ToRadians(179.0f));

            l.shadow.depthBias  = std::max(0.0f, l.shadow.depthBias);
            l.shadow.normalBias = std::max(0.0f, l.shadow.normalBias);
            l.shadow.mapSize    = std::max<uint32_t>(1, l.shadow.mapSize);
        }

        /// Get the LightType of a Light variant
        inline LightType TypeOf(const Light& light)
        {
            return std::visit(
                []<typename LightKind>(const LightKind& l) -> LightType
                {
                    using T = std::decay_t<LightKind>;
                    if constexpr (std::is_same_v<T, DirectionalLight>)
                        return LightType::DIRECTIONAL;
                    if constexpr (std::is_same_v<T, PointLight>)
                        return LightType::POINT;
                    if constexpr (std::is_same_v<T, SpotLight>)
                        return LightType::SPOT;

                    // Fallback for unexpected variant alternatives.
                    return LightType::DIRECTIONAL;
                },
                light);
        }
    } // namespace LightMath

} // namespace Murder
