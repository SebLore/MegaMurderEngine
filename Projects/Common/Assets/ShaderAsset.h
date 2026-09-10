// ReSharper disable CppClangTidyPerformanceEnumSize
#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <cstddef>

#include <Core/AssetTags.h>
#include <Common/Assets/ShaderStages.h>

namespace Murder
{
    enum class ShaderSourceKind : uint8_t
    {
        HLSL, // source file
        CSO   // pre-compiled
    };

    // uint32_t to match D3D UINT compile flags
    enum class ShaderCompileFlags : uint32_t
    {
        None             = 0,
        Debug            = 1u << 0,
        SkipOptimization = 1u << 1,
        WarningsAsErrors = 1u << 2,
    };

    inline ShaderCompileFlags operator|(ShaderCompileFlags a, ShaderCompileFlags b)
    {
        return static_cast<ShaderCompileFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline ShaderCompileFlags operator&(ShaderCompileFlags a, ShaderCompileFlags b)
    {
        return static_cast<ShaderCompileFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    inline ShaderCompileFlags& operator|=(ShaderCompileFlags& a, ShaderCompileFlags b)
    {
        a = a | b;
        return a;
    }
    inline ShaderCompileFlags& operator&=(ShaderCompileFlags& a, ShaderCompileFlags b)
    {
        a = a & b;
        return a;
    }

    inline bool HasFlag(ShaderCompileFlags v, ShaderCompileFlags f) { return static_cast<uint32_t>(v & f) != 0; }

    struct ShaderMacroCpu
    {
        std::string name;
        std::string value;
    };

    struct ShaderAsset
    {
        ShaderId         sourceId{};
        ShaderSourceKind sourceKind = ShaderSourceKind::HLSL;

        ShaderStage shaderStage = ShaderStage::Vertex;
        RenderStage renderStage = RenderStage::MAIN;

        std::string entryPoint = "main";
        std::string target; // "vs_5_0", "ps_5_0", etc.

        // list of macros to compile with. Only needed for hlsl types, cso should already be compiled with the correct macros
        std::vector<ShaderMacroCpu> macros;

        ShaderCompileFlags compileFlags = ShaderCompileFlags::None;
        uint32_t           effectFlags  = 0;

        std::vector<std::byte> byteCode; // optional cached shader bytes (typically cso)
    };
} // namespace Murder
