#pragma once

#include <Core/Math/CameraProjection.h>
#include <Core/Math/CameraMath.h>

namespace Murder
{
    struct Camera
    {
        Vector3    position = { 0, 0, 0 };
        Quaternion rotation = { 0, 0, 0, 1 };

        // stored for convenience when lookAt/To
        Vector3 worldUp = { 0, 1, 0 };

        // projection parameters, stored as a variant to allow different types of projection
        CameraProjection lens = PerspectiveProjection{};

      public:
        // direction vectors in world space, computed from rotation
        Vector3 Forward() const { return Vector3::Transform(Vector3(0, 0, 1), Matrix::CreateFromQuaternion(rotation)); }
        Vector3 Right() const { return Vector3::Transform(Vector3(1, 0, 0), Matrix::CreateFromQuaternion(rotation)); }
        Vector3 Up() const { return Vector3::Transform(Vector3(0, 1, 0), Matrix::CreateFromQuaternion(rotation)); }

        // matrix getters, creates
        Matrix View() const { return CameraMath::View(position, Forward(), Up()); }
        Matrix Projection() const { return CameraMath::Projection(lens); }
        Matrix ViewProjection() const { return View() * Projection(); }

        // convenience setters

        /// Sets the camera's orientation to look at the target point. Target cannot be camera position.
        void LookAt(const Vector3& target) { rotation = CameraMath::RotationFromLookAt(position, target, worldUp); }

        /// Sets the camera's orientation to look in the given direction. Direction cannot be zero vector.
        void LookTo(const Vector3& direction)
        {
            rotation = CameraMath::RotationFromLookTo(position, direction, worldUp);
        }

        // Lens setters
        void SetPerspective(float fovY, float aspectRatio, float nearZ, float farZ)
        {
            lens = CameraMath::EnsureValid(
                PerspectiveProjection{
                    .fovY        = fovY,
                    .aspectRatio = aspectRatio,
                    .nearZ       = nearZ,
                    .farZ        = farZ,
                });
        }
        void SetOrthographic(float width, float height, float nearZ, float farZ)
        {
            lens = CameraMath::EnsureValid(
                OrthographicProjection{
                    .width  = width,
                    .height = height,
                    .nearZ  = nearZ,
                    .farZ   = farZ,
                });
        }
        void SetOrthographicOffCenter(float left, float right, float bottom, float top, float nearZ, float farZ)
        {
            lens = CameraMath::EnsureValid(
                OrthographicOffCenterProjection{
                    .left   = left,
                    .right  = right,
                    .top    = top,
                    .bottom = bottom,
                    .nearZ  = nearZ,
                    .farZ   = farZ,
                });
        }

        [[nodiscard]] DirectX::BoundingFrustum Frustum() const
        {
            return CameraMath::ToBoundingFrustum(lens, position, rotation);
        }
    };

} // namespace Murder
