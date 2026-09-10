#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <stdexcept>
#include <cstdint>

#include "AssetTraits.h"

namespace fs = std::filesystem;

class AssetRegistry
{
  public:
    /// filesystem directories, root should be relative to working directory of the app
    struct Dirs
    {
        // different roots for file assets vs binary files
        fs::path assetsRoot = "assets";
        fs::path binRoot    = ".";

        // under assetsRoot
        fs::path models    = "models";
        fs::path materials = "materials";
        fs::path textures  = "textures";

        // under binRoot or assetRoot, depends on file type
        fs::path shaders = "shaders";
    };

    enum class AssetSource : uint8_t
    {
        File,
        Virtual
    };

    // Public read-only metadata view (does not expose internal storage)
    struct AssetInfo
    {
        AssetType        type{};
        AssetSource      source{ AssetSource::File };
        std::string_view stringKey{}; // stable dedupe key (file or virtual)
        const fs::path*  fullPath{};  // nullptr if not file-backed
    };

  public:
    AssetRegistry()  = default;
    ~AssetRegistry() = default;

    // no copying
    AssetRegistry(const AssetRegistry&)            = delete;
    AssetRegistry& operator=(const AssetRegistry&) = delete;

    // default move for now
    AssetRegistry(AssetRegistry&&) noexcept            = default;
    AssetRegistry& operator=(AssetRegistry&&) noexcept = default;

    // -- Directory Management --
    void        SetDirs(Dirs d) { m_dirs = std::move(d); }
    const Dirs& GetDirs() const { return m_dirs; }

    static const fs::path& DirForType(const Dirs& dirs, AssetType type)
    {
        switch (type)
        {
        case AssetType::Mesh:
        case AssetType::Model:
            return dirs.models;
        case AssetType::Material:
            return dirs.materials;
        case AssetType::Texture:
            return dirs.textures;
        case AssetType::Shader:
            return dirs.shaders;
        default:
            return dirs.assetsRoot;
        }
    }

    // -- Registration (!= loading) --
    [[nodiscard]] MeshId    RegisterMesh(std::string_view relativePath) { return RegisterFile<MeshId>(relativePath); }
    [[nodiscard]] ModelId   RegisterModel(std::string_view relativePath) { return RegisterFile<ModelId>(relativePath); }
    [[nodiscard]] TextureId RegisterTexture(std::string_view relativePath)
    {
        return RegisterFile<TextureId>(relativePath);
    }
    [[nodiscard]] MaterialId RegisterMaterial(std::string_view relativePath)
    {
        return RegisterFile<MaterialId>(relativePath);
    }
    [[nodiscard]] ShaderId RegisterShader(std::string_view relativePath)
    {
        return RegisterFile<ShaderId>(relativePath);
    }

    /// Register a string, hash&cash, return the hashed asset id of the correct type (MeshId, MaterialId)
    template <class Id>
    Id RegisterFile(std::string_view relativePath)
    {
        const auto&   base = BaseDir(AssetTraits<Id>::type);
        Core::AssetId raw  = RegisterInternalFile(AssetTraits<Id>::type, base, relativePath);
        return Id{ raw }; // convert raw id to the correct tagged id
    }

    /// Register a memory asset with alias, like textures created in memory that we still want to register and cache.
    template <class Id>
    Id RegisterMemory(std::string_view alias)
    {
        Core::AssetId raw = RegisterInternalMemory(AssetTraits<Id>::type, alias);
        return Id{ raw };
    }

    // -- Raw getters --

    /**
     * @brief Get the full path for the asset, throws if the id doesn't exist or if the asset is not file-backed (i.e. virtual).
     * @param id the id being looked up
     * @return the full path for the asset, if the asset is file-backed
     * @throws std::runtime_error if the id doesn't exist or if the asset is not file-backed (i.e. virtual).
     */
    const fs::path& GetFullPath(Core::AssetId id) const;

    /**
     * @brief Try to get the full path for the asset, returns nullptr if the id doesn't exist or if the asset is not file-backed (i.e. virtual).
     * @param id the id being looked up
     * @return pointer to the full path for the asset, if the asset is file-backed, nullptr otherwise
     */
    const fs::path* TryGetFullPath(Core::AssetId id) const noexcept;

    // Entry getters
    /// returns a string_view of the normalized key matching the id if one exists, throwing if it doesn't
    std::string_view GetNormalizedKey(Core::AssetId id) const { return GetEntry(id).stringKey; }
    AssetType        GetType(Core::AssetId id) const { return GetEntry(id).type; }
    AssetSource      GetSource(Core::AssetId id) const { return GetEntry(id).source; }

    // for when we want more info about the asset
    AssetInfo GetInfo(Core::AssetId id) const;
    bool      TryGetInfo(Core::AssetId id, AssetInfo& out) const;

    template <class Fn>
    void ForEach(const Fn& fn) const
    {
        for (const auto& [id, e] : m_entries)
            fn(id, GetInfo(e));
    }

    template <class Fn>
    void ForEach(AssetType type, const Fn& fn) const
    {
        for (const auto& [id, e] : m_entries)
            if (e.type == type)
                fn(id, GetInfo(e));
    }

    // -- Typed getters --

    /// Get the full, absolute path for the id's asset
    template <class Id>
    const fs::path& GetFullPath(Id id) const
    {
        const auto& e = GetEntryTyped(AssetTraits<Id>::type, id.value);
        if (e.source != AssetSource::File)
            throw std::runtime_error("GetFullPath: asset is not file-backed");
        return e.fullPath;
    }

    template <class Id>
    std::string_view GetNormalizedKey(Id id) const
    {
        return GetEntryTyped(AssetTraits<Id>::type, id.value).stringKey;
    }

    /// Clear all entries and keys
    void Clear()
    {
        m_entries.clear();
        m_keysToIds.clear();
    }

  private:
    /// Small struct for managing asset string entries
    struct Entry
    {
        AssetType   type{};
        AssetSource source = AssetSource::File;
        std::string stringKey; // normalized key for dedupe + hashing
        fs::path    fullPath;  // root/base/relative path, only when source == file
    };
    static AssetInfo GetInfo(const Entry& e)
    {
        return AssetInfo{
            .type      = e.type,
            .source    = e.source,
            .stringKey = e.stringKey,
            .fullPath  = (e.source == AssetSource::File) ? &e.fullPath : nullptr,
        };
    }

  private:
    /// Get entry for the id, checking that it exists and is the expected type. Should be used for external getters since we can't guarantee type correctness internally
    const Entry& GetEntryTyped(AssetType expected, Core::AssetId id) const;
    /// Get entry for the id, throws if it doesn't. We don't care about the type internally. Use GetEntryTyped for external getters
    const Entry& GetEntry(Core::AssetId id) const;
    /// Returns a pointer to an Entry if it exists, otherwise returns nullptr.
    const Entry* FindEntry(Core::AssetId id) const noexcept;
    /// Get the base directory path for the type of asset (i.e. BaseDir(Mesh) -> "../assets/meshes" (or absolute)
    const fs::path& BaseDir(AssetType type) const;

    bool TryGetExistingId(const std::string& key, AssetType type, Core::AssetId& out) const;
    bool TryReuseExistingId(const std::string& key, AssetType type, Core::AssetId id, Core::AssetId& out);

    /**
     * @brief Hash the path, return an AssetId for the hashed path. Used for file-backed assets, the path is used for deduping and type checking.
     * @param type the type of the asset, used for type checking and base dir resolution
     * @param baseDir the base directory for the asset type, used for resolving the full path and normalized key
     * @param relativePath the user-provided path, can be absolute or relative to the base dir, used for resolving the full path and normalized key
     * @return AssetId hashed from the path
     * @note This is hashing only, no loading of files actually happens here. AssetId will always be valid.
     */
    Core::AssetId RegisterInternalFile(AssetType type, const fs::path& baseDir, std::string_view relativePath);

    /**
     * @brief Hash the alias, return an AssetId for the hashed id. Used for assets that are created in memory but we still want to register and cache with a string key.
     * @param type the type of the asset, used for type checking and base dir resolution
     * @param alias the user-provided alias, used for resolving the normalized key
     * @return AssetId hashed from the alias
     */
    Core::AssetId RegisterInternalMemory(AssetType type, std::string_view alias);

  private:
    Dirs m_dirs;

    std::unordered_map<Core::AssetId, Entry>       m_entries;   // id -> entry
    std::unordered_map<std::string, Core::AssetId> m_keysToIds; // stringKey -> id (dedupe)
};
