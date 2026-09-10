#include "Systems.h"
#include "SystemCommon.h"

namespace Game
{
    bool PlayerInputSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        if (!PlayerControlActive(ecs))
            return false;

        auto& i    = ecs.ContextRef<Input>();
        auto& mode = ecs.ContextRef<InputMode>();

        auto view = ecs.View<PlayerTag, Controls, PlayerInput, PlayerState>().each();
        for (auto [entity, controls, input, state] : view)
        {
            input = {};

            if (state.IsDead() || state.IsDying())
                continue;

            // early out if not in "game mode"
            if (mode.uiMode || !mode.appFocused)
                continue; // Continue instead of return to handle multiple players

            // keyboard
            input.moveForward = (i.IsDown(controls.forward) - i.IsDown(controls.backward));
            input.moveRight   = (i.IsDown(controls.right) - i.IsDown(controls.left));
            input.moved       = input.moveForward || input.moveRight;
            input.jump        = i.IsPressed(controls.jump);

            // mouse
            int dx = i.GetMouseDeltaX();
            int dy = i.GetMouseDeltaY();

            if (mode.suppressLookOnce)
            {
                dx                    = 0;
                dy                    = 0;
                mode.suppressLookOnce = false;
            }

            if (!mode.uiMode)
            {
                input.lookDX = static_cast<float>(dx);
                input.lookDY = static_cast<float>(dy);
                input.turned = input.lookDX || input.lookDY;
            }

            // only fire in game mode
            if (GameRunning(ecs))
                input.fired = i.IsPressed(controls.fire);
        }
        return false;
    }

    bool PlayerUpdateSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        if (!PlayerControlActive(ecs))
            return false;

        auto view = ecs.View<
            PlayerTag,
            const PlayerState,
            const PlayerInput,
            const PlayerParams,
            const CharacterController,
            TransformC,
            CameraLook,
            RecurringSound,
            const Weapon>();

        Physics& physics = ecs.ContextRef<Physics>();

        view.each(
            [&](Entity                     e,
                const PlayerState&         state,
                const PlayerInput&         player,
                const PlayerParams&        params,
                const CharacterController& character,
                TransformC&                t,
                CameraLook&                ang,
                RecurringSound&            walkSound,
                const Weapon&              weapon)
            {
                if (state.IsDead() || state.IsDying())
                    return;

                // Rotate first so that movement is relative to new rotation
                if (player.turned)
                {
                    const float radPerPixel = params.mouseSensitivity * 0.001f;

                    ang.yaw   += player.lookDX * radPerPixel;
                    ang.pitch += player.lookDY * radPerPixel;

                    // clamp pitch to avoid flipping
                    constexpr float limit = PI_DIV2 - 0.001f;
                    ang.pitch             = std::clamp(ang.pitch, -limit, limit);

                    t.rotation = Quaternion::CreateFromYawPitchRoll(ang.yaw, ang.pitch, 0.0f);
                    t.rotation.Normalize();
                }

                const Vector3& up = Vector3::Up;
                Vector3        dir{};

                if (player.moved)
                {
                    // Get direction relative to player orientation
                    Vector3 forward = Vector3::Transform(Vector3::UnitZ, t.rotation);
                    Vector3 right   = Vector3::Transform(Vector3::UnitX, t.rotation);

                    // Horizontal forward vector
                    forward -= up * forward.Dot(up);
                    forward.Normalize();

                    dir =
                        forward * static_cast<float>(player.moveForward) + right * static_cast<float>(player.moveRight);
                    dir.Normalize(); // Normalize to prevent faster diagonal movement

                    {
                        GroundState groundState;
                        bool        foundGroundState = physics.GetCharacterGroundedState(character.id, groundState);

                        if (foundGroundState && groundState == GroundState::ON_GROUND)
                        {
                            walkSound.elapsed += dt;
                            if (walkSound.elapsed >= walkSound.cooldown)
                            {
                                auto& events = ecs.Context<FrameEvents>();
                                events.audio.push_back({ walkSound.key });
                                walkSound.elapsed -= walkSound.cooldown;
                            }
                        }
                        else
                        {
                            // Reset when airborne so the walk sound plays after landing
                            walkSound.elapsed = walkSound.cooldown;
                        }
                    }
                }

                physics.SetCharacterInput(
                    character,
                    { .desiredVelocity = dir * params.moveSpeed,
                      .jumpVelocity    = player.jump ? up * params.jumpSpeed : Vector3::Zero });

                if (player.fired)
                {
                    auto&   events     = ecs.Context<FrameEvents>();
                    Vector3 shotOrigin = t.position;
                    Vector3 shotDir    = Vector3::Transform(Vector3::UnitZ, t.rotation);
                    shotDir.Normalize();

                    if (CameraAttachment* attachment = ecs.TryGet<CameraAttachment>(e))
                        shotOrigin += attachment->offset;

                    events.shots.emplace_back(e, shotOrigin, shotDir, weapon.type);
                }
            });
        return false;
    }

    bool PlayerStateSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        Physics& physics = ecs.ContextRef<Physics>();

        auto     view    = ecs.View<PlayerTag, PlayerState, Health, CharacterController, Velocity>();

        for (auto [entity, state, health, character, velocity] : view.each())
        {
            if (state.IsDead() || state.IsDying())
                continue;

            if (health.hp <= 0.0f)
                continue;

            // assume we're alive if hp > 0 and we are neither dead not dying
            state.life = PlayerState::Life::Alive;

            GroundState groundState;
            bool        foundGroundState = physics.GetCharacterGroundedState(character.id, groundState);
            state.grounded               = foundGroundState && groundState == GroundState::ON_GROUND;

            const float horizontalSpeedSq = velocity.x * velocity.x + velocity.z * velocity.z;

            if (!state.grounded)
                state.movement = (velocity.y > 0.05f) ? PlayerState::Movement::Jumping : PlayerState::Movement::Falling;
            else if (horizontalSpeedSq > 0.01f)
                state.movement = PlayerState::Movement::Moving;
            else
                state.movement = PlayerState::Movement::Idle;
        }
        return false;
    }

    bool PlayerDeathSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        // don't bother if player control is not active, e.g. in edit mode or after game over
        if (!PlayerControlActive(ecs))
            return false;

        auto& cfg = ecs.Context<Config::GameConfig>();

        auto view = ecs.View<PlayerTag, PlayerState, Health>().each();

        for (auto [entity, state, health] : view)
        {
            if (state.IsAlive() && health.hp <= 0.0f)
            {
                state.life         = PlayerState::Life::Dying;
                state.controllable = false;
                state.vulnerable   = false;

                // time from 0 hp to game end
                if (!ecs.Has<DeathTimer>(entity))
                    ecs.Emplace<DeathTimer>(entity, cfg.player.deathDuration, 0.0f);
            }

            if (state.IsDying())
            {
                if (auto* timer = ecs.TryGet<DeathTimer>(entity))
                {
                    timer->Update(dt);

                    if (timer->Done())
                    {
                        state.life = PlayerState::Life::Dead;
                        ecs.Remove<DeathTimer>(entity);
                    }
                }
            }
        }

        return false;
    }
} // namespace Game
