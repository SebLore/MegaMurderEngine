#pragma once

#include "Transform.h"

namespace Murder::TransformMath
{
    [[nodiscard]] inline Matrix World(const Transform& t)
    {
        return Matrix::CreateScale(t.scale) * Matrix::CreateFromQuaternion(t.rotation) *
               Matrix::CreateTranslation(t.position);
    }

    [[nodiscard]] inline Matrix WorldInverse(const Transform& t)
    {
        const Matrix world = World(t);
        return world.Invert();
    }

    [[nodiscard]] inline Matrix WorldInverseTranspose(const Transform& t)
    {
        // Good for transforming normals (especially with non-uniform scale).
        // With normals treated as direction vectors (w = 0), translation is ignored.
        const Matrix inv = WorldInverse(t);
        return inv.Transpose();
    }

    // assumes left-handed, i.e. Z 1 = forward

    [[nodiscard]] inline Vector3 Forward(const Transform& t)
    {
        return Vector3::Transform(Vector3(0, 0, 1), Matrix::CreateFromQuaternion(t.rotation));
    }

    [[nodiscard]] inline Vector3 Right(const Transform& t)
    {
        return Vector3::Transform(Vector3(1, 0, 0), Matrix::CreateFromQuaternion(t.rotation));
    }

    [[nodiscard]] inline Vector3 Up(const Transform& t)
    {
        return Vector3::Transform(Vector3(0, 1, 0), Matrix::CreateFromQuaternion(t.rotation));
    }
} // namespace Murder::TransformMath
