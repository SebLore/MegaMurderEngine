#include "Load/AssetPathUtils.h"

#include <fstream>

namespace Murder::AssetPathUtils
{
    fs::path
    ResolveAssetPath(AssetType type, const fs::path& path, const AssetRegistry::Dirs& dirs, const fs::path* contextFile)
    {
        namespace fs = fs;

        fs::path        resolved = path;
        std::error_code ec;

        if (fs::exists(resolved, ec))
            return resolved;

        const fs::path lookupPath = resolved.is_absolute() ? resolved.filename() : resolved;

        const fs::path root    = (type == AssetType::Shader) ? dirs.binRoot : dirs.assetsRoot;
        const fs::path rootAbs = fs::absolute(root).lexically_normal();
        const fs::path typeDir = AssetRegistry::DirForType(dirs, type);

        if (contextFile != nullptr)
        {
            const fs::path contextual = (contextFile->parent_path() / lookupPath).lexically_normal();
            if (fs::exists(contextual, ec))
                return contextual;
        }

        const fs::path typeRelative = (rootAbs / typeDir / lookupPath.filename()).lexically_normal();
        if (fs::exists(typeRelative, ec))
            return typeRelative;

        const fs::path rootRelative = (rootAbs / lookupPath).lexically_normal();
        if (fs::exists(rootRelative, ec))
            return rootRelative;

        if (contextFile != nullptr)
            return (contextFile->parent_path() / lookupPath).lexically_normal();

        return rootRelative;
    }

    bool ReadFileBytes(const fs::path& path, std::vector<std::byte>& outBytes)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
            return false;

        const std::streamsize size = file.tellg();
        if (size <= 0)
            return false;

        outBytes.resize(static_cast<size_t>(size));
        file.seekg(0, std::ios::beg);

        if (!file.read(reinterpret_cast<char*>(outBytes.data()), size))
        {
            outBytes.clear();
            return false;
        }

        return true;
    }
} // namespace Murder::AssetPathUtils
