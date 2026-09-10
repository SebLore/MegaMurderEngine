#include "Physics.h"

#include <Jolt/Math/Vec3.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/PlaneShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/TriangleShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>

#include <Utility/Logging.h>

#define LOG_TAG "Physics"

using namespace JPH;
using namespace Murder;

// Code based on:
// https://jrouwe.github.io/JoltPhysics/index.html
// https://github.com/jrouwe/JoltPhysics/blob/master/HelloWorld/HelloWorld.cpp
// https://github.com/jrouwe/JoltPhysics/blob/master/Samples/Tests/Character/CharacterVirtualTest.cpp

namespace JoltMath
{
    inline Vec3 ToJolt(const Vector3& in)
    {
        return { in.x, in.y, in.z };
    }

    inline Quat ToJolt(const Quaternion& in)
    {
        return { in.x, in.y, in.z, in.w };
    }

    inline Vector3 FromJolt(const Vec3& in)
    {
        return { in.GetX(), in.GetY(), in.GetZ() };
    }

    inline Quaternion FromJolt(const Quat& in)
    {
        return { in.GetX(), in.GetY(), in.GetZ(), in.GetW() };
    }
}

namespace Layers
{
    static constexpr ObjectLayer NON_MOVING(0);
    static constexpr ObjectLayer MOVING(1);
    static constexpr ObjectLayer NUM_LAYERS(2);
};

namespace BroadPhaseLayers
{
    static constexpr BroadPhaseLayer NON_MOVING(0);
    static constexpr BroadPhaseLayer MOVING(1);
    static constexpr uint NUM_LAYERS(2);
};

class Physics::BroadPhaseLayerInterfaceImpl : public BroadPhaseLayerInterface
{
public:
    BroadPhaseLayerInterfaceImpl()
    {
        m_ObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        m_ObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
    }

