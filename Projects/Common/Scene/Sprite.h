#pragma once

#include <cstdint>
#include <string>

#include <Core/AssetTags.h>
#include <Core/MurderCore.hpp>

namespace Murder
{
    struct RectI
    {
        int left   = 0;
        int top    = 0;
        int right  = 0;
        int bottom = 0;
    };

    struct Sprite
    {
        enum class Anchor : uint8_t
        {
            Center,
            TopLeft,
            TopRight,
            BottomLeft,
            BottomRight
        };

        Transform2D transform{};
        Vector2     origin{};
        std::string name{};

        RectI srcRect{};
        RectI dstRect{};

        Anchor anchor  = Anchor::Center;
        Color  tint    = { 1.0f, 1.0f, 1.0f, 1.0f };
        float  layer   = 0.0f;
        bool   visible = false;

        TextureId textureId{};
    };
} // namespace Murder
