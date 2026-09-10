#pragma once

#include <Common/Scene/Sprite.h>
#include <SpriteText.h>

struct LayeredSpriteElement
{
    Murder::Sprite background{};
    Murder::Sprite active{};
    Murder::Sprite foreground{};

    void SetVisibility(bool vis)
    {
        background.visible = vis;
        active.visible     = vis;
        foreground.visible = vis;
    }
};

struct HealthBarHud
{
    LayeredSpriteElement spriteElement;

    float barJump    = 1.0f;
    float lastHealth = 100.0f;
    float health     = 100.0f;
    float maxHealth  = 100.0f;

    size_t     maxSize = 0;
    SpriteText display{};
};

//struct EndMenuStruct
//{
//    
//};

struct ComboBarHud
{
    LayeredSpriteElement spriteElement;

    Murder::Vector2 anchorPos{}; // store original x and y for when scaling
    size_t          maxSize = 0;
    SpriteText      display{};
};

struct Hud
{
    ModelId viewModel{};

    Murder::Sprite crosshair{};

    HealthBarHud  healthBar{};
    ComboBarHud   comboBar{};

    float      timerTime = 0.0f;
    SpriteText timer{};
    SpriteText speed{};

    Murder::Vector3 oldPlayerPos = {0.0f, 0.0f, 0.0f}; // used to calculate delta pos for speed
    float speedCooldown = 0.0f;
    std::vector<SpriteText> staticText;

  public:
    SpriteText& AddText(const std::wstring& text, const Murder::Transform2D& initialTransform)
    {
        staticText.emplace_back(
            SpriteText{
                .Text     = text,
                .Position = initialTransform.position,
                .Size     = 1.0f,
                .Rotation = 0.0f,
                .Layer    = 0.0f,
                .Visible  = true,
            });

        return staticText.back();
    }
    size_t TextCount() const { return staticText.size() + 2; }

    SpriteText& AddTimerToHud(std::wstring text, const Murder::Transform2D& initialTransform)
    {
        timer = { std::move(text), initialTransform.position, 1.0f, 0.0f, 0.0f, true };
        return timer;
    }

    SpriteText& AddSpeedToHud(std::wstring text, const Murder::Transform2D& initialTransform)
    {
        speed = { std::move(text), initialTransform.position, 1.0f, 0.0f, 0.0f, true };
        return speed;
    }

    // helper function to extract all sprites to a vector
    void GetSpritePointers(std::vector<Murder::Sprite*>& out)
    {
        constexpr size_t count = 7; // increase as we add sprites

        out.reserve(out.size() + count);

        out.push_back(&crosshair);
        out.push_back(&healthBar.spriteElement.background);
        out.push_back(&healthBar.spriteElement.active);
        out.push_back(&healthBar.spriteElement.foreground);
        out.push_back(&comboBar.spriteElement.background);
        out.push_back(&comboBar.spriteElement.active);
        out.push_back(&comboBar.spriteElement.foreground);
        //out.push_back(&endMenu);
    }
};
