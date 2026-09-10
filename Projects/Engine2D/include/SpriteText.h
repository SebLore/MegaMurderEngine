#pragma once

#include <string>

#include <Core\Base\Common.h>

struct SpriteText
{
    std::wstring    Text{};
    Murder::Vector2 Position = { 0.0f, 0.0f };
    float           Size     = 1.0f;
    float           Rotation = 0.0f;
    float           Layer    = 0.0f;
    bool            Visible  = true;
};
