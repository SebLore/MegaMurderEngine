#include "pch.h"

#include "Systems.h"
#include "SystemCommon.h"

#include "Audio/AudioSystem.h"
#include "Engine/Hud.h"

#include <Utility/Logging.h>

#define LOG_TAG "Systems"

// STL
#include <string>
#include <functional>

#include "Utility/RNG.h"

using namespace DirectX;
using namespace SimpleMath;
using namespace ECS;

namespace Game
{

    // helper functions

    static bool IsPointInRectangle(Vector2 point, const RectI& rect)
    {
        return (
            point.x >= static_cast<float>(rect.left) && point.x <= static_cast<float>(rect.right) &&
            point.y >= static_cast<float>(rect.top) && point.y <= static_cast<float>(rect.bottom));
    }

    namespace
    {
        using namespace Murder;
    } // namespace

    /*****************************************************************************
    * STATIC SYSTEMS
    *****************************************************************************/

    bool GameInputModeSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        const auto& state = ecs.ContextRef<GameState>();
        auto&       mode  = ecs.ContextRef<InputMode>();

        if (state.IsPaused() || state.IsEnded() || state.IsMainMenu())
        {
            if (!mode.uiMode)
            {
                mode.uiMode           = true;
                mode.suppressLookOnce = true;
            }
        }
        else if (state.IsRunning())
        {
            if (mode.uiMode)
            {
                mode.uiMode           = false;
                mode.suppressLookOnce = true;
            }
        }

        if (state.IsMainMenu())
            ecs.EmplaceSystem<MainMenuSystem>();

