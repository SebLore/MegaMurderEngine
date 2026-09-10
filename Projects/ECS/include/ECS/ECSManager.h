#pragma once

#include "EnttWrapper.h"
#include "ISystem.h"

#include <vector>
#include <memory>
#include <utility>

// for convenience
#include <type_traits>

// ensure std::ranges

namespace ECS
{
    class ECSManager : public EnttWrapper
    {
      public:
        ECSManager()  = default;
        ~ECSManager() = default;

        /// insert a system
        void EmplaceSystem(std::unique_ptr<ISystem> system)
        {
            m_Systems.emplace_back(std::move(system));
            m_OrderDirty = true;
        }

        /// @brief Emplaces a system with the usage ECS::EmplaceSystem<TSystem>(Args)
        /// @tparam TSystem System inheriting from ISystem
        /// @tparam Args Argument list
        /// @param args arguments
        /// @return Reference to the emplaced pointer
        /// @throws throws if the pointer is not emplaced
        template <class TSystem, class... Args>
        TSystem& EmplaceSystem(Args&&... args)
        {
            static_assert(std::is_base_of_v<ISystem, TSystem>, "TSystem must derive from ISystem");

            auto ptr = std::make_unique<TSystem>(std::forward<Args>(args)...);

            TSystem& ref = *ptr;
            m_Systems.emplace_back(std::move(ptr));
            m_OrderDirty = true;

            return ref;
        }

        /// Emplace multiple systems that don't need arguments
        template <class... TSystems>
        void EmplaceSystems()
        {
            (EmplaceSystem<TSystems>(), ...);
        }

        /// update all systems
        void Update(float dt = 0.0f)
        {
            if (m_OrderDirty)
                RebuildUpdateOrder();

            std::vector<size_t> toRemove;
            toRemove.reserve(m_OrderedSystems.size());

            for (const auto& [system, index] : m_OrderedSystems)
                if (system->OnUpdate(*this, dt))
                    toRemove.push_back(index);

            if (!toRemove.empty())
                RemoveSystems(std::move(toRemove));
        }

        // Getters
        size_t GetSystemCount() const { return m_Systems.size(); }

      private:
        void RebuildUpdateOrder()
        {
            m_OrderedSystems.clear();
            m_OrderedSystems.reserve(m_Systems.size());

            for (size_t i = 0; i < m_Systems.size(); i++)
            {
                auto& s = m_Systems[i];
                m_OrderedSystems.push_back(SystemRef{ s.get(), i });
            }

            // stable_sort keeps insertion order for equal priority
            std::ranges::stable_sort(
                m_OrderedSystems,
                [](const SystemRef a, const SystemRef b)
                {
                    return a.first->Priority() > b.first->Priority(); // higher goes first
                });

            m_OrderDirty = false;
        }

        void RemoveSystems(std::vector<size_t> indices)
        {
            if (indices.empty())
                return;

            // sort and unique indices to remove, so we can remove from m_Systems in one pass
            std::ranges::sort(indices);
            indices.erase(std::ranges::unique(indices).begin(), indices.end());

            // delete from the back to the front so earlier indices don't lose their position
            for (auto it = indices.rbegin(); it != indices.rend(); ++it)
                m_Systems.erase(m_Systems.begin() + *it);

            m_OrderDirty = true;
        }

      private:
        using SystemRef = std::pair<ISystem*, size_t>; // system and insertion order, for stable sorting and removal

        std::vector<std::unique_ptr<ISystem>> m_Systems;

        // handle order
        /// flag to reinsert system pointers. defaults to true to sort after creation
        bool m_OrderDirty = true;

        /// pointers to systems in update order, sorted by priority. updates when m_OrderDirty is true
        std::vector<SystemRef> m_OrderedSystems;
    };

} // namespace ECS
