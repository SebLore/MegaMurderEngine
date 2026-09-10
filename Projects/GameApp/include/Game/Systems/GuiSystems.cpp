
#include "Systems.h"

#include "SystemCommon.h"

#include <Input.h>
#include <imgui.h>

#include <ECS.hpp>

#include <AssetManager.h>

#include "Engine/Hud.h"

#include "Game/Components.h"
#include "Game/DebugSettings.h"

#include <Utility/ErrorHandling.h>
#include <Utility/Logging.h>

#include <Common/Scene/Sprite.h>

#include "DebugDrawQueue.h"
#include "Common/Scene/RenderWorld.h"

#define LOG_TAG "GUI Sys"

using namespace ECS;
using namespace Murder;

namespace
{
    struct HudSpriteCfg
    {
        Transform2D    Transform{};
        Vector2        SrcRectDim = { 1.0f, 1.0f };
        Sprite::Anchor Anchor     = Sprite::Anchor::Center;
    };

    SpriteText MakeOverlayText(const std::wstring& text, const Vector2& pos, float size = 1.25f, float layer = 1.0f)
    {
        return SpriteText{
            .Text     = text,
            .Position = pos,
            .Size     = size,
            .Layer    = layer,
            .Visible  = true,
        };
    }

    void ConstructDestRect(Sprite& s)
    {
        Vector2 topLeft = s.transform.position;
        Vector2 size    = s.transform.scale;

        switch (s.anchor)
        {
        case Sprite::Anchor::Center:
            topLeft.x -= size.x * 0.5f;
            topLeft.y -= size.y * 0.5f;
            s.origin   = Vector2{ size.x * 0.5f, size.y * 0.5f };
            break;

        case Sprite::Anchor::TopRight:
            topLeft.x -= size.x;
            s.origin   = Vector2{ size.x, 0.0f };
            break;

        case Sprite::Anchor::BottomLeft:
            topLeft.y -= size.y;
            s.origin   = Vector2{ 0.0f, size.y };
            break;

        case Sprite::Anchor::BottomRight:
            topLeft.x -= size.x;
            topLeft.y -= size.y;
            s.origin   = Vector2{ size.x, size.y };
            break;

        case Sprite::Anchor::TopLeft:
        default:
            s.origin = Vector2{ 0.0f, 0.0f };
            break;
        }

        s.dstRect = {
            .left   = static_cast<int>(topLeft.x),
            .top    = static_cast<int>(topLeft.y),
            .right  = static_cast<int>(topLeft.x + size.x),
            .bottom = static_cast<int>(topLeft.y + size.y),
        };
    }

    Sprite MakeOverlaySprite(
        ECSManager&      ecs,
        std::string_view texturePath,
        const Vector2&   pos,
        const Vector2&   scale,
        const Vector2&   srcScale,
        const Vector2&   displace = { 0.0f, 0.0f },
        float            layer    = 0.9f)
    {
        auto& am = ecs.ContextRef<AssetManager>();

        TextureId id = am.RegisterTexture(texturePath);
        if (!am.LoadTexture(id))
        {
            LOG_WARN("Texture not found: " << texturePath << ". Using fallback.");
            return {};
        }

        Sprite s{
            .transform = { .position = pos, .scale = scale },
            .anchor    = Sprite::Anchor::Center,
            .tint      = { 1.0f, 1.0f, 1.0f, 0.0f }, // fade in
            .layer     = layer,
            .visible   = true,
            .textureId = am.RegisterTexture(texturePath),
        };

        s.srcRect = {
            .left   = 0,
            .top    = 0,
            .right  = static_cast<int>(srcScale.x),
            .bottom = static_cast<int>(srcScale.y),
        };

        ConstructDestRect(s);
        return s;
    }

    Sprite MakeSprite(AssetManager& am, std::string_view texturePath, const HudSpriteCfg& cfg)
    {
        Sprite s{};
        s.textureId = am.RegisterTexture(texturePath);
        s.transform = cfg.Transform;
        s.anchor    = cfg.Anchor;
        s.srcRect   = {
              .left   = 0,
              .top    = 0,
              .right  = static_cast<int>(cfg.SrcRectDim.x),
              .bottom = static_cast<int>(cfg.SrcRectDim.y),
        };
        s.visible = true;
        ConstructDestRect(s);
        return s;
    }