    virtual uint GetNumBroadPhaseLayers() const override
    {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override
    {
        JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
        return m_ObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
    {
        switch ((BroadPhaseLayer::Type)inLayer)
        {
        case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING: return "NON_MOVING";
        case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING: return "MOVING";
        default: JPH_ASSERT(false); return "INVALID";
        }
    }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
    BroadPhaseLayer m_ObjectToBroadPhase[Layers::NUM_LAYERS];
};

static ObjectLayer GetLayer(BodyType bodyType)
{
    return bodyType == BodyType::DYNAMIC ? Layers::MOVING : Layers::NON_MOVING;
}

static EMotionType GetMotionType(BodyType bodyType)
{
    return bodyType == BodyType::DYNAMIC ? EMotionType::Dynamic : EMotionType::Static;
}

struct Physics::CharacterData
{
    JPH::Ref<JPH::CharacterVirtual> character = nullptr;
    CharacterInput input{};
    float gravityFactor = 1.0f;
};

Physics::Physics()
{
    Init();
}

Physics::~Physics()
{
    Remove();
}

void Physics::Init()
{
    // Not good if we'd want more than one Physics instance
    {
        RegisterDefaultAllocator();

        Factory::sInstance = new Factory();

        RegisterTypes();
    }

    // Pre-allocates memory for updates. Use TempAllocatorMalloc to allocate on demand
    m_TempAllocator = new TempAllocatorImpl(m_AllocatorSize);

    // Use JobSystemSingleThreaded for single-threading
    m_JobSystem = new JobSystemThreadPool(
        cMaxPhysicsJobs,
        cMaxPhysicsBarriers,
        std::thread::hardware_concurrency() - 1
    );

    m_BroadPhaseLayerInterface = new BroadPhaseLayerInterfaceImpl();

    m_PhysicsSystem = new PhysicsSystem();
    m_PhysicsSystem->Init(
        m_MaxBodies,
        m_NumBodyMutexes,
        m_MaxBodyPairs,
        m_MaxContactConstraints,
        *m_BroadPhaseLayerInterface,
        m_ObjectVsBroadPhaseLayerFilter,
        m_ObjectLayerPairFilter
    );
}

void Physics::Remove()
{
    BodyIDVector bodyIds;
    m_PhysicsSystem->GetBodies(bodyIds);

    for (BodyID& bodyId : bodyIds)
    {
        BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();

        bodyInterface.RemoveBody(bodyId);
        bodyInterface.DestroyBody(bodyId);
    }

    m_BodyIds.clear();
    m_Characters.clear();

    if (m_PhysicsSystem)
        delete m_PhysicsSystem;
    m_PhysicsSystem = nullptr;

    if (m_BroadPhaseLayerInterface)
        delete m_BroadPhaseLayerInterface;
    m_BroadPhaseLayerInterface = nullptr;

    if (m_JobSystem)
        delete m_JobSystem;
    m_JobSystem = nullptr;

    if (m_TempAllocator)
        delete m_TempAllocator;
    m_TempAllocator = nullptr;

    // Not good if we'd want more than one Physics instance
    {
        UnregisterTypes();

        delete Factory::sInstance;
        Factory::sInstance = nullptr;
    }
}

void Physics::Update(float dt)
{
    // TODO: Look into updating once every 1 / 60th of a second

    for (auto& [id, data] : m_Characters)
    {
        UpdateCharacter(data, dt);
    }

    const int collisionSteps = 1;
    m_PhysicsSystem->Update(dt, collisionSteps, m_TempAllocator, m_JobSystem);
}

void Physics::UpdateCharacter(const CharacterData& data, float dt)
{
    static constexpr float MAX_VERTICAL_VELOCITY = 20.0f;
    static constexpr float MAX_HORIZONTAL_VELOCITY = 20.0f;
    static constexpr float ACCELERATION = 10.0f;

    Ref<CharacterVirtual> character = data.character;
    CharacterInput input = data.input;

    const Vec3& up = character->GetUp();
    const Vec3 gravity = m_PhysicsSystem->GetGravity() * data.gravityFactor;
    Vec3 velocity = character->GetLinearVelocity();

    // Apply forces
    velocity += gravity * dt;

    // Handle input
    bool grounded = character->GetGroundState() == CharacterVirtual::EGroundState::OnGround;

    Vec3 vertical = up * velocity.Dot(up);
    Vec3 horizontal = velocity - vertical;

    Vec3 desiredHorizontal = JoltMath::ToJolt(input.desiredVelocity);
    Vec3 dv = desiredHorizontal - horizontal;

    // This is not quite correct as it just targets desiredVelocity
    horizontal += dv * ACCELERATION * dt;

    if (grounded)
    {
        character->UpdateGroundVelocity();
        Vec3 groundVelocity = character->GetGroundVelocity();

        if (input.jumpVelocity.LengthSquared() > 0.0f)
            vertical = JoltMath::ToJolt(input.jumpVelocity);
        else
            vertical = up * groundVelocity.Dot(up);
    }

    // Clamp
    if (vertical.LengthSq() > MAX_VERTICAL_VELOCITY * MAX_VERTICAL_VELOCITY)
    {
        vertical = vertical.Normalized() * MAX_VERTICAL_VELOCITY;
    }

    if (horizontal.LengthSq() > MAX_HORIZONTAL_VELOCITY * MAX_HORIZONTAL_VELOCITY)
    {
        horizontal = horizontal.Normalized() * MAX_HORIZONTAL_VELOCITY;
    }

    velocity = vertical + horizontal;

    // Sanitize
    if (abs(velocity.GetX()) < FLT_EPSILON)
        velocity.SetX(0.0f);
    if (abs(velocity.GetY()) < FLT_EPSILON)
        velocity.SetY(0.0f);
    if (abs(velocity.GetZ()) < FLT_EPSILON)
        velocity.SetZ(0.0f);

    character->SetLinearVelocity(velocity);

    // Limit rotation around the character's up axis
    Quat rotation = character->GetRotation();
    character->SetRotation(rotation.GetTwist(up));

    // Update
    character->ExtendedUpdate(dt,
        gravity,
        {},
        m_PhysicsSystem->GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
        m_PhysicsSystem->GetDefaultLayerFilter(Layers::MOVING),
        {},
        {},
        *m_TempAllocator
    );

    // Restore rotation
    character->SetRotation(rotation);
}

PhysicsID Physics::CreatePlane(Vector3 position, Vector3 normal)
{
    normal.Normalize();

    Ref<Shape> shape = new PlaneShape(JPH::Plane(JoltMath::ToJolt(normal), 0.0f));
    return CreateBody(shape, position, Quaternion::Identity, BodyType::STATIC);
}

PhysicsID Physics::CreateTriangle(Vector3 v0, Vector3 v1, Vector3 v2)
{
    Ref<Shape> shape = new TriangleShape(
        JoltMath::ToJolt(v0),
        JoltMath::ToJolt(v1),
        JoltMath::ToJolt(v2)
    );
    return CreateBody(shape, Vector3::Zero, Quaternion::Identity, BodyType::STATIC);
}

PhysicsID Physics::CreateMesh(Vector3 position, const std::vector<Vector3>& vertexPositions, const std::vector<uint32_t>& indices)
{
    VertexList vertices;
    vertices.reserve(vertexPositions.size());

    for (const Vector3& pos : vertexPositions)
    {
        vertices.emplace_back(pos.x, pos.y, pos.z);
    }

    IndexedTriangleList triangles;
    triangles.reserve(indices.size() / 3);

    for (size_t i = 0; i < indices.size(); i += 3)
    {
        triangles.emplace_back(indices[i], indices[i + 1], indices[i + 2]);
    }

    MeshShapeSettings settings(vertices, triangles);
    Shape::ShapeResult result = settings.Create();

    if (!result.IsValid())
    {
        return NullPhysicsID;
    }
    return CreateBody(result.Get(), position, Quaternion::Identity, BodyType::STATIC);
}

PhysicsID Physics::CreateBox(Vector3 position, Quaternion rotation, Vector3 extents, BodyType type)
{
    Ref<Shape> shape = new BoxShape(JoltMath::ToJolt(extents));
    return CreateBody(shape, position, rotation, type);
}

PhysicsID Physics::CreateSphere(Vector3 position, float radius, BodyType type)
{
    Ref<Shape> shape = new SphereShape(radius);
    return CreateBody(shape, position, Quaternion::Identity, type);
}

PhysicsID Physics::CreateCapsule(Vector3 position, Quaternion rotation, float height, float radius, BodyType type)
{
    Ref<Shape> shape = new CapsuleShape(height * 0.5f, radius);
    return CreateBody(shape, position, rotation, type);
}

PhysicsID Physics::CreateCylinder(Vector3 position, Quaternion rotation, float height, float radius, BodyType type)
{
    Ref<Shape> shape = new CylinderShape(height * 0.5f, radius);
    return CreateBody(shape, position, rotation, type);
}

PhysicsID Physics::CreateCharacter(const CharacterDesc& desc)
{
    Ref<Shape> shape = new CapsuleShape(desc.height * 0.5f, desc.radius);

    Ref<CharacterVirtualSettings> settings = new CharacterVirtualSettings();
    settings->mShape = shape;
    settings->mShapeOffset = { 0.0f, desc.height * 0.5f + desc.radius, 0.0f }; // Move origin from center to bottom
    settings->mMass = desc.mass;
    settings->mBackFaceMode = EBackFaceMode::IgnoreBackFaces;

    Ref<CharacterVirtual> character = new CharacterVirtual(
        settings,
        JoltMath::ToJolt(desc.position),
        JoltMath::ToJolt(desc.rotation),
        m_PhysicsSystem
    );

    PhysicsID id = m_CharacterIdCounter++;
    m_Characters.emplace(id, CharacterData{ .character = character });
    return id;
}

void Physics::DestroyBody(PhysicsID id)
{
    if (const BodyID* bodyId = GetBodyID(id))
    {
        BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();

        bodyInterface.RemoveBody(*bodyId);
        bodyInterface.DestroyBody(*bodyId);

        m_BodyIds.erase(id);
    }
}

void Physics::DestroyCharacter(PhysicsID id)
{
    if (const CharacterData* data = GetCharacterData(id))
    {
        m_Characters.erase(id);
    }
}

bool Physics::GetBodyTransform(PhysicsID id, Transform& transform) const
{
    if (const BodyID* bodyId = GetBodyID(id))
    {
        BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();
        transform.position = JoltMath::FromJolt(bodyInterface.GetPosition(*bodyId));
        transform.rotation = JoltMath::FromJolt(bodyInterface.GetRotation(*bodyId));
        return true;
    }
    return false;
}

bool Physics::SetBodyTransform(PhysicsID id, const Transform& transform)
{
    if (BodyID* bodyId = GetBodyID(id))
    {
        BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();

        bodyInterface.SetPositionAndRotation(
            *bodyId,
            JoltMath::ToJolt(transform.position),
            JoltMath::ToJolt(transform.rotation),
            EActivation::DontActivate
        );
        return true;
    }
    return false;
}

bool Physics::GetBodyVelocity(PhysicsID id, Vector3& velocity) const
{
    if (const BodyID* bodyId = GetBodyID(id))
    {
        BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();
        velocity = JoltMath::FromJolt(bodyInterface.GetLinearVelocity(*bodyId));
        return true;
    }
    return false;
}

bool Physics::SetBodyVelocity(PhysicsID id, Vector3 velocity)
{
    if (BodyID* bodyId = GetBodyID(id))
    {
        BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();

        bodyInterface.SetLinearVelocity(*bodyId, JoltMath::ToJolt(velocity));
        return true;
    }
    return false;
}

bool Physics::GetBodyGravityFactor(PhysicsID id, float& gravity)
{
    if (const BodyID* bodyId = GetBodyID(id))
    {
        BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();
        gravity = bodyInterface.GetGravityFactor(*bodyId);
        return true;
    }
    return false;
}

bool Physics::SetBodyGravityFactor(PhysicsID id, float gravity)
{
    if (BodyID* bodyId = GetBodyID(id))
    {
        BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();

        bodyInterface.SetGravityFactor(*bodyId, gravity);
        return true;
    }
    return false;
}

bool Physics::SetCharacterInput(PhysicsID id, CharacterInput input)
{
    if (CharacterData* data = GetCharacterData(id))
    {
        data->input = input;
        return true;
    }
    return false;
}

bool Physics::GetCharacterTransform(PhysicsID id, Transform& transform) const
{
    if (const CharacterData* data = GetCharacterData(id))
    {
        Ref<CharacterVirtual> character = data->character;
        transform.position = JoltMath::FromJolt(character->GetPosition());
        transform.rotation = JoltMath::FromJolt(character->GetRotation());
        return true;
    }
    return false;
}

bool Physics::SetCharacterTransform(PhysicsID id, const Transform& transform)
{
    if (CharacterData* data = GetCharacterData(id))
    {
        Ref<CharacterVirtual> character = data->character;

        character->SetPosition(JoltMath::ToJolt(transform.position));
        character->SetRotation(JoltMath::ToJolt(transform.rotation));
        return true;
    }
    return false;
}

bool Physics::GetCharacterVelocity(PhysicsID id, Vector3& velocity) const
{
    if (const CharacterData* data = GetCharacterData(id))
    {
        Ref<CharacterVirtual> character = data->character;
        velocity = JoltMath::FromJolt(character->GetLinearVelocity());
        return true;
    }
    return false;
}

bool Physics::SetCharacterVelocity(PhysicsID id, Vector3 velocity)
{
    if (CharacterData* data = GetCharacterData(id))
    {
        Ref<CharacterVirtual> character = data->character;
        character->SetLinearVelocity(JoltMath::ToJolt(velocity));
        return true;
    }
    return false;
}

bool Physics::GetCharacteryGravityFactor(PhysicsID id, float& gravity)
{
    if (const CharacterData* data = GetCharacterData(id))
    {
        gravity = data->gravityFactor;
        return true;
    }
    return false;
}

bool Physics::SetCharacteryGravityFactor(PhysicsID id, float gravity)
{
    if (CharacterData* data = GetCharacterData(id))
    {
        data->gravityFactor = gravity;
        return true;
    }
    return false;
}

bool Physics::GetCharacterGroundedState(Murder::PhysicsID id, GroundState& state)
{
    if (CharacterData* data = GetCharacterData(id))
    {
        switch (data->character->GetGroundState())
        {
        case CharacterBase::EGroundState::OnGround:
            state = GroundState::ON_GROUND;
            break;
        case CharacterBase::EGroundState::OnSteepGround:
            state = GroundState::ON_STEEP_GROUND;
            break;
        case CharacterBase::EGroundState::NotSupported:
            state = GroundState::NOT_SUPPORTED;
            break;
        case CharacterBase::EGroundState::InAir:
            state = GroundState::IN_AIR;
            break;
        default:
            state = GroundState::IN_AIR;
            break;
        }
        return true;
    }
    return false;
}

bool Physics::IsBodyActive(PhysicsID id)
{
    if (const BodyID* bodyId = GetBodyID(id))
    {
        return m_PhysicsSystem->GetBodyInterface().IsActive(*bodyId);
    }
    return false;
}

bool Physics::SetBodyActive(PhysicsID id, bool active)
{
    if (BodyID* bodyId = GetBodyID(id))
    {
        BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();

        if (active)
            bodyInterface.ActivateBody(*bodyId);
        else
            bodyInterface.DeactivateBody(*bodyId);
        return true;
    }
    return false;
}

bool Physics::ActivateBody(PhysicsID id)
{
    return SetBodyActive(id, true);
}

bool Physics::DeactivateBody(PhysicsID id)
{
    return SetBodyActive(id, false);
}

bool Physics::RayCast(Vector3 origin, Vector3 direction, float distance, RayResult& result)
{
    RRayCast ray(
        JoltMath::ToJolt(origin),
        JoltMath::ToJolt(direction).Normalized() * distance
    );

    RayCastResult res;
    bool hit = m_PhysicsSystem->GetNarrowPhaseQuery().CastRay(ray, res);

    if (!hit)
        return false;

    PhysicsID hitId = NullPhysicsID;

    for (auto& [id, bodyId] : m_BodyIds)
    {
        if (bodyId == res.mBodyID)
        {
            hitId = id;
            break;
        }
    }

    result = {
        .hitBody = hitId,
        .fraction = res.mFraction,
        .distance = res.mFraction * distance
    };

    return hit;
}

Physics::CharacterData* Physics::GetCharacterData(PhysicsID id) const
{
    auto it = m_Characters.find(id);

    if (it == m_Characters.end())
        return nullptr;

    return const_cast<CharacterData*>(&it->second);
}

BodyID* Physics::GetBodyID(PhysicsID id) const
{
    auto it = m_BodyIds.find(id);

    if (it == m_BodyIds.end())
        return nullptr;

    return const_cast<BodyID*>(&it->second);
}

PhysicsID Physics::CreateBody(Ref<Shape> shape, Vector3 position, Quaternion rotation, BodyType type)
{
    BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();

    BodyID bodyId = bodyInterface.CreateAndAddBody(BodyCreationSettings(
        shape,
        JoltMath::ToJolt(position),
        JoltMath::ToJolt(rotation),
        GetMotionType(type),
        GetLayer(type)
    ), EActivation::Activate);

    PhysicsID id = m_BodyIdCounter++;
    m_BodyIds.emplace(id, bodyId);
    return id;
}

#undef LOG_TAG
