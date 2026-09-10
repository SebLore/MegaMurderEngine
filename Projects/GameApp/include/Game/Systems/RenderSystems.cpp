#include "Systems.h"

#include "SystemCommon.h"

#include "Common/Scene/RenderWorld.h"
#include "Engine/Hud.h"

namespace Game
{

    bool BuildRenderWorldSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        auto& rw    = ecs.ContextRef<RenderWorld>();
        auto& state = ecs.ContextRef<GameState>();

        rw.Clear();

        // Active camera
        Camera camera{};
        {
            const auto& ui = ecs.ContextRef<UiViewport>();

            const auto player = ecs.FindFirst<PlayerTag>();
            if (player != entt::null)
            {
                const auto& transform = ecs.Get<TransformC>(player);
                const auto& [offset]  = ecs.Get<CameraAttachment>(player);

                camera.position = transform.position + offset;
                camera.rotation = transform.rotation;
            }

            const float aspect = (ui.height > FLT_EPSILON) ? (ui.width / ui.height) : (4.0f / 3.0f);

            camera.SetPerspective(ToRadians(60.0f), aspect, 0.1f, 1000.0f);
        }
        rw.camera = camera;

        // World models
        {
            auto view = ecs.View<ModelComponent>(entt::exclude<ViewModelTag>);

            for (auto entity : view)
            {
                const auto& model = view.get<ModelComponent>(entity);

                Matrix world = TransformMath::World(model.transform);

                if (const auto* transform = ecs.TryGet<TransformC>(entity))
                    world *= TransformMath::World(*transform);

                rw.models.emplace_back(
                    ModelInstance{ .id                = model.id,
                                   .world             = world,
                                   .kind              = ModelKind::World,
                                   .materialOverrides = &model.materialOverrides,
                                   .textureOverrides  = &model.textureOverrides });
            }
        }

        // View model
        if (state.IsRunning() || state.IsPaused() || state.IsMainMenu())
        {
            const auto vm = ecs.View<ViewModelTag>().front();
            if (const auto* model = ecs.TryGet<ModelComponent>(vm))
            {
                Matrix world = TransformMath::World(model->transform);

                for (auto [player, weapon] : ecs.View<PlayerTag, Weapon>().each())
                {
                    Vector3 localOffset(0.3f, -0.3f, 0.75f);
                    Vector3 worldOffset = Vector3::Transform(localOffset, camera.rotation);

                    float recoilAmount = -PI_DIV4 * (weapon.weaponCooldown / weapon.timeBetweenShots);

                    Quaternion localRotation = Quaternion::CreateFromYawPitchRoll(0.0f, recoilAmount, 0.0f);

                    Transform vmTransform{};
                    vmTransform.position = camera.position + worldOffset;
                    vmTransform.rotation = localRotation * camera.rotation;
                    vmTransform.scale    = Vector3{ 0.2f, 0.2f, 0.2f };

                    world = TransformMath::World(vmTransform);
                    break;
                }

                rw.models.emplace_back(
                    ModelInstance{ .id                = model->id,
                                   .world             = world,
                                   .kind              = ModelKind::ViewModel,
                                   .materialOverrides = &model->materialOverrides,
                                   .textureOverrides  = &model->textureOverrides });
            }
        }

        // Lights
        {
            auto lightView = ecs.View<DirectionalLight>();
            for (auto e : lightView)
                rw.directionalLights.push_back(lightView.get<DirectionalLight>(e));
        }

        {
            auto lightView = ecs.View<PointLight>();
            for (auto e : lightView)
                rw.pointLights.push_back(lightView.get<PointLight>(e));
        }

        {
            auto lightView = ecs.View<SpotLight>();
            for (auto e : lightView)
                rw.spotLights.push_back(lightView.get<SpotLight>(e));
        }

        rw.ambientLight = Vector3{ 0.1f, 0.1f, 0.1f };

        // HUD sprites
        {
            auto& hud = ecs.Context<Hud>();

            hud.GetSpritePointers(rw.sprites);

            rw.spriteTexts.push_back(&hud.timer);
            rw.spriteTexts.push_back(&hud.speed);

            for (auto& text : hud.staticText)
                rw.spriteTexts.push_back(&text);

            rw.spriteTexts.push_back(&hud.comboBar.display);
            rw.spriteTexts.push_back(&hud.healthBar.display);
        }

        {
            auto menuView = ecs.View<MenuComponent>();
            for (auto [e, menu] : menuView.each())
            {

                rw.sprites.push_back(&menu.background);

                for (auto& button : menu.buttons)
                    rw.sprites.push_back(&button.sprite);
            }
        }

        {
            auto menuView = ecs.View<TextComponent>();
            for (auto [e, menu] : menuView.each())
            {
                rw.spriteTexts.push_back(&menu.points);
                rw.spriteTexts.push_back(&menu.buttons);

                //for (auto& button : menu.textLines)
                //{
                //    rw.spriteTexts.push_back(&button);
                //}
            }
        }

        // collect any other sprites or sprite text for rendering
        {
            for (auto [entity, sprite] : ecs.View<Sprite>().each())
                rw.sprites.push_back(&sprite);

            for (auto [entity, text] : ecs.View<SpriteText>().each())
                rw.spriteTexts.push_back(&text);
        }

        return false;
    }
} // namespace Game
