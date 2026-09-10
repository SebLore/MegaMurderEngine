#pragma once

#include <cstdint>

namespace Game
{
    struct PlayerState
    {
        enum class Life : uint8_t
        {
            Alive,
            Dying,
            Dead
        };

        enum class Movement : uint8_t
        {
            Idle,
            Moving,
            Jumping,
            Falling
        };

        enum class Action : uint8_t
        {
            None,
            Stunned // do we wanna get stunned when hit?
        };

        Life       life = Life::Alive;
        Movement movement = Movement::Idle;
        Action     action = Action::None;

        bool grounded = false;
        bool controllable = true;
        bool vulnerable = true;

        bool IsAlive() const { return life == Life::Alive; }
        bool IsDying() const { return life == Life::Dying; }
        bool IsDead()  const { return life == Life::Dead; }

        bool CanMove() const
        {
            return life == Life::Alive && controllable && action != Action::Stunned;
        }

        bool CanLook() const
        {
            return life == Life::Alive && controllable;
        }

        bool CanFire() const
        {
            return life == Life::Alive
                && controllable
                && action != Action::Stunned;
        }
    };
}