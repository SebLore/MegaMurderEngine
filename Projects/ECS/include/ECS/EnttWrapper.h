#pragma once

// needs to be absolute/relative to keep this a utility project
#include "../../../../external/entt/src/entt/entt.hpp"

#include <vector>

namespace ECS
{
    using Entity = entt::entity;

    class EnttWrapper
    {
      public:
        EnttWrapper()  = default;
        ~EnttWrapper() = default;

        /// Creates a new entity
        Entity Create() { return m_Registry.create(); }

        /// Destroy the entity
        void Destroy(Entity e) { m_Registry.destroy(e); }

        // Context variables
        /// Create once (will assert if already present)
        template <class T, class... Args>
        T& EmplaceContext(Args&&... args)
        {
            return m_Registry.ctx().emplace<T>(std::forward<Args>(args)...);
        }

        /// Create if missing, otherwise return existing
        template <class T, class... Args>
        T& GetOrEmplaceContext(Args&&... args)
        {
            auto& ctx = m_Registry.ctx();
            if (auto* ptr = ctx.find<T>())
                return *ptr;
            return ctx.emplace<T>(std::forward<Args>(args)...);
        }

        /// @return true if context was erased
        template <class T>
        bool EraseContext()
        {
            return m_Registry.ctx().erase<T>();
        }

        template <class T>
        T& Context()
        {
            return m_Registry.ctx().get<T>();
        }

        template <class T>
        const T& Context() const
        {
            return m_Registry.ctx().get<T>();
        }


        // ---- Context: references (non-owning) ----
        template <class T>
        void SetContextRef(T& value)
        {
            // replace if present, otherwise emplace
            auto& ctx = m_Registry.ctx();
            if (auto* ptr = ctx.find<std::reference_wrapper<T>>())
                *ptr = std::ref(value);
            else
                ctx.emplace<std::reference_wrapper<T>>(std::ref(value));
        }

        template <class T>
        T& ContextRef()
        {
            return m_Registry.ctx().get<std::reference_wrapper<T>>().get();
        }

        template <class T>
        const T& ContextRef() const
        {
            return m_Registry.ctx().get<std::reference_wrapper<T>>().get();
        }

        template <class T>
        T* TryContextRef()
        {
            if (auto* ptr = m_Registry.ctx().find<std::reference_wrapper<T>>())
                return &ptr->get();
            return nullptr;
        }

        template <class T>
        const T* TryContextRef() const
        {
            if (auto* ptr = m_Registry.ctx().find<std::reference_wrapper<T>>())
                return &ptr->get();
            return nullptr;
        }

        // ---- Context: pointers (also non-owning) ----
        template <class T>
        void SetContextPtr(T* value)
        {
            auto& ctx = m_Registry.ctx();
            if (auto* ptr = ctx.find<T*>())
                *ptr = value;
            else
                ctx.emplace<T*>(value);
        }

        template <class T>
        T* ContextPtr()
        {
            return m_Registry.ctx().get<T*>();
        }

        template <class T>
        const T* ContextPtr() const
        {
            return m_Registry.ctx().get<T*>();
        }

        template <class T>
        T* TryContextPtr()
        {
            if (auto* ptr = m_Registry.ctx().find<T*>())
                return *ptr;
            return nullptr;
        }

        template <class T>
        const T* TryContextPtr() const
        {
            if (auto* ptr = m_Registry.ctx().find<T*>())
                return *ptr;
            return nullptr;
        }

        template <class T>
        T* TryContext()
        {
            return m_Registry.ctx().find<T>();
        }

        template <class T>
        const T* TryContext() const
        {
            return m_Registry.ctx().find<T>();
        }

        template <typename... Owned>
        auto View()
        {
            return m_Registry.view<Owned...>();
        }

        template <typename... Owned, typename... Exclude>
        auto View(entt::exclude_t<Exclude...> ex)
        {
            return m_Registry.view<Owned...>(ex);
        }

        template <typename... Owned, typename... Get>
        auto View(entt::get_t<Get...> get)
        {
            return m_Registry.view<Owned...>(get);
        }

        template <typename... Owned, typename... Get, typename... Exclude>
        auto View(entt::get_t<Get...> get, entt::exclude_t<Exclude...> ex)
        {
            return m_Registry.view<Owned...>(get, ex);
        }

        // -- const --
        template <typename... Owned>
        auto View() const
        {
            return m_Registry.view<Owned...>();
        }

        template <typename... Owned, typename... Exclude>
        auto View(entt::exclude_t<Exclude...> ex) const
        {
            return m_Registry.view<Owned...>(ex);
        }

        template <typename... Owned, typename... Get>
        auto View(entt::get_t<Get...> get) const
        {
            return m_Registry.view<Owned...>(get);
        }

        template <typename... Owned, typename... Get, typename... Exclude>
        auto View(entt::get_t<Get...> get, entt::exclude_t<Exclude...> ex) const
        {
            return m_Registry.view<Owned...>(get, ex);
        }

        // view operations
        template <class T>
        decltype(auto) Get(Entity e)
        {
            return m_Registry.get<T>(e);
        }

        template <class T>
        auto TryGet(Entity e)
        {
            return m_Registry.try_get<T>(e);
        }

        template <class T, class... Args>
        decltype(auto) Emplace(Entity e, Args&&... args)
        {
            return m_Registry.emplace<T>(e, std::forward<Args>(args)...);
        }

        template <class T, class... Args>
        decltype(auto) EmplaceOrReplace(Entity e, Args&&... args)
        {
            return m_Registry.emplace_or_replace<T>(e, std::forward<Args>(args)...);
        }

        template <class... T>
        bool Has(Entity e) const
        {
            return m_Registry.all_of<T...>(e);
        }

        template <class T>
        void Remove(Entity e)
        {
            m_Registry.remove<T>(e);
        }

        // Find
        template <class T>
        Entity FindFirst()
        {
            auto view = m_Registry.view<T>();
            auto it = view.begin();
            return (it == view.end()) ? entt::null : *it;
        }

        template <class T>
        Entity FindFirst() const
        {
            auto view = m_Registry.view<T>();
            auto it = view.begin();
            return (it == view.end()) ? entt::null : *it;
        }

        // destroy by component
        template <class T>
        bool DestroyFirst()
        {
            const Entity e = FindFirst<T>();
            if (e == entt::null)
                return false;

            m_Registry.destroy(e);
            return true;
        }

        template <class T>
        size_t DestroyAll()
        {
            auto view = m_Registry.view<T>();

            std::vector<Entity> toDestroy;
            toDestroy.reserve(view.size_hint());

            for (auto e : view)
                toDestroy.push_back(e);

            for (auto e : toDestroy)
                m_Registry.destroy(e);

            return toDestroy.size();
        }

        /// access registry for specific scenarios
        entt::registry& reg() { return m_Registry; }

      private:
        entt::registry m_Registry;
    };

} // namespace ECS