    LayeredSpriteElement CreateLayerSprite(
        AssetManager&       am,
        std::string_view    activeTex,
        std::string_view    backgroundTex,
        std::string_view    foregroundTex,
        const HudSpriteCfg& cfg)
    {
        LayeredSpriteElement out{};

        out.active       = MakeSprite(am, activeTex, cfg);
        out.active.layer = 0.1f;

        out.background = MakeSprite(am, backgroundTex, cfg);

        out.foreground       = MakeSprite(am, foregroundTex, cfg);
        out.foreground.layer = 0.2f;

        return out;
    }

    HealthBarHud& AddHealthToHud(
        AssetManager&       am,
        Hud&                h,
        std::string_view    activeTex,
        std::string_view    backgroundTex,
        std::string_view    foregroundTex,
        const HudSpriteCfg& cfg)
    {
        constexpr int offset = 0;

        auto& hp       = h.healthBar;
        auto& activeHp = hp.spriteElement.active;
        auto& hpText   = hp.display;

        Transform2D textTransform = {
            .position = { cfg.Transform.position.x + 100, cfg.Transform.position.y - 65 },
            .scale    = { 1.0f, 1.0f },
            .rotation = cfg.Transform.rotation,
        };

        hp.spriteElement = CreateLayerSprite(am, activeTex, backgroundTex, foregroundTex, cfg);

        activeHp.transform = {
            .position = { cfg.Transform.position.x + offset, cfg.Transform.position.y - offset },
            .scale    = { cfg.Transform.scale.x - offset * 2, cfg.Transform.scale.y - offset * 2 },
            .rotation = cfg.Transform.rotation,
        };

        ConstructDestRect(activeHp);

        hpText.Text     = L"100/100";
        hpText.Position = textTransform.position;
        hpText.Size     = textTransform.scale.x;
        hpText.Rotation = textTransform.rotation;
        hpText.Layer    = 0.2f;

        hp.maxSize = static_cast<size_t>(activeHp.transform.scale.x);

        return hp;
    }

    ComboBarHud& AddComboToHud(
        AssetManager&       am,
        Hud&                h,
        std::string_view    activeTex,
        std::string_view    backgroundTex,
        std::string_view    foregroundTex,
        const HudSpriteCfg& cfg)
    {
        HudSpriteCfg activeCfg = cfg;

        auto& combo              = h.comboBar;
        //auto& comboBackground = combo.spriteElement.background;
        auto& comboActive        = combo.spriteElement.active;
        auto& activeTransformCfg = activeCfg.Transform;

        constexpr float margin = 2.0f;

        activeTransformCfg.position.x -= margin; // Offset right edge inward
        activeTransformCfg.position.y += margin; // Offset down

        activeTransformCfg.scale.x -= margin * 2; // Make narrower
        activeTransformCfg.scale.y -= margin * 2; // Make shorter

        comboActive       = MakeSprite(am, activeTex, activeCfg);
        comboActive.layer = 0.1f;

        combo.anchorPos = comboActive.transform.position;
        combo.maxSize   = static_cast<size_t>(activeTransformCfg.scale.x);
        combo.spriteElement.SetVisibility(false);

        combo.spriteElement = CreateLayerSprite(am, activeTex, backgroundTex, foregroundTex, cfg);

        auto& comboText           = combo.display;
        comboText.Text            = L"No combo";
        Transform2D textTransform = {
            .position = { cfg.Transform.position.x - 140, cfg.Transform.position.y + 60 },
            .scale    = { 1.0f, 1.0f },
            .rotation = cfg.Transform.rotation,
        };
        comboText.Position = textTransform.position;
        comboText.Size     = textTransform.scale.x * 0.75f;
        comboText.Rotation = textTransform.rotation;
        comboText.Layer    = 0.2f;

        return combo;
    }
} // namespace

