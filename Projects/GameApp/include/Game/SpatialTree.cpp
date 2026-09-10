#include "SpatialTree.h"

namespace Murder
{

    SpatialTree::SpatialTree(const Bounds& root, int maxDepth, int maxObjects)
        : m_root(std::make_unique<Node>(root)), m_maxDepth(maxDepth), m_maxObjects(maxObjects)
    {
    }

    SpatialTree::SpatialTree(float extents, int maxDepth, int maxObjects)
        : m_maxDepth(maxDepth), m_maxObjects(maxObjects)
    {
        Bounds root = AABBFromCenterExtents({ 0.0f, 0.0f, 0.0f }, { extents, extents, extents });

        m_root = std::make_unique<Node>(root);
    }

    void SpatialTree::Initialize(const Bounds& root, int maxDepth, int maxObjects)
    {
        Clear(); // clear existing data
        m_root       = std::make_unique<Node>(root);
        m_maxDepth   = maxDepth;
        m_maxObjects = maxObjects;
    }

    void SpatialTree::Initialize(float extents, int maxDepth, int maxObjects)
    {
        Clear();
        m_maxDepth   = maxDepth;
        m_maxObjects = maxObjects;

        Bounds root = AABBFromCenterExtents({ 0.0f, 0.0f, 0.0f }, { extents, extents, extents });

        m_root = std::make_unique<Node>(root);
    }

    void SpatialTree::Insert(ECS::Entity entity, const Bounds& bounds)
    {
        if (!m_root || entity == entt::null || !bounds.IsValid())
            return;

        InsertRecursive(m_root.get(), Entry{ entity, bounds }, 0);
    }

    void SpatialTree::Insert(const Entry& entry)
    {
        if (!m_root || entry.entity == entt::null || !entry.bounds.IsValid())
            return;

        InsertRecursive(m_root.get(), entry, 0);
    }

    void SpatialTree::InsertVector(const std::vector<Entry>& entries)
    {
        for (const auto& entry : entries)
            Insert(entry);
    }

    void SpatialTree::Query(const Frustum& frustum, std::vector<ECS::Entity>& visibleEntities) const
    {
        if (!m_root)
            return;

        QueryRecursive(m_root.get(), frustum, visibleEntities);
    }

    void SpatialTree::Clear() { ClearNode(m_root.get()); }

    void SpatialTree::InsertRecursive(Node* node, const Entry& entry, int depth)
    {
        // If node has children, try to push down
        if (!node->IsLeaf())
        {
            int childIndex = GetChildIndex(node->bounds, entry.bounds);
            if (childIndex != -1)
            {
                InsertRecursive(node->children[childIndex].get(), entry, depth + 1);
                return;
            }
        }

        // Store index here
        node->entries.push_back(entry);

        // Split if needed
        if (node->entries.size() > m_maxObjects && depth < m_maxDepth)
        {
            if (node->IsLeaf())
                Subdivide(node);

            std::vector<Entry> entries = std::move(node->entries);
            node->entries.clear();

            for (const auto& en : entries)
            {
                const int childIndex = GetChildIndex(node->bounds, en.bounds);

                if (childIndex != -1)
                    InsertRecursive(node->children[childIndex].get(), en, depth + 1);
                else
                    node->entries.push_back(en);
            }
        }
    }

    void SpatialTree::Subdivide(Node* node)
    {
        if (!node || !node->IsLeaf())
            return;

        for (int i = 0; i < 8; i++)
            node->children[i] = std::make_unique<Node>(MakeChildBB(node->bounds, i));
    }

    int SpatialTree::GetChildIndex(const Bounds& parent, const Bounds& box) const
    {
        for (int i = 0; i < 8; i++)
        {
            Bounds child = MakeChildBB(parent, i);

            if (child.Contains(box))
                return i;
        }
        return -1;
    }

    void SpatialTree::QueryRecursive(
        Node*                           node,
        const DirectX::BoundingFrustum& frustum,
        std::vector<ECS::Entity>&       visibleEntities) const
    {
        if (!node)
            return;

        if (!frustum.Intersects(ToBoundingBox(node->bounds)))
            return;

        for (const auto& object : node->entries)
            if (frustum.Intersects(ToBoundingBox(object.bounds)))
                visibleEntities.push_back(object.entity);

        if (!node->IsLeaf())
            for (const auto& child : node->children)
                QueryRecursive(child.get(), frustum, visibleEntities);
    }

    void SpatialTree::ClearNode(Node* node)
    {
        if (!node)
            return;

        node->entries.clear();

        for (auto& child : node->children)
        {
            ClearNode(child.get());
            child.reset();
        }
    }

    Bounds SpatialTree::MakeChildBB(const Bounds& parent, int index) noexcept
    {
        const Vector3 center  = parent.Center();
        const Vector3 extents = parent.Extents();
        const Vector3 half    = extents * 0.5f;

        Vector3 childCenter  = center;
        childCenter.x       += (index & 1) ? half.x : -half.x;
        childCenter.y       += (index & 2) ? half.y : -half.y;
        childCenter.z       += (index & 4) ? half.z : -half.z;

        return AABBFromCenterExtents(childCenter, half);
    }

    DirectX::BoundingBox SpatialTree::ToBoundingBox(const Bounds& box) noexcept
    {
        const DirectX::XMFLOAT3 center  = box.Center();
        const DirectX::XMFLOAT3 extents = box.Extents();

        return DirectX::BoundingBox{ center, extents };
    }
} // namespace Murder
