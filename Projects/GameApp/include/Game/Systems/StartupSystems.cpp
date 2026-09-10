#include "Systems.h"

#include "SystemCommon.h"

#include <Utility/Logging.h>
#include <Utility/ErrorHandling.h>
#define LOG_TAG "StartupSystems"

// === Include for LoadSystem ==================
#include <Engine/Hud.h>

#include "Audio/AudioSystem.h"
// =============================================

namespace Game
{
    using DirectX::Keyboard;

    /*****************************************************************************
* Startup Systems
*****************************************************************************/

    bool PlayerInitializeSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        auto& cfg  = ecs.Context<Config::GameConfig>();
        using Keys = Keyboard::Keys;

        // create player entity and set up input state
        auto player = ecs.Create();

        // Gameplay components
        ecs.Emplace<PlayerState>(player);
        ecs.Emplace<PlayerTag>(player);
        ecs.Emplace<PlayerInput>(player);

        ecs.Emplace<PlayerParams>(
            player,
            PlayerParams{ .moveSpeed        = cfg.player.moveSpeed,
                          .jumpSpeed        = cfg.player.jumpSpeed,
                          .mouseSensitivity = cfg.player.mouseSensitivity });

        // Camera
        ecs.Emplace<CameraAttachment>(player, cfg.player.cameraAttachment);
        ecs.Emplace<Health>(player, cfg.player.maxHealth);

        ecs.Emplace<Weapon>(
            player,
            Weapon{ .type             = cfg.weapon.type,
                    .cooldown         = 0.0f,
                    .timeBetweenShots = cfg.weapon.timeBetweenShots,
                    .weaponCooldown   = 0.0f,
                    .damage           = cfg.weapon.damage,
                    .range            = cfg.weapon.range });
        ecs.Emplace<Combo>(player);
        ecs.Emplace<ZoneTracker>(player);

        // Physics
        ecs.Emplace<Velocity>(player);

        TransformC& transform = ecs.Emplace<TransformC>(player, cfg.player.spawnTransform);
        Vector3     euler     = transform.rotation.ToEuler();
        ecs.Emplace<CameraLook>(player, euler.y, euler.x);

        transform.rotation = DirectX::XMQuaternionRotationAxis({ 0, 1, 0 }, PI);

        Physics& physics = ecs.ContextRef<Physics>();
        ecs.Emplace<CharacterController>(
            player,
            physics.CreateCharacter(
                { .height   = cfg.player.characterHeight,
                  .radius   = cfg.player.characterRadius,
                  .position = transform.position,
                  .rotation = transform.rotation }));

        // sound
        ecs.Emplace<RecurringSound>(
            player,
            cfg.player.footstepCooldown,
            cfg.player.footstepStartElapsed,
            cfg.player.footstepSoundKey);

        // controls
        ecs.Emplace<Controls>(
            player,
            Controls{
                .forward  = Keys::W,
                .backward = Keys::S,
                .left     = Keys::A,
                .right    = Keys::D,
                .jump     = Keys::Space,
                .fire     = Input::Control::MouseButton::LEFT,
            });

        return true;
    }

    bool AudioInitializeSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        auto& soundAPI = ecs.EmplaceContext<SoundAPI>();
        soundAPI.LoadBank("rock");
        soundAPI.LoadBank("walking");
        soundAPI.LoadBank("gun");
        soundAPI.LoadBank("enemy");
        soundAPI.LoadBank("Damage");
        soundAPI.LoadBank("Ambient");
        soundAPI.LoadBank("EnemySound");

        //ecs.Emplace<RecurringSound>(e, 54.0f, 0.0f, "rock");

        ecs.EmplaceSystem<SoundEventSystem>();

        soundAPI.PlaySound(TypeAudio::OneShot, "AmbientSound");

        return true;
    }

    bool RuntimeInitializeSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        // Add all run-time systems
        // Emplace in order of dependencies.
        // System relies on player input -> emplace after player input system

        // -- Input-based systems --
        ecs.EmplaceSystem<InputFocusSystem>();
        ecs.EmplaceSystem<GamePauseSystem>();
        ecs.EmplaceSystem<GameInputModeSystem>();
        ecs.EmplaceSystem<GameOverlaySystem>();
        ecs.EmplaceSystem<PlayerInputSystem>();

        // -- Update systems : depend on systems

        // Debug