bool Game::InputFocusSystem::OnUpdate(ECSManager& ecs, float dt)
{
    auto& input = ecs.ContextRef<Input>();
    auto& focus = ecs.ContextRef<InputMode>();
    auto& state = ecs.ContextRef<GameState>();

    if (!focus.appFocused)
        return false;

    bool blockMouse = false;
#ifdef USING_IMGUI
    blockMouse =
        ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || ImGui::IsAnyItemHovered() || ImGui::IsAnyItemActive();
#endif

    if (state.IsEditing())
    {
        // Click RMB to enter UI mode
        if (input.IsPressed(Input::Control::MouseButton::RIGHT) && !focus.uiMode)
        {
            focus.uiMode           = true;
            focus.suppressLookOnce = true;
        }

        // Click LMB to go back to look/move mode
        if (input.IsPressed(Input::Control::MouseButton::LEFT) && focus.uiMode && !blockMouse)
        {
            focus.uiMode           = false;
            focus.suppressLookOnce = true;
        }
    }

    return false;
}

bool Game::GameOverlaySystem::OnUpdate(ECSManager& ecs, float dt)
{
    auto&       state       = ecs.ContextRef<GameState>();
    const auto& ui          = ecs.ContextRef<UiViewport>();
    auto        entityCombo = ecs.FindFirst<Combo>();
    auto        points      = ecs.TryGet<Combo>(entityCombo);
    auto&       input       = ecs.ContextRef<Input>();

    static constexpr float pausedSize  = 1.5f;
    static constexpr float editingSize = 1.0f;
    static constexpr float layer       = 1.0f;

    const float width = ui.width;

    const Vector2 gameOverSize = { ui.width, ui.height };
    const Vector2 pausedPos    = { width * 0.5f, pausedSize * 100.0f };
    const Vector2 editingPos   = { width - 160.0f, editingSize * 40.0f };
    const Vector2 menuPos      = { width * 0.5f, ui.height * 0.5f };

    enum class ButtonAction : uint8_t
    {
        Start,
        Options,
        Exit,
    };

    if (state.IsMainMenu())
    {
        Entity e = ecs.FindFirst<MainMenuTag>();

        if (e == entt::null)
        {
            e = ecs.Create();
            ecs.Emplace<MainMenuTag>(e);

            auto& menu = ecs.Emplace<MenuComponent>(e);

            Vector2 scaleSkullSrc = { 1920, 1080 };
            Vector2 scaleSkull    = { ui.width, ui.height };
            Vector2 displace      = Vector2{ 0, 0 }; // displace

            menu.background =
                MakeOverlaySprite(ecs, "skullFMOD.png", menuPos, scaleSkull, scaleSkullSrc, displace, 0.95f);
            //menu.background = MakeOverlaySprite(ecs, "skull.png", menuPos, scaleSkull, displace, 0.95f);

            const Vector2 buttonScale = { 355, 124 };

            const size_t buttonCount = 2;
            Sprite sprite = MakeOverlaySprite(ecs, "Button.png", menuPos, buttonScale, buttonScale, { 0, 0 }, layer);
            //Sprite sprite = MakeOverlaySprite(ecs, "skullFMOD.png", menuPos, scaleSkull, displace, 0.99f);

            for (size_t i = 0; i < buttonCount; i++)
            {
                auto& btn =
                    menu.buttons.emplace_back(MenuButton{ .scale = buttonScale, .position = {}, .sprite = sprite });
                btn.sprite.transform.position = Vector2{ menuPos.x, menuPos.y + static_cast<float>(i) * -200.0f };
            }
        }
        else
        {
            auto view = ecs.View<MainMenuTag, MenuComponent>();

            for (auto ent : view)
            {
                auto& menu = view.get<MenuComponent>(ent);

                menu.background.visible = true;
                menu.background.tint.w  = 1.0f;
                ConstructDestRect(menu.background);

                for (auto& button : menu.buttons)
                {
                    button.sprite.visible = true;
                    button.sprite.tint.w  = 1.0f;
                    ConstructDestRect(button.sprite);
                }
            }
        }

        if (e != entt::null)
        {
            if (auto* text = ecs.TryGet<SpriteText>(e))
            {
                text->Position = Vector2{ menuPos.x - 110, menuPos.y - 120.0f };
                text->Size     = 1.5f;
                text->Layer    = layer;
                text->Visible  = true;
            }
            else
            {

                wchar_t        buff[256];
                const wchar_t* endMenuTemplate = LR"(
                    START 


                     EXIT
                )";

                int score = static_cast<int>(points->score);
                //float timeSpent = hud.timerTime;

                //int totalAmount = score + static_cast<int>(timeSpent);

                swprintf_s(buff, _countof(buff), endMenuTemplate, score /*, timeSpent, totalAmount*/);

                SpriteText name = MakeOverlayText(buff, { menuPos.x, menuPos.y }, 1.5f, layer);
                ecs.Emplace<SpriteText>(e, std::move(name));
            }
        }
    }
    else
    {
        ecs.DestroyFirst<MainMenuTag>();
    }

    // PAUSED overlay
    if (state.IsPaused())
    {
        Entity e = ecs.FindFirst<PauseOverlayTag>();

        if (e == entt::null)
        {
            e = ecs.Create();
            ecs.Emplace<PauseOverlayTag>(e);

            SpriteText text = MakeOverlayText(L"PAUSED", pausedPos, pausedSize, layer);

            ecs.Emplace<SpriteText>(e, std::move(text));
        }
        else if (auto* text = ecs.TryGet<SpriteText>(e))
        {
            text->Text     = L"PAUSED";
            text->Position = pausedPos;
            text->Visible  = true;
        }
    }
    else
    {
        ecs.DestroyFirst<PauseOverlayTag>();
    }

    // EDITING overlay
    if (state.IsEditing())
    {
        Entity e = ecs.FindFirst<EditOverlayTag>();
        if (e == entt::null)
        {
            e = ecs.Create();
            ecs.Emplace<EditOverlayTag>(e);

            auto text = MakeOverlayText(L"EDITING", editingPos, editingSize, layer);

            ecs.Emplace<SpriteText>(e, std::move(text));
        }
        else if (auto* text = ecs.TryGet<SpriteText>(e))
        {
            text->Text     = L"EDITING";
            text->Position = editingPos;
            text->Visible  = true;
        }
    }
    else
    {
        ecs.DestroyFirst<EditOverlayTag>();
    }

    // GAME OVER sprite overlay
    if (state.IsEnded())
    {
        Entity e   = ecs.FindFirst<GameOverOverlayTag>();
        auto&  hud = ecs.Context<Hud>();
        auto&  am  = ecs.ContextRef<AssetManager>();

        float t               = 0.4f;
        bool  showRestartText = false;

        if (auto* timer = ecs.TryContext<RestartDelay>())
        {
            constexpr float fadeDuration = 1.5f;
            t                            = std::clamp(timer->elapsed / fadeDuration, 0.0f, 1.0f);
            showRestartText              = timer->elapsed >= fadeDuration;
        }
        float alpha = t * (2.0f - t);

        if (e == entt::null)
        {
            e = ecs.Create();
            ecs.Emplace<GameOverOverlayTag>(e);

            auto& menu = ecs.Emplace<MenuComponent>(e);

            Vector2 endScale    = { ui.width, ui.height };
            Vector2 endScaleSrc = { 581, 632 };
            Vector2 displace    = { -1400, -900 };

            menu.background = MakeOverlaySprite(ecs, "endMenu.png", menuPos, endScale, endScaleSrc, displace, 0.99f);

            const Vector2 buttonScale = { 355, 124 };

            const size_t buttonCount = 2;
            Sprite sprite = MakeOverlaySprite(ecs, "Button.png", menuPos, buttonScale, buttonScale, { 0, 0 }, layer);

            for (size_t i = 0; i < buttonCount; i++)
            {
                auto& btn =
                    menu.buttons.emplace_back(MenuButton{ .scale = buttonScale, .position = {}, .sprite = sprite });
                btn.sprite.transform.position =
                    Vector2{ menuPos.x + 200 + static_cast<float>(i) * -500.0f, menuPos.y - 200 };
            }
        }
        else
        {
            auto view = ecs.View<GameOverOverlayTag, MenuComponent>();

            for (auto ent : view)
            {
                auto& menu = view.get<MenuComponent>(ent);

                menu.background.visible = true;
                menu.background.tint.w  = alpha;

                ConstructDestRect(menu.background);

                for (auto& button : menu.buttons)
                {
                    button.sprite.visible = true;
                    button.sprite.tint.w  = alpha;
                    ConstructDestRect(button.sprite);
                }
            }
        }

        if (showRestartText && e != entt::null)
        {
            auto* text = ecs.TryGet<TextComponent>(e);
            if (text == nullptr)
            {
                auto& textMenu = ecs.Emplace<TextComponent>(e);

                wchar_t        buffPoints[512];
                const wchar_t* endMenuTemplatePoints = LR"(
                    RESTART                             END
                )";

                swprintf_s(buffPoints, _countof(buffPoints), endMenuTemplatePoints);

                textMenu.buttons = MakeOverlayText(buffPoints, { menuPos.x - 200, menuPos.y - 245 }, 1.5f, layer);

                wchar_t        buff[512];
                const wchar_t* endMenuTemplate = LR"(
                Base Score      %d
                Time                  %.2fs
                Kills                    %d
                No damage      %d

                Total score      %d
                )";

                int   scoreTotal     = static_cast<int>(points->totalScore);
                int   killCount      = points->killCount;
                int   unbrokenPoints = points->broken ? 500 : 0;
                float timeSpent      = hud.timerTime;
                int   totalScore     = scoreTotal + killCount + unbrokenPoints;

                swprintf_s(
                    buff,
                    _countof(buff),
                    endMenuTemplate,
                    scoreTotal,
                    timeSpent,
                    killCount,
                    unbrokenPoints,
                    totalScore);

                textMenu.points = MakeOverlayText(buff, { menuPos.x, menuPos.y + 200.0f }, 1.5f, layer);
            }
            else
            {
                text->points.Visible = true;
            }
        }
    }
    else
    {
        ecs.DestroyFirst<GameOverOverlayTag>();
    }

    return false;
}

