#pragma once

#include <array>
#include <memory>

#include "ECS/EnttWrapper.h"
#include <Core/Math/Types.h>

namespace Murder
{
    using Bounds  = AABB;
    using Frustum = DirectX::BoundingFrustum;

    class SpatialTree
    {
      public:
        struct Entry
        {
            ECS::Entity entity{ entt::null };
            Bounds      bounds;
        };

        struct Node
        {
            Bounds                               bounds;
            std::vector<Entry>                   entries;
            std::array<std::unique_ptr<Node>, 8> children;

            Node(const Bounds& box) : bounds(box) {}
            bool IsLeaf() const
            {
                for (int i = 0; i < 8; i++)
                    if (children[i])
                        return false;
                return true;
            }

            Node(const Node&)            = delete;
            Node& operator=(const Node&) = delete;
        };

      public:
        SpatialTree()  = default;
        ~SpatialTree() = default;

        SpatialTree(const Bounds& root, int maxDepth = 5, int maxObjects = 8);
        SpatialTree(float extents, int maxDepth = 5, int maxObjects = 8);

        void Initialize(const Bounds& root, int maxDepth = 5, int maxObjects = 8);
        void Initialize(float extents, int maxDepth = 5, int maxObjects = 8);

        void Insert(ECS::Entity entity, const Bounds& bounds);
        void Insert(const Entry& entry);
        void InsertVector(const std::vector<Entry>& entries);

        void Query(const Frustum& frustum, std::vector<ECS::Entity>& visibleEntities) const;

        void Clear();

        const Bounds* GetRootBounds() const { return m_root ? &m_root->bounds : nullptr; }
        constexpr int GetMaxDepth() const { return m_maxDepth; }
        constexpr int GetMaxObjects() const { return m_maxObjects; }

      private:
        void InsertRecursive(Node* node, const Entry& entry, int depth);

        void Subdivide(Node* node);
        int  GetChildIndex(const Bounds& parent, const Bounds& box) const;

        void QueryRecursive(
            Node*                           node,
            const DirectX::BoundingFrustum& frustum,
            std::vector<ECS::Entity>&       visibleEntities) const;

        static void ClearNode(Node* node);

        // helper function to create a child bb in one of the parent's octants
        static Bounds               MakeChildBB(const Bounds& parent, int index) noexcept;
        static DirectX::BoundingBox ToBoundingBox(const Bounds& box) noexcept;

      private:
        std::unique_ptr<Node> m_root;
        int                   m_maxDepth   = 5;
        int                   m_maxObjects = 8;
    };

} // namespace Murder
