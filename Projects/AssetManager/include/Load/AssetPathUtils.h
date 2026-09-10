#pragma once

#include <filesystem>
#include <vector>

#include "Registry/AssetRegistry.h"

namespace Murder::AssetPathUtils
{
    namespace fs = std::filesystem;

    fs::path ResolveAssetPath(
        AssetType                  type,
        const fs::path&            path,
        const AssetRegistry::Dirs& dirs,
        const fs::path*            contextFile = nullptr);

    bool ReadFileBytes(const fs::path& path, std::vector<std::byte>& outBytes);
} // namespace Murder::AssetPathUtils