bool Game::HUDInitializeSystem::OnUpdate(ECSManager& ecs, float dt)
{
    auto& hud = ecs.Context<Hud>();
    auto& am  = ecs.ContextRef<AssetManager>();

    const auto& ui     = ecs.ContextRef<UiViewport>();
    const float width  = ui.width;
    const float height = ui.height;

    const float widthHalf  = width * 0.5f;
    const float heightHalf = height * 0.5f;

    // Crosshair
    {
        Sprite crosshair{};
        crosshair.name      = "crosshair_ui";
        crosshair.textureId = am.RegisterTexture("crosshair.png");
        crosshair.srcRect   = { .left = 0, .top = 0, .right = 32, .bottom = 32 };

        const int half = static_cast<int>(std::lround(16.0f * ui.scale));
        const int cx   = static_cast<int>(std::lround(widthHalf));
        const int cy   = static_cast<int>(std::lround(heightHalf));

        crosshair.dstRect = { .left = cx - half, .top = cy - half, .right = cx + half, .bottom = cy + half };
        crosshair.layer   = 0.0f;
        crosshair.visible = true;

        if (crosshair.textureId.value == 0)
            LOG_ERROR("Invalid texture id for texture " << "crosshair.png");
        else
            hud.crosshair = std::move(crosshair);
    }

    // Health bar
    {

        Transform2D initHealth = { .position = { 50.0f, height - 50 },
                                   .scale    = { 256.0f, 32.0f },
                                   .rotation = (-5.0f / 360.0f) * DirectX::XM_2PI };

        HudSpriteCfg healthCfg = { .Transform  = initHealth,
                                   .SrcRectDim = { 128.0f, 32.0f },
                                   .Anchor     = Sprite::Anchor::BottomLeft };

        AddHealthToHud(am, hud, "healthBarResize.png", "healthBackground.png", "healthForeground.png", healthCfg);
    }

    // Combo bar
    Transform2D initCombo = { .position = { width - 50.0f, 70.0f }, .scale = { 256.0f, 32.0f } };

    HudSpriteCfg comboCfg = { .Transform  = initCombo,
                              .SrcRectDim = { 128.0f, 32.0f },
                              .Anchor     = Sprite::Anchor::TopRight };

    AddComboToHud(am, hud, "comboActive.png", "healthBackground.png", "comboForeground.png", comboCfg);

    // Timer
    Transform2D timerTransform = { { widthHalf, 50.0f } };
    hud.AddTimerToHud(L"00:00:00", timerTransform);

    // Speed
    Transform2D speedTransform = { { widthHalf - 0.70f * widthHalf, 50.0f } };
    hud.AddSpeedToHud(L"00 m/s", speedTransform);

    return true;
}

