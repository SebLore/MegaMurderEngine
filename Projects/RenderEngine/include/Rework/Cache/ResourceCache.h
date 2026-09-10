#pragma once

#include "Core/AssetTags.h"

namespace Murder
{
    struct MeshGpu;

    template <class Type, class TypeId>
    class GpuResourceCache
    {
      public:
        virtual ~GpuResourceCache() = default;
        virtual Type* GetOrCreate(TypeId id) = 0;
    };

    using MeshCache = GpuResourceCache<MeshGpu, MeshId>;

} // namespace Murder