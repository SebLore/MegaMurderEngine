#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace Core
{
    /**
     * @brief long long ID tag for loaded assets
     */
    using AssetId = uint64_t;

    inline constexpr uint64_t FNV_1A_OFFSET_BASIS = 0xCBF29CE484222325ull;
    inline constexpr uint64_t FNV_1A_PRIME        = 0x100000001b3ull;

    /// hashing string_view into AssetId using the FNV-1a algorithm
    /// more: https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
    [[nodiscard]] constexpr AssetId Hash(std::string_view str) noexcept
    {
        uint64_t hash = FNV_1A_OFFSET_BASIS;
        for (const unsigned char c : str)
        {
            // xor each byte with the hash and then multiply by the prime
            hash ^= static_cast<uint64_t>(c);
            hash *= FNV_1A_PRIME; // wraps 2^64 automatically, no need for modulo
        }
        return hash;
    }

    /// Convert AssetId to a hex string, easier to read than raw number
    [[nodiscard]] inline std::string ToHexString(AssetId id)
    {
        constexpr char digits[] = "0123456789ABCDEF";
        std::string    out(18, '0');
        out[0] = '0';
        out[1] = 'x';

        for (int i = 0; i < 16; ++i)
        {
            const uint64_t shift = static_cast<uint64_t>(60 - (i * 4));
            out[2 + i]           = digits[(id >> shift) & 0xF];
        }

        return out;
    }

} // namespace Core