bool Game::UpdateHudSystem::OnUpdate(ECSManager& ecs, float dt)
{
    auto& h     = ecs.Context<Hud>();
    auto& state = ecs.ContextRef<GameState>();

    const bool gameRunning = GameRunning(ecs);

    auto view = ecs.View<PlayerTag, Health, PlayerState>();

    // Temp resize fix
    {
        const auto& ui     = ecs.ContextRef<UiViewport>();
        const float width  = ui.width;
        const float height = ui.height;

        const float widthHalf  = width * 0.5f;
        const float heightHalf = height * 0.5f;

        // if game isn't running we want to hide crosshair

        // crosshair
        {
            const int half = static_cast<int>(std::lround(16.0f * ui.scale));
            const int cx   = static_cast<int>(std::lround(widthHalf));
            const int cy   = static_cast<int>(std::lround(heightHalf));

            h.crosshair.dstRect = { .left = cx - half, .top = cy - half, .right = cx + half, .bottom = cy + half };
            h.crosshair.visible = gameRunning;
        }

        // health
        auto& healthBar                         = h.healthBar.spriteElement;
        healthBar.active.transform.position     = Vector2{ 50.0f, height - 50.0f };
        healthBar.background.transform.position = Vector2{ 50.0f, height - 50.0f };
        healthBar.foreground.transform.position = Vector2{ 50.0f, height - 50.0f };
        ConstructDestRect(healthBar.background);
        ConstructDestRect(healthBar.foreground);
        h.healthBar.display.Position = Vector2{ 140.0f, height - 120.0f };

        // combo
        auto& comboBar                         = h.comboBar.spriteElement;
        comboBar.active.transform.position     = Vector2{ width - 50.0f, 70.0f };
        comboBar.background.transform.position = Vector2{ width - 50.0f, 70.0f };
        comboBar.foreground.transform.position = Vector2{ width - 50.0f, 70.0f };
        ConstructDestRect(comboBar.background);
        ConstructDestRect(comboBar.foreground);
        h.comboBar.display.Position = Vector2{ width - 190.0f, 130 };

        // timer
        h.timer.Position = Vector2{ widthHalf, 50.0f };

        // speed
        h.speed.Position = Vector2{ 150.0f, 50.0f };

        // menu
        auto menuView = ecs.View<MenuComponent>(entt::exclude<GameOverOverlayTag>);
        for (auto [e, menu] : menuView.each())
        {
            menu.background.transform.position = Vector2{ widthHalf, heightHalf };
            menu.background.transform.scale    = Vector2{ width, height };

            int tempButtonDisplace =
                0; // only here to temp fix the resize of the screen and where the buttons need to be placed
            for (auto& button : menu.buttons)
            {
                button.sprite.transform.position =
                    Vector2{ widthHalf, heightHalf + static_cast<float>(tempButtonDisplace) * -200.0f };
                tempButtonDisplace++;
            }
        }
        auto endView = ecs.View<MenuComponent, TextComponent>(entt::exclude<MainMenuTag>);
        for (auto [e, menu, textMenu] : endView.each())
        {
            menu.background.transform.position = Vector2{ widthHalf, heightHalf };
            menu.background.transform.scale    = Vector2{ width, height };

            int tempButtonDisplace =
                0; // only here to temp fix the resize of the screen and where the buttons need to be placed
            for (auto& button : menu.buttons)
            {
                button.sprite.transform.position =
                    Vector2{ widthHalf + 200 + static_cast<float>(tempButtonDisplace) * -500.0f, heightHalf - 200 };
                tempButtonDisplace++;
            }

            textMenu.buttons.Position = Vector2{ widthHalf - 200, heightHalf - 245 };
            textMenu.points.Position  = Vector2{ widthHalf, heightHalf + 200.0f };
        }
    }

    auto& healthBar       = h.healthBar;
    auto& activeHealth    = healthBar.spriteElement.active;
    auto& activeTransform = activeHealth.transform;

    // Should only be one
    for (auto e : view)
    {
        if (auto state = ecs.TryGet<PlayerState>(e))
        {
            if (state->IsDying() || state->IsDead())
                continue;
        }
        if (auto health = ecs.TryGet<Health>(e))
        {
            h.healthBar.health      = health->hp;
            activeTransform.scale.x = h.healthBar.maxSize * (health->hp / health->maxHp);

            ConstructDestRect(activeHealth);

            int curHP = static_cast<int>(h.healthBar.health);
            int maxHP = static_cast<int>(h.healthBar.maxHealth);

            healthBar.display.Text = std::to_wstring(curHP) + L"/" + std::to_wstring(maxHP);
        }
    }
    // combo
    {
        auto& bar     = h.comboBar;
        auto& sprites = bar.spriteElement;

        auto comboEntity = ecs.FindFirst<Combo>();
        auto combo       = ecs.TryGet<Combo>(comboEntity);
        if (combo->rank > ComboRank::None)
        {
            if (!bar.display.Visible)
            {
                sprites.SetVisibility(true);
                bar.display.Visible = true;
            }

            const auto& data     = ComboTable[static_cast<int>(combo->rank)];
            const float progress = (combo->score < data.threshold) ? combo->score / data.threshold : 1.0f;
            const float width    = bar.maxSize * progress;

            sprites.active.transform.scale.x = width;
            sprites.active.tint =
                Color{ std::clamp(2.0f - progress * 2, 0.0f, 1.0f), std::clamp(progress * 2.0f, 0.0f, 1.0f), 0.0f };
            ConstructDestRect(sprites.active);
        }
        else
        {
            sprites.SetVisibility(false);
            bar.display.Visible = false;
        }

        std::wstring comboRank = L"";
        switch (combo->rank)
        {
        case ComboRank::SPlus:
            comboRank = L"S+";
            break;
        case ComboRank::S:
            comboRank = L"S";
            break;
        case ComboRank::A:
            comboRank = L"A";
            break;
        case ComboRank::B:
            comboRank = L"B";
            break;
        case ComboRank::C:
            comboRank = L"C";
            break;
        case ComboRank::D:
            comboRank = L"D";
            break;
        case ComboRank::None:
            comboRank = L"";
            break;
        }

        bar.display.Text = comboRank + L" RANK";
    }

    // timer
    if (gameRunning)
    {
        h.timerTime += dt;

        uint32_t totalMilliseconds = static_cast<uint32_t>(h.timerTime * 1000.0f);

        uint32_t min          = totalMilliseconds / 60000;
        uint32_t sec          = totalMilliseconds / 1000 % 60;
        uint32_t milliseconds = totalMilliseconds % 1000;

        wchar_t buffer[16];
        swprintf_s(buffer, L"%02u:%02u:%03u", min, sec, milliseconds);
        h.timer.Text = std::wstring(buffer);

        // Speed
        {
            h.speedCooldown += dt;
            if (h.speedCooldown >= 0.05f) // 20 times per second
            {
                auto speedView = ecs.View<PlayerTag, Transform>();
                for (auto e : speedView)
                {
                    if (auto pos = ecs.TryGet<Transform>(e))
                    {
                        Vector3 deltaPos = pos->position - h.oldPlayerPos;

                        wchar_t speedBuffer[16];
                        swprintf_s(speedBuffer, L"%05.2f m/s", deltaPos.Length() / h.speedCooldown);
                        h.speed.Text = std::wstring(speedBuffer);

                        h.oldPlayerPos = pos->position;
                    }
                }
                h.speedCooldown = 0.0f;
            }
        }
    }

    return false;
}
