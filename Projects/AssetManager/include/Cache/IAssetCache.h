#pragma once

#include <Core/AssetId.h>
#include <memory>

namespace Murder
{
    /// @brief Typed base interface for asset caches
    template <typename AssetType>
    class IAssetCache
    {
      public:
        virtual ~IAssetCache() = default;

        /// Tries to get a cached asset
        /// @return Pointer if found, otherwise nullptr
        virtual AssetType* TryGet(Core::AssetId id) = 0;

        /// Tries to get a cached asset (const overload)
        /// @return Pointer if found, otherwise nullptr
        virtual const AssetType* TryGet(Core::AssetId id) const = 0;

        /// @brief Caches a loaded asset and transfers ownership
        virtual void Cache(Core::AssetId id, std::unique_ptr<AssetType> asset) = 0;

        /// @brief Clears all cached assets
        virtual void Clear() = 0;
    };
} // namespace Murder