#ifdef _DEBUG
        ecs.EmplaceSystem<GameEditSystem>();
        ecs.EmplaceSystem<DebugGuiStartSystem>(); // this should go before anything that uses gui
        ecs.EmplaceSystem<DebugSpawnEntitySystem>();
        ecs.EmplaceSystem<DebugDrawZonesSystem>();
        ecs.EmplaceSystem<DebugLevelEditorSystem>();
#endif

        // -- Gameplay systems : depend on update --
        ecs.EmplaceSystem<MainMenuSystem>();
        ecs.EmplaceSystem<PlayerUpdateSystem>();
        ecs.EmplaceSystem<InvulnerabilitySystem>();
        ecs.EmplaceSystem<WeaponUpdateSystem>();
        ecs.EmplaceSystem<ComboDrainSystem>();
        ecs.EmplaceSystem<ComboSystem>();
        ecs.EmplaceSystem<EnemyTargetSystem>();
        ecs.EmplaceSystem<HitEnemySystem>();
        ecs.EmplaceSystem<DeadEnemySystem>();
        ecs.EmplaceSystem<WobbleSystem>();
        ecs.EmplaceSystem<ZoneDetectionSystem>();
        ecs.EmplaceSystem<ZoneEnterSystem>();
        ecs.EmplaceSystem<EncounterUpdateSystem>();
        ecs.EmplaceSystem<UpdateHudSystem>();
        ecs.EmplaceSystem<ShotsFiredSystem>();
        ecs.EmplaceSystem<AudioSystem>();
        //ecs.EmplaceSystem<EnemyAudioSystem>();

        // -- Physics systems : change and correct what happens after update and input
        ecs.EmplaceSystem<PhysicsPreUpdateSystem>();
        ecs.EmplaceSystem<PhysicsUpdateSystem>();
        ecs.EmplaceSystem<PhysicsPostUpdateSystem>();

        // -- pre-pass systems : right before we render --

        // player state should be set after all the others systems have touched it
        ecs.EmplaceSystem<PlayerStateSystem>();
        ecs.EmplaceSystem<PlayerDeathSystem>();
        ecs.EmplaceSystem<GameOverSystem>();

        ecs.EmplaceSystem<BuildRenderWorldSystem>();

#ifdef _DEBUG
        // close Imgui
        ecs.EmplaceSystem<DebugGuiEndSystem>();
