#include "AssetRegistry.h"

#include <Utility/ErrorHandling.h>

namespace
{
    fs::path JoinAndNormalizeFullPath(const fs::path& root, const fs::path& baseDir, const fs::path& userPath)
    {
        fs::path full = userPath.is_absolute() ? userPath : (root / baseDir / userPath);
        full          = full.lexically_normal();

        return fs::absolute(full).lexically_normal();
    }

    std::string NormalizePath(const fs::path& input, const fs::path& rootDir)
    {
        fs::path p = input;

        // Interpret relative paths relative to rootDir
        if (p.is_relative())
            p = rootDir / p;

        p = p.lexically_normal();

        fs::path rel;
        try
        {
            rel = fs::relative(p, rootDir);
        }
        catch (...) // unexpected
        {
            rel = p;
        }

        std::string s = rel.generic_string();

#ifdef _WIN32
        for (char& c : s)
            if (c >= 'A' && c <= 'Z')
                c = static_cast<char>(c - 'A' + 'a');
#endif

        return s;
    }

    // flatten the string
    std::string NormalizeAlias(std::string_view s)
    {
        std::string out(s);

#ifdef _WIN32
        for (char& c : out)
            if (c >= 'A' && c <= 'Z')
                c = static_cast<char>(c - 'A' + 'a');
#endif

        return out;
    }

    /// Helper to get the asset type as a string
    const char* TypeName(AssetType t)
    {
        switch (t)
        {
        case AssetType::Mesh:
            return "mesh";
        case AssetType::Texture:
            return "texture";
        case AssetType::Material:
            return "material";
        case AssetType::Model:
            return "model";
        case AssetType::Shader:
            return "shader";
        }
        return "unknown";
    }

} // namespace

Core::AssetId
AssetRegistry::RegisterInternalFile(AssetType type, const fs::path& baseDir, std::string_view relativePath)
{
    THROWIA_IF(relativePath.empty(), "path was empty");

    // remember the relative path provided by the user, i.e. "mymesh.fbx"
    const fs::path user = fs::path(relativePath);

    // make root absolute and normalized once to ensure stability
    const fs::path& root    = (type == AssetType::Shader) ? m_dirs.binRoot : m_dirs.assetsRoot;
    const fs::path  rootAbs = fs::absolute(root).lexically_normal();

    // Build full path  'root/base/relative'
    const fs::path full = JoinAndNormalizeFullPath(rootAbs, baseDir, user);

    // Next, we want to create a normalized key, it works better with caching and hashing
    const fs::path keyInput = user.is_absolute() ? user : (baseDir / user);

    const std::string rel = NormalizePath(keyInput, rootAbs);

    const std::string mount      = (type == AssetType::Shader) ? "bin://" : "assets://";
    const std::string normalized = mount + TypeName(type) + "/" + rel;

    // Deduping: the same key should give the same id
    Core::AssetId existing{};
    if (TryGetExistingId(normalized, type, existing))
        return existing;

    // Hash the normalized key to get the asset id
    const Core::AssetId id = Core::Hash(normalized);

    // One more collision check/re-registering safety so we don't reuse/overwrite an existing id
    if (TryReuseExistingId(normalized, type, id, existing))
        return existing;

    // store it as a path entry for reverse lookups and type checking
    m_entries.emplace(
        id,
        Entry{ .type = type, .source = AssetSource::File, .stringKey = normalized, .fullPath = full });

    // Reverse lookup ids with normalized key
    m_keysToIds.emplace(normalized, id);

    // return the hashed id
    return id;
}

