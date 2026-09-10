#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include "Common/Scene/Sprite.h"

struct RenderSprite
{
    Murder::Transform2D transform{};
    Murder::Vector2     origin{};
    std::string         name{};

    Murder::RectI srcRect{};
    Murder::RectI dstRect{};

    Murder::Sprite::Anchor anchor  = Murder::Sprite::Anchor::Center;
    Murder::Color          tint    = { 1.0f, 1.0f, 1.0f, 1.0f };
    float                  layer   = 0.0f;
    bool                   visible = false;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture;
};
