#pragma once

#include <algorithm>
#include <cmath>
#include <type_traits>

#include "Types.h"
#include "CameraProjection.h"

namespace Murder::CameraMath
{
    namespace detail
    {
        // clang-format off
        inline constexpr float kEps = 1e-6f; // small epsilon to prevent division by zero
        inline constexpr float kMaxAspectRatio = 1000.0f; // maximum aspect ratio to avoid extreme projections
        inline constexpr float kMinFovDegrees = 1.0f; // minimum FOV in degrees
        inline constexpr float kMaxFovDegrees = 179.0f; // maximum FOV in degrees
        inline constexpr float kNearParallel = 0.999f; // threshold for considering vectors as near-parallel
        // clang-format on

        [[nodiscard]] inline float ClampFovRadians(float fovY) noexcept
        {
            // Clamp FOV to a reasonable range (e.g., 1 degree to 179 degrees in radians)
            constexpr float minFov = DirectX::XMConvertToRadians(kMinFovDegrees);
            constexpr float maxFov = DirectX::XMConvertToRadians(kMaxFovDegrees);
            return std::clamp(fovY, minFov, maxFov);
        }

    } // namespace detail

    inline PerspectiveProjection EnsureValid(PerspectiveProjection p)
    {
        p.fovY        = detail::ClampFovRadians(p.fovY);
        p.aspectRatio = std::clamp(p.aspectRatio, detail::kEps, detail::kMaxAspectRatio);
        p.nearZ       = (std::max)(detail::kEps, p.nearZ);
        p.farZ        = (std::max)(p.nearZ + detail::kEps, p.farZ);
        return p;
    }

    inline OrthographicProjection EnsureValid(OrthographicProjection p)
    {
        p.width  = (std::max)(detail::kEps, p.width);
        p.height = (std::max)(detail::kEps, p.height);
        p.nearZ  = (std::max)(detail::kEps, p.nearZ);
        p.farZ   = (std::max)(p.nearZ + detail::kEps, p.farZ);
        return p;
    }

    inline OrthographicOffCenterProjection EnsureValid(OrthographicOffCenterProjection p)
    {
        if (p.left > p.right)
            std::swap(p.left, p.right);
        if (p.bottom > p.top)
            std::swap(p.bottom, p.top);

        if ((p.right - p.left) < detail::kEps)
        {
            const float mid = (p.left + p.right) * 0.5f;
            p.left          = mid - detail::kEps * 0.5f;
            p.right         = mid + detail::kEps * 0.5f;
        }

        if ((p.top - p.bottom) < detail::kEps)
        {
            const float mid = (p.bottom + p.top) * 0.5f;
            p.bottom        = mid - detail::kEps * 0.5f;
            p.top           = mid + detail::kEps * 0.5f;
        }

        p.nearZ = (std::max)(detail::kEps, p.nearZ);
        p.farZ  = (std::max)(p.nearZ + detail::kEps, p.farZ);
        return p;
    }

    inline Vector3 NormalizeSafe(Vector3 v, Vector3 fallback = Vector3(0, 0, 1))
    {
        const float lengthSq = v.LengthSquared();
        if (lengthSq <= detail::kEps)
            return fallback;
        v.Normalize();
        return v;
    }

    inline Quaternion RotationFromLookTo(const Vector3& eye, const Vector3& direction, const Vector3& worldUp)
    {
        using namespace DirectX;

        Vector3 fwd = NormalizeSafe(direction, { 0, 0, 1 });
        Vector3 up  = NormalizeSafe(worldUp, { 0, 1, 0 });

        // Handle near-parallel up/forward
        const float d = std::abs(fwd.Dot(up));

        // If forward and up are nearly parallel, choose a different up vector to avoid gimbal lock
        if (d > detail::kNearParallel)
            up = (std::abs(fwd.y) < detail::kNearParallel) ? Vector3(0, 1, 0) : Vector3(1, 0, 0);

        // Build LH view, invert to camera world transform, extract rotation
        XMMATRIX view  = XMMatrixLookToLH(eye, fwd, up);
        XMMATRIX world = XMMatrixInverse(nullptr, view);

        Quaternion q = Quaternion::CreateFromRotationMatrix(Matrix(world));
        q.Normalize();

        return q;
    }

    inline Quaternion RotationFromLookAt(const Vector3& eye, const Vector3& target, const Vector3& worldUp)
    {
        return RotationFromLookTo(eye, target - eye, worldUp);
    }

    [[nodiscard]] inline Matrix View(Vector3 eye, Vector3 forward, Vector3 up)
    {

        return Matrix(DirectX::XMMatrixLookToLH(eye, forward, up));
    }

    [[nodiscard]] inline Matrix Projection(const CameraProjection& lens)
    {
        return std::visit(
            [](const auto& p) -> Matrix
            {
                using Lens = std::decay_t<decltype(p)>;

                if constexpr (std::is_same_v<Lens, PerspectiveProjection>)
                {
                    const auto v = EnsureValid(p);
                    return Matrix(DirectX::XMMatrixPerspectiveFovLH(v.fovY, v.aspectRatio, v.nearZ, v.farZ));
                }
                else if constexpr (std::is_same_v<Lens, OrthographicProjection>)
                {
                    const auto v = EnsureValid(p);
                    return Matrix(DirectX::XMMatrixOrthographicLH(v.width, v.height, v.nearZ, v.farZ));
                }
                else if constexpr (std::is_same_v<Lens, OrthographicOffCenterProjection>)
                {
                    const auto v = EnsureValid(p);
                    return Matrix(
                        DirectX::XMMatrixOrthographicOffCenterLH(v.left, v.right, v.bottom, v.top, v.nearZ, v.farZ));
                }
                else
                {
                    static_assert(!sizeof(Lens), "Unknown lens type");
                }
                return Matrix{}; // should never reach here
            },
            lens);
    }

    [[nodiscard]] inline DirectX::BoundingFrustum
    ToBoundingFrustum(const CameraProjection& lens, const Vector3& position, const Quaternion& rotation)
    {
        DirectX::BoundingFrustum local;
        DirectX::BoundingFrustum::CreateFromMatrix(local, Projection(lens));

        DirectX::BoundingFrustum world;
        local.Transform(world, 1.0f, rotation, position);
        return world;
    }
} // namespace Murder::CameraMath