Core::AssetId AssetRegistry::RegisterInternalMemory(AssetType type, std::string_view alias)
{
    if (alias.empty())
        THROWIA("empty alias");

    const std::string a = NormalizeAlias(alias);

    // Stable dedupe key (cannot collide with file keys)
    const std::string key = std::string("virtual://") + TypeName(type) + "/" + a;

    // look for existing key
    Core::AssetId existing{};
    if (TryGetExistingId(key, type, existing))
        return existing;

    // no dupe, hash for asset id
    const Core::AssetId id = Core::Hash(key);
    if (TryReuseExistingId(key, type, id, existing))
        return existing;

    // create stored entry
    Entry e{};
    e.type      = type;
    e.source    = AssetSource::Virtual;
    e.stringKey = key;
    e.fullPath.clear();

    m_entries.emplace(id, std::move(e));
    m_keysToIds.emplace(key, id);
    return id;
}
// -- Getters --

const fs::path& AssetRegistry::GetFullPath(Core::AssetId id) const
{
    const auto& e = GetEntry(id);
    if (e.source != AssetSource::File)
        THROWRE("asset is not file-backed");
    return e.fullPath;
}

const fs::path* AssetRegistry::TryGetFullPath(Core::AssetId id) const noexcept
{
    const Entry* e = FindEntry(id);
    if (!e)
        return nullptr;
    if (e->source != AssetSource::File)
        return nullptr;
    return &e->fullPath;
}

AssetRegistry::AssetInfo AssetRegistry::GetInfo(Core::AssetId id) const
{
    const auto& e = GetEntry(id);
    return AssetInfo{ .type      = e.type,
                      .source    = e.source,
                      .stringKey = e.stringKey,
                      .fullPath  = (e.source == AssetSource::File) ? &e.fullPath : nullptr };
}

// Recommended header change: bool TryGetInfo(Core::AssetId, AssetInfo& out) const;
bool AssetRegistry::TryGetInfo(Core::AssetId id, AssetInfo& out) const
{
    const Entry* e = FindEntry(id);
    if (!e)
        return false;

    out = AssetInfo{ .type      = e->type,
                     .source    = e->source,
                     .stringKey = e->stringKey,
                     .fullPath  = (e->source == AssetSource::File) ? &e->fullPath : nullptr };
    return true;
}

// ==| Private Functions |=========================================================================

const AssetRegistry::Entry& AssetRegistry::GetEntryTyped(AssetType expected, Core::AssetId id) const
{
    const Entry& e = GetEntry(id);
    if (e.type != expected)
        THROWRE("Asset type mismatch");
    return e;
}

const AssetRegistry::Entry& AssetRegistry::GetEntry(Core::AssetId id) const
{
    const Entry* e = FindEntry(id);
    if (!e)
        THROWRE("Unknown asset id");
    return *e;
}

const AssetRegistry::Entry* AssetRegistry::FindEntry(Core::AssetId id) const noexcept
{
    auto it = m_entries.find(id);
    if (it == m_entries.end())
        return nullptr;
    return &it->second;
}

bool AssetRegistry::TryGetExistingId(const std::string& key, AssetType type, Core::AssetId& out) const
{
    auto it = m_keysToIds.find(key);
    if (it == m_keysToIds.end())
        return false;

    out = it->second;
    if (const auto& entry = GetEntry(out); entry.type != type)
        THROWRE("key registered under different AssetType");

    return true;
}

bool AssetRegistry::TryReuseExistingId(const std::string& key, AssetType type, Core::AssetId id, Core::AssetId& out)
{
    auto it = m_entries.find(id);
    if (it == m_entries.end())
        return false;

    if (it->second.stringKey != key)
        THROWRE("AssetId hash collision");
    if (it->second.type != type)
        THROWRE("AssetId reused with different AssetType");

    m_keysToIds.emplace(key, id);
    out = id;
    return true;
}

const fs::path& AssetRegistry::BaseDir(AssetType type) const
{
    switch (type)
    {
    case AssetType::Mesh:
        return m_dirs.models;
    case AssetType::Texture:
        return m_dirs.textures;
    case AssetType::Material:
        return m_dirs.materials;
    case AssetType::Shader:
        return m_dirs.shaders;
    case AssetType::Model:
        return m_dirs.models;
    }
    THROWRE("Unknown AssetType");
    return m_dirs.assetsRoot; // to silence compiler, should never get here
}