        return false;
    }

    bool GamePauseSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        auto& state = ecs.ContextRef<GameState>();
        auto& input = ecs.ContextRef<Input>();

        if (state.IsMainMenu() || state.IsEnded())
            return false;

        if (input.IsPressed(Keyboard::Keys::Escape))
        {
            if (state.IsRunning() || state.IsEditing())
                state.Pause();

            else if (state.IsPaused())
                state.Resume();
        }

        return false;
    }

    bool GameOverSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        auto& gameState = ecs.ContextRef<GameState>();

        if (gameState.IsEnded() || gameState.IsExiting())
            return false;

        for (auto [entity, player] : ecs.View<PlayerTag, PlayerState>().each())
        {
            if (player.IsDead())
            {
                LOG_DEBUG("Player has died.");
                gameState.End();

                auto& delay    = ecs.GetOrEmplaceContext<RestartDelay>();
                delay.duration = 2.0f;
                delay.Reset();

                ecs.EmplaceSystem<GameRestartDelaySystem>();
                break;
            }
        }

        return false;
    }

    bool AudioSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        auto& soundAPI    = ecs.Context<SoundAPI>();
        auto& audioEvents = ecs.Context<FrameEvents>().audio;
        auto  view        = ecs.View<EnemyTag, TransformC, HitTag>();
        for (auto entity : view)
        {
            TransformC& transform = ecs.Get<TransformC>(entity);
            ;
            for (auto& event : audioEvents)
            {
                if (event.key == "monsterDeath")
                {
                    FMOD_VECTOR fmodPos{ transform.position.x, transform.position.y, transform.position.z };

                    // Get forward
                    Vector3 forward = Vector3::Transform(Vector3::UnitZ, transform.rotation);
                    forward.Normalize();
                    // rebuild right/up from world up
                    const Vector3 worldUp = Vector3::UnitY;

                    Vector3 right = worldUp.Cross(forward);
                    if (right.LengthSquared() < FLT_EPSILON)
                        right = Vector3::UnitX;
                    else
                        right.Normalize();

                    Vector3 up = forward.Cross(right);
                    up.Normalize();

                    soundAPI.PlaySound(
                        TypeAudio::Audio3D,
                        event.key,
                        fmodPos,
                        { forward.x, forward.y, forward.z },
                        { up.x, up.y, up.z });
                }
            }
        }
        soundAPI.Update();
        return false;
    }

    bool EnemyAudioSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        auto& events = ecs.Context<FrameEvents>();
        auto  view   = ecs.View<EnemyTag, RecurringSound, TransformC>();

        view.each(
            [&](Entity e, RecurringSound& sound, TransformC& transform)
            {
                sound.elapsed += dt;

                if (sound.elapsed >= sound.cooldown)
                {
                    events.audio.push_back({ sound.key });
                    sound.elapsed -= sound.cooldown;
                }
            });

        return false;
    }

    bool SoundEventSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        auto& audio = ecs.Context<SoundAPI>();

        for (auto& [key] : ecs.Context<FrameEvents>().audio)
        {
            if (key == "monsterDeath")
                continue;
            //else
            audio.PlaySound(TypeAudio::OneShot, key);
        }

        ecs.Context<FrameEvents>().audio.clear();

        return false;
    }

    // ==| Dynamic Systems |==============================================================================
    bool GameRestartDelaySystem::OnUpdate(ECSManager& ecs, float dt)
    {
        auto& gameState = ecs.ContextRef<GameState>();

        if (!gameState.IsEnded())
            return true;

        auto& delay = ecs.Context<RestartDelay>();
        delay.Update(dt);

        if (!delay.Done())
            return false;

        ecs.EmplaceSystem<GameResetSystem>();
        return true;
    }

    // TODO: we reaaaallly need a config file or something to store all these magic numbers and strings
    bool GameResetSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        auto& gameState = ecs.ContextRef<GameState>();
        auto& input     = ecs.ContextRef<Input>();

        if (!gameState.IsEnded())
            return true;

        bool         returnToGame  = false;
        Entity       e             = ecs.FindFirst<GameOverOverlayTag>();
        auto&        menu          = ecs.Get<MenuComponent>(e);
        static float timer         = 1.0f;
        static float time          = 0.0f;
        time                      += dt;

        for (int x = 0; x < menu.buttons.size(); x++)
        {
            Vector2 mousePos = { static_cast<float>(input.GetMouseX()), static_cast<float>(input.GetMouseY()) };

            if (IsPointInRectangle(mousePos, menu.buttons[0].sprite.dstRect))
            {
                if (input.IsPressed(Input::Control::MouseButton::LEFT))
                    gameState.Exit();
            }

            if (IsPointInRectangle(mousePos, menu.buttons[1].sprite.dstRect))
            {
                if (input.IsPressed(Input::Control::MouseButton::LEFT))
                {
                    returnToGame = true; // what does this even do?
                    break;
                }
            }
            return false;
        }

        LOG_DEBUG("Resetting game...");

        auto& cfg     = ecs.Context<Config::GameConfig>();
        auto& physics = ecs.ContextRef<Physics>();

        // TODO: handle this from config
        // Reset players
        for (auto [entity, transform, health, playerParams, velocity, combo, zoneTracker, state] :
             ecs.View<PlayerTag, TransformC, Health, PlayerParams, Velocity, Combo, ZoneTracker, PlayerState>().each())
        {
            transform.position = cfg.player.spawnTransform.position;
            transform.rotation = cfg.player.spawnTransform.rotation;
            transform.scale    = cfg.player.spawnTransform.scale;

            velocity = Vector3::Zero;

            health.hp = health.maxHp = cfg.player.maxHealth;

            playerParams.moveSpeed = cfg.player.moveSpeed;

            combo.score      = 0.0f;
            combo.totalScore = 0.0f;
            combo.rank       = ComboRank::None;

            zoneTracker.current = entt::null;
            zoneTracker.last    = entt::null;

            state.life         = PlayerState::Life::Alive;
            state.movement     = PlayerState::Movement::Idle;
            state.action       = PlayerState::Action::None;
            state.grounded     = false;
            state.controllable = true;
            state.vulnerable   = true;

            ecs.Remove<Invulnerability>(entity);
            ecs.Remove<DeathTimer>(entity);

            if (auto* playerInput = ecs.TryGet<PlayerInput>(entity))
                *playerInput = {};

            if (auto* look = ecs.TryGet<CameraLook>(entity))
            {
                Vector3 euler = transform.rotation.ToEuler();
                look->yaw     = euler.y;
                look->pitch   = euler.x;
            }

            if (auto* character = ecs.TryGet<CharacterController>(entity))
            {
                physics.SetCharacterTransform(*character, transform);
                physics.SetCharacterVelocity(*character, Vector3::Zero);
            }
        }

        // reset hud timer
        if (auto* hud = ecs.TryContext<Hud>())
            hud->timerTime = 0.0f;

        // Remove enemies
        {
            std::vector<Entity> toDestroy;
            for (auto [entity] : ecs.View<EnemyTag>().each())
                toDestroy.push_back(entity);

            for (auto entity : toDestroy)
            {
                if (auto* body = ecs.TryGet<RigidBody>(entity))
                    physics.DestroyBody(*body);

                ecs.Destroy(entity);
            }
        }

        // Reset encounters
        {
            for (auto [entity, encounter] : ecs.View<Encounter>().each())
            {
                encounter.triggered  = false;
                encounter.active     = false;
                encounter.enemyCount = 0;
                encounter.wave       = -1;

                RemoveEncounterDoors(ecs, entity);
            }
        }

        // Reset Win zone
        {
            for (auto [entity, winZone] : ecs.View<WinZone>().each())
                winZone.triggered = false;
        }

        // Clear frame events
        if (auto* events = ecs.TryContext<FrameEvents>())
        {
            events->shots.clear();
            events->hits.clear();
            events->zoneEnters.clear();
            events->zoneExits.clear();
            events->audio.clear();
        }

        // Reset game state
        if (auto* delay = ecs.TryContext<RestartDelay>())
            delay->Reset();

        // Restart game
        gameState.Run();

        // Remove this system
        return true;
    }

    bool MainMenuSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        auto&  gameState = ecs.ContextRef<GameState>();
        auto&  input     = ecs.ContextRef<Input>();
        auto&  mode      = ecs.ContextRef<InputMode>();
        Entity e         = ecs.FindFirst<MainMenuTag>();

        if (!gameState.IsMainMenu())
            return true;

        auto& menu = ecs.Get<MenuComponent>(e);

        //for (int x = 0; x < menu.buttons.size(); x++) // why loop ???
        //{
        Vector2 mousePos = { static_cast<float>(input.GetMouseX()), static_cast<float>(input.GetMouseY()) };

        if (IsPointInRectangle(mousePos, menu.buttons[0].sprite.dstRect))
        {
            if (input.IsPressed(Input::Control::MouseButton::LEFT))
                gameState.Exit();
        }

        if ((IsPointInRectangle(mousePos, menu.buttons[1].sprite.dstRect) &&
             input.IsPressed(Input::Control::MouseButton::LEFT)) ||
            input.IsPressed(Keyboard::Keys::Enter))
        {
            LOG_DEBUG("Stating the game from the menu...");
            gameState.Run();
            mode.uiMode           = false;
            mode.suppressLookOnce = true;
            return true;
        }
        //} // random loop

        return false;
    }

#undef LOG_TAG

} // namespace Game
