#pragma once

#include <Jolt/Jolt.h>

#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyID.h>

#include <unordered_map>

#include <Core/MurderCore.hpp>

JPH_SUPPRESS_WARNINGS

namespace Murder
{
    using PhysicsID = uint32_t;

    inline constexpr PhysicsID NullPhysicsID = 0;
};

enum class BodyType : uint8_t
{
    STATIC,
    DYNAMIC
};

struct CharacterDesc
{
    float height = 2.0f;
    float radius = 0.5f;
    float mass = 50.0f;
    Murder::Vector3 position{};
    Murder::Quaternion rotation{};
};

struct CharacterInput
{
    Murder::Vector3 desiredVelocity{};
    Murder::Vector3 jumpVelocity{};
};

enum class GroundState
{
    ON_GROUND,
    ON_STEEP_GROUND,
    NOT_SUPPORTED,
    IN_AIR
};

struct RayResult
{
    Murder::PhysicsID hitBody = Murder::NullPhysicsID;
    float fraction = 0.0f;
    float distance = 0.0f;
};

class Physics
{
public:
    Physics();
    ~Physics();

    Physics(const Physics&) = delete;
    Physics& operator=(const Physics&) = delete;

    void Init();
    void Remove();

    void Update(float dt);

    Murder::PhysicsID CreatePlane(Murder::Vector3 position, Murder::Vector3 normal);
    Murder::PhysicsID CreateTriangle(Murder::Vector3 v0, Murder::Vector3 v1, Murder::Vector3 v2);
    Murder::PhysicsID CreateMesh(Murder::Vector3 position, const std::vector<Murder::Vector3>& vertexPositions, const std::vector<uint32_t>& indices);

    Murder::PhysicsID CreateBox(Murder::Vector3 position, Murder::Quaternion rotation, Murder::Vector3 extents, BodyType type);
    Murder::PhysicsID CreateSphere(Murder::Vector3 position, float radius, BodyType type);
    Murder::PhysicsID CreateCapsule(Murder::Vector3 position, Murder::Quaternion rotation, float height, float radius, BodyType type);
    Murder::PhysicsID CreateCylinder(Murder::Vector3 position, Murder::Quaternion rotation, float height, float radius, BodyType type);

    Murder::PhysicsID CreateCharacter(const CharacterDesc& desc);
    
    void DestroyBody(Murder::PhysicsID id);

    void DestroyCharacter(Murder::PhysicsID id);
    
    /// @return true if transform was found
    bool GetBodyTransform(Murder::PhysicsID id, Murder::Transform& transform) const;
    /// @return true if transform was set
    bool SetBodyTransform(Murder::PhysicsID id, const Murder::Transform& transform);
    /// @return true if velocity was found
    bool GetBodyVelocity(Murder::PhysicsID id, Murder::Vector3& velocity) const;
    /// @return true if velocity was set
    bool SetBodyVelocity(Murder::PhysicsID id, Murder::Vector3 velocity);
    /// @param[out] gravity scalar applied to the global gravity
    /// @return true if gravity was found
    bool GetBodyGravityFactor(Murder::PhysicsID id, float& gravity);
    /// @param gravity scalar applied to the global gravity
    /// @return true if gravity was set
    bool SetBodyGravityFactor(Murder::PhysicsID id, float gravity);

    /// @return true if input was set
    bool SetCharacterInput(Murder::PhysicsID id, CharacterInput input);
    /// @return true if transform was found
    bool GetCharacterTransform(Murder::PhysicsID id, Murder::Transform& transform) const;
    /// @return true if transform was set
    bool SetCharacterTransform(Murder::PhysicsID id, const Murder::Transform& transform);
    /// @return true if velocity was found
    bool GetCharacterVelocity(Murder::PhysicsID id, Murder::Vector3& velocity) const;
    /// @return true if velocity was set
    bool SetCharacterVelocity(Murder::PhysicsID id, Murder::Vector3 velocity);
    /// @param[out] gravity scalar applied to the global gravity
    /// @return true if gravity was found
    bool GetCharacteryGravityFactor(Murder::PhysicsID id, float& gravity);
    /// @param gravity scalar applied to the global gravity
    /// @return true if gravity was set
    bool SetCharacteryGravityFactor(Murder::PhysicsID id, float gravity);

    /// @return true if grounded state was found
    bool GetCharacterGroundedState(Murder::PhysicsID id, GroundState& state);

    /// @return true if body was found and is active
    bool IsBodyActive(Murder::PhysicsID id);
    /// @return true if state was updated
    bool SetBodyActive(Murder::PhysicsID id, bool active);
    /// @return true if state was updated
    bool ActivateBody(Murder::PhysicsID id);
    /// @return true if state was updated
    bool DeactivateBody(Murder::PhysicsID id);

    bool RayCast(Murder::Vector3 origin, Murder::Vector3 direction, float distance, RayResult& result);

    // Exposed in case something isn't covered
    JPH::PhysicsSystem* GetSystem() const { return m_PhysicsSystem; }

private:
    class BroadPhaseLayerInterfaceImpl;

    struct CharacterData;

private:
    // Helper methods

    CharacterData* GetCharacterData(Murder::PhysicsID id) const;
    JPH::BodyID* GetBodyID(Murder::PhysicsID id) const;
    
    Murder::PhysicsID CreateBody(JPH::Ref<JPH::Shape> shape, Murder::Vector3 position, Murder::Quaternion rotation, BodyType type);

    void UpdateCharacter(const CharacterData& data, float dt);

private:
    JPH::PhysicsSystem* m_PhysicsSystem = nullptr;
    JPH::TempAllocator* m_TempAllocator = nullptr;
    JPH::JobSystemThreadPool* m_JobSystem = nullptr;

    BroadPhaseLayerInterfaceImpl* m_BroadPhaseLayerInterface = nullptr;
    JPH::ObjectVsBroadPhaseLayerFilter m_ObjectVsBroadPhaseLayerFilter;
    JPH::ObjectLayerPairFilter m_ObjectLayerPairFilter;

    // Consider merging these
    Murder::PhysicsID m_BodyIdCounter = 1;
    Murder::PhysicsID m_CharacterIdCounter = 1;

    std::unordered_map<Murder::PhysicsID, JPH::BodyID> m_BodyIds; // Bodies are stored directly in Jolt
    std::unordered_map<Murder::PhysicsID, CharacterData> m_Characters;

    // --------- Values that should be configurable ---------
    
    // The size for temporary allocations during physics updates (in bytes)
    const size_t m_AllocatorSize = 10 * 1024 * 1024;
    // The max amount of rigid bodies that can be added to the physics system
    const JPH::uint m_MaxBodies = 65536;
    // How many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings
    const JPH::uint m_NumBodyMutexes = 0;
    // The max amount of body pairs that can be queued at any time (the broad phase will detect overlapping body pairs
    // based on their bounding boxes and will insert them into a queue for the narrowphase)
    const JPH::uint m_MaxBodyPairs = 65536;
    // The maximum size of the contact constraint buffer. If more contacts (collisions between bodies) than this are detected,
    // then these contacts will be ignored and bodies will start interpenetrating / fall through the world
    const JPH::uint m_MaxContactConstraints = 10240;
};
