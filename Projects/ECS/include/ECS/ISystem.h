#pragma once

#include <cstdint>

namespace ECS
{
    class ECSManager;

    enum class SystemType : uint8_t
    {
        Static,  ///< added once on initialization, never removed
        Dynamic, ///< can be added/removed at runtime
    };

    /// Base system interface, all systems must implement this
    class ISystem
    {
      public:
        virtual ~ISystem() = default;

        /// Return true to remove system from manager, false to keep it
        virtual bool OnUpdate(ECSManager&, float) = 0;

        /// Static or Dynamic
        virtual SystemType GetType() const { return SystemType::Dynamic; }

        /// Higher priority systems update first, default is 0 "no priority"
        virtual int Priority() const { return 0; }; // lower priority systems update first
    };

} // namespace ECS
