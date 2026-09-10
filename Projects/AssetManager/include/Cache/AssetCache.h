#pragma once

#include "IAssetCache.h"

#include <memory>
#include <unordered_map>

#include <Common/CpuAssets.hpp>

namespace Murder
{
    /// Generic typed asset cache implementing IAssetCache<T>
    template <typename AssetType>
    class AssetCache : public IAssetCache<AssetType>
    {
      public:
        /// Default virtual destructor
        ~AssetCache() override = default;

        /// Tries to get a typed asset pointer from cache
        /// @return Pointer if found, otherwise nullptr
        AssetType* TryGet(Core::AssetId id) override
        {
            const auto it = m_Cache.find(id);
            return it != m_Cache.end() ? it->second.get() : nullptr;
        }

        /// Tries to get a const typed asset pointer from cache
        /// @return Pointer if found, otherwise nullptr
        const AssetType* TryGet(Core::AssetId id) const override
        {
            const auto it = m_Cache.find(id);
            return it != m_Cache.end() ? it->second.get() : nullptr;
        }

        /// Stores a typed asset using unique ownership
        void Cache(Core::AssetId id, std::unique_ptr<AssetType> asset) override { m_Cache[id] = std::move(asset); }

        /// Clears all cached assets
        void Clear() override { m_Cache.clear(); }

      private:
        std::unordered_map<Core::AssetId, std::unique_ptr<AssetType>> m_Cache;
    };

    /// Type aliases for specific asset caches
    using MeshCache     = AssetCache<MeshAsset>;
    using ModelCache    = AssetCache<ModelAsset>;
    using TextureCache  = AssetCache<TextureAsset>;
    using MaterialCache = AssetCache<MaterialAsset>;
    using ShaderCache   = AssetCache<ShaderAsset>;
    /// TODO: add more as needed

} // namespace Murder