#endif
        // Signal that we are ready to run the game loop
        ecs.ContextRef<GameState>().stage = GameState::Stage::MainMenu;

        return true;
    }

    bool LoadSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        LOG_DEBUG("Loading...");

        // emplace first in case systems rely on context
        ecs.EmplaceContext<FrameEvents>();
        ecs.EmplaceContext<Hud>();
        ecs.EmplaceContext<DebugSettings>();

        // World Create system emplaces all the components required in World
        ecs.EmplaceSystem<WorldCreateSystem>();

        // Initialize the player system
        ecs.EmplaceSystem<PlayerInitializeSystem>();

        //initial�ze the audio system
        ecs.EmplaceSystem<AudioInitializeSystem>();

        // Initialize HUD
        ecs.EmplaceSystem<HUDInitializeSystem>();

        // Add all the runtime systems. This needs to be LAST!!
        ecs.EmplaceSystem<RuntimeInitializeSystem>();

        // remove system after first iteration update
        return true;
    }

    bool WorldCreateSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        auto& assets = ecs.ContextRef<AssetManager>();
        auto& cfg    = ecs.Context<Config::GameConfig>();

        // Initializes the initial scene
        // TODO: add all static geometry, i.e. level, here

        // GUN
        {
            Entity e = AddModelComponent(ecs, cfg.viewModel.name, cfg.viewModel.localTransform);
            ecs.Emplace<ViewModelTag>(e);
        }

        // LEVEL
        {
            Physics& physics = ecs.ContextRef<Physics>();

            // create level and add its data to the physics for collision
            auto ent = ecs.Create();
            ecs.Emplace<LevelTag>(ent);

            auto& modelC = ecs.Emplace<ModelComponent>(ent, CreateModelComponentFromConfig(assets, cfg.level.model));
            if (!modelC.id)
            {
                LOG_ERROR("Failed to load level " << cfg.level.model.name);
                THROWRE("Level load failed.");
            }

            // root/global transform
            auto& transform = ecs.Emplace<TransformC>(ent);

            // Create a mesh for each part/mesh that makes up the level's model.
            if (const auto model = assets.TryGetModel(modelC.id))
            {
                for (auto& part : model->parts)
                {
                    if (const auto mesh = assets.TryGetMesh(part.meshId))
                    {
                        // Consider attaching to entities
                        physics.CreateMesh(transform.position, mesh->positions, mesh->indices);
                    }
                }
            }
        }

        // Ugly encounters
        {
            // Pre-load doors
            assets.LoadModel(assets.RegisterModel("forcefield.glb"));

            // First encounter
            {
                auto encounter = ecs.Create();
                ecs.Emplace<Encounter>(encounter);
                ecs.Emplace<ZoneBoundary>(encounter, Vector3{ -20.0f, 2.5f, -10.0f }, Vector3{ 7.5f, 5.0f, 10.0f });

                for (int i = 0; i < 3; i++)
                {
                    auto enemySpawn = ecs.Create();
                    ecs.Emplace<EnemySpawn>(enemySpawn, encounter);
                    ecs.Emplace<TransformC>(enemySpawn, TransformC{ .position = { -25.0f, 1.0f, -5.0f - i * 2.0f } });
                }

                auto entrance = ecs.Create();
                ecs.Emplace<DoorSpawn>(entrance, encounter, Vector3{ 0.0f, 2.0f, -3.0f });
                ecs.Emplace<TransformC>(
                    entrance,
                    TransformC{ .position = { -11.0f, 0.0f, -8.5f },
                                .rotation = Quaternion::CreateFromAxisAngle(Vector3::Up, PI_DIV2) });

                auto exit = ecs.Create();
                ecs.Emplace<DoorSpawn>(exit, encounter, Vector3{ -2.0f, 2.0f, -1.0f });
                ecs.Emplace<TransformC>(
                    exit,
                    TransformC{ .position = { -19.7f, 0.0f, 1.8f },
                                .rotation = Quaternion::CreateFromAxisAngle(Vector3::Up, -PI * 0.25f) });
            }

            // Second encounter
            {
                auto encounter = ecs.Create();
                ecs.Emplace<Encounter>(encounter);
                ecs.Emplace<ZoneBoundary>(encounter, Vector3{ -45.0f, 2.5f, -1.0f }, Vector3{ 10.0f, 5.0f, 20.0f });

                for (int wave = 0; wave < 2; wave++)
                {
                    for (int i = 0; i < 5; i++)
                    {
                        auto enemySpawn = ecs.Create();
                        ecs.Emplace<EnemySpawn>(enemySpawn, encounter, wave);
                        ecs.Emplace<TransformC>(
                            enemySpawn,
                            TransformC{ .position = { -40.0f - i * 2.0f, 2.0f, -10.0f } });
                    }

                    for (int i = 0; i < 5; i++)
                    {
                        auto enemySpawn = ecs.Create();
                        ecs.Emplace<EnemySpawn>(enemySpawn, encounter, wave);
                        ecs.Emplace<TransformC>(
                            enemySpawn,
                            TransformC{ .position = { -40.0f - i * 2.0f, 2.0f, 15.0f } });
                    }
                }

                auto entrance = ecs.Create();
                ecs.Emplace<DoorSpawn>(entrance, encounter, Vector3{ 0.0f, 2.0f, -3.0f });
                ecs.Emplace<TransformC>(
                    entrance,
                    TransformC{ .position = { -31.0f, 0.0f, 5.9f },
                                .rotation = Quaternion::CreateFromAxisAngle(Vector3::Up, PI_DIV2) });

                auto exit = ecs.Create();
                ecs.Emplace<DoorSpawn>(exit, encounter, Vector3{ 0.0f, 2.0f, -3.0f });
                ecs.Emplace<TransformC>(
                    exit,
                    TransformC{ .position = { -43.0f, 0.0f, 21.0f }, .rotation = Quaternion::Identity });
            }

            // Third encounter
            {
                auto encounter = ecs.Create();
                ecs.Emplace<Encounter>(encounter);
                ecs.Emplace<ZoneBoundary>(encounter, Vector3{ -42.5f, -4.0f, 50.0f }, Vector3{ 13.0f, 3.0f, 13.0f });

                for (int i = 0; i < 3; i++)
                {
                    auto enemySpawn = ecs.Create();
                    ecs.Emplace<EnemySpawn>(enemySpawn, encounter);
                    float zOffset = static_cast<float>(i);
                    ecs.Emplace<TransformC>(
                        enemySpawn,
                        TransformC{ .position = { -50.5f, -4.0f, 47.0f + zOffset + 3.0f } });
                }

                for (int i = 0; i < 3; i++)
                {
                    auto enemySpawn = ecs.Create();
                    ecs.Emplace<EnemySpawn>(enemySpawn, encounter);
                    ecs.Emplace<TransformC>(enemySpawn, TransformC{ .position = { -35.5f, -4.0f, 47.0f + i + 3.0f } });
                }

                auto entrance = ecs.Create();
                ecs.Emplace<DoorSpawn>(entrance, encounter, Vector3{ 0.0f, 2.0f, -3.0f });
                ecs.Emplace<TransformC>(
                    entrance,
                    TransformC{ .position = { -42.5f, -6.0f, 35.0f },
                                .rotation = Quaternion::CreateFromAxisAngle(Vector3::Up, PI) });

                auto exit = ecs.Create();
                ecs.Emplace<DoorSpawn>(exit, encounter, Vector3{ 0.0f, 2.0f, -3.0f });
                ecs.Emplace<TransformC>(
                    exit,
                    TransformC{ .position = { -42.5f, -6.0f, 64.0f }, .rotation = Quaternion::Identity });
            }
        }
        // Win zone
        {
            auto winZone = ecs.Create();
            ecs.Emplace<WinZone>(winZone);
            ecs.Emplace<ZoneBoundary>(winZone, Vector3{ -42.5f, -4.0f, 80.0f }, Vector3{ 13.0f, 3.0f, 13.0f });
        }

        // SCENE/CAMERA
        {
            auto& uiview = ecs.ContextRef<UiViewport>();

            auto   camEntity = ecs.Create();
            Camera cam{
                .position = { 0.0f, 2.0f, 0.0f },
                .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
                .lens     = PerspectiveProjection{ .fovY        = ToRadians(60.0f),
                                                   .aspectRatio = uiview.width / uiview.height,
                                                   .nearZ       = 0.1f,
                                                   .farZ        = 1000.0f },
            };
            ecs.Emplace<Camera>(camEntity, cam);
            ecs.Emplace<MainCameraTag>(camEntity);
        }

        // SCENE/LIGHTS
        {
            auto lightEntity = ecs.Create();

            PointLight l{
                .color     = Vector3{ 1.0f, 0.85f, 0.7f },
                .position  = Vector3{ 0.0f, 3.0f, 0.0f },
                .intensity = 0.5f,
                .maxRange  = 7.0f,
            };

            ecs.Emplace<PointLight>(lightEntity, l);
        }
        // emplace from game config
        for (const auto& pointLightConfig : cfg.pointLights)
        {
            auto lightEntity = ecs.Create();

            PointLight l{
                .color     = pointLightConfig.color,
                .position  = pointLightConfig.position,
                .intensity = pointLightConfig.intensity,
                .maxRange  = pointLightConfig.maxRange,
            };

            ecs.Emplace<PointLight>(lightEntity, l);
        }

        for (const auto& spotLightConfig : cfg.spotLights)
        {
            auto lightEntity = ecs.Create();

            SpotLight l{
                .color             = spotLightConfig.color,
                .position          = spotLightConfig.position,
                .direction         = spotLightConfig.direction,
                .intensity         = spotLightConfig.intensity,
                .maxRange          = spotLightConfig.maxRange,
                .innerAngleRadians = spotLightConfig.innerAngleRadians,
                .outerAngleRadians = spotLightConfig.outerAngleRadians,
            };
            ecs.Emplace<SpotLight>(lightEntity, l);
        }

        {
            auto lightEntity = ecs.Create();

            SpotLight l{
                .color             = Vector3{ 0.75f, 0.85f, 1.0f },
                .position          = Vector3{ -3.0f, 4.0f, -1.0f },
                .direction         = Vector3{ 0.25f, -1.0f, 0.1f },
                .intensity         = 1.4f,
                .maxRange          = 16.0f,
                .innerAngleRadians = ToRadians(15.0f),
                .outerAngleRadians = ToRadians(30.0f),
            };

            ecs.Emplace<SpotLight>(lightEntity, l);
        }

        return true;
    }

} // namespace Game

#undef LOG_TAG
