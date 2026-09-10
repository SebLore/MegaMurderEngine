#pragma once

#include "../Base/Common.h"

namespace Murder
{
    struct Transform
    {
        Vector3    position{ 0, 0, 0 };
        Quaternion rotation{ 0, 0, 0, 1 };
        Vector3    scale{ 1, 1, 1 };
    };

    struct Transform2D
    {
        Vector2 position = { 0.0f, 0.0f }; ///< position in screen coordinates
        Vector2 scale = { 1.0f, 1.0f };
        float   rotation = 0.0f;
    };


    namespace TransformMath
    {
        Matrix World(const Transform& t);
        Matrix WorldInverse(const Transform& t);
        Matrix WorldInverseTranspose(const Transform& t);

        Vector3 Forward(const Transform& t);
        Vector3 Right(const Transform& t);
        Vector3 Up(const Transform& t);
    } // namespace TransformMath

} // namespace Murder
