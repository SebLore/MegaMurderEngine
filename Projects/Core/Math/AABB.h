#pragma once

#include "../Base/Common.h"

namespace Murder
{
    struct AABB
    {
        Vector3 min = { std::numeric_limits<float>::max(),
                        std::numeric_limits<float>::max(),
                        std::numeric_limits<float>::max() };

        Vector3 max = { -std::numeric_limits<float>::max(),
                        -std::numeric_limits<float>::max(),
                        -std::numeric_limits<float>::max() };

        [[nodiscard]] bool IsValid() const { return min.x <= max.x && min.y <= max.y && min.z <= max.z; }

        void Reset()
        {
            min = Vector3{ std::numeric_limits<float>::max(),
                           std::numeric_limits<float>::max(),
                           std::numeric_limits<float>::max() };
            max = Vector3{ -std::numeric_limits<float>::max(),
                           -std::numeric_limits<float>::max(),
                           -std::numeric_limits<float>::max() };
        }

        void Expand(const Vector3& p)
        {
            min.x = (std::min)(min.x, p.x);
            min.y = (std::min)(min.y, p.y);
            min.z = (std::min)(min.z, p.z);

            max.x = (std::max)(max.x, p.x);
            max.y = (std::max)(max.y, p.y);
            max.z = (std::max)(max.z, p.z);
        }

        void Expand(const AABB& other)
        {
            if (!other.IsValid())
                return;

            Expand(other.min);
            Expand(other.max);
        }

        [[nodiscard]] Vector3 Center() const { return (min + max) * 0.5f; }

        [[nodiscard]] Vector3 Extents() const { return (max - min) * 0.5f; }

        [[nodiscard]] Vector3 Size() const { return (max - min); }

        [[nodiscard]] bool Contains(const Vector3& p) const
        {
            return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y && p.z >= min.z && p.z <= max.z;
        }

        [[nodiscard]] bool Contains(const AABB& other) const
        {
            if (!IsValid() || !other.IsValid())
                return false;

            return other.min.x >= min.x && other.max.x <= max.x && other.min.y >= min.y && other.max.y <= max.y &&
                   other.min.z >= min.z && other.max.z <= max.z;
        }
    };

    [[nodiscard]] inline AABB AABBFromCenterExtents(const Vector3& center, const Vector3& extents)
    {
        AABB box;
        box.min = center - extents;
        box.max = center + extents;
        return box;
    }
} // namespace Murder
