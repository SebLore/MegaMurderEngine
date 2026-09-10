#include "DebugDrawQueue.h"

#include <algorithm>
#include <stdexcept>

#include "DebugPrimitives.h"

using namespace DirectX;

void DebugDrawQueue::Initialize(size_t reserveLineSegments)
{
    std::scoped_lock lock(m_Mutex);
    m_frameDepth.reserve(reserveLineSegments);
    m_frameOverlay.reserve(reserveLineSegments);
    m_persistDepth.reserve(reserveLineSegments / 4);
    m_persistOverlay.reserve(reserveLineSegments / 4);
}

void DebugDrawQueue::BeginFrame(float dt)
{
    std::scoped_lock lock(m_Mutex);

    m_Active = true;

    m_frameDepth.clear();
    m_frameOverlay.clear();

    auto age = [dt](std::vector<LineCmd>& v)
    {
        for (auto& l : v)
            l.ttl -= dt;
        std::erase_if(v, [](const LineCmd& l) { return l.ttl <= 0.0f; });
    };

    age(m_persistDepth);
    age(m_persistOverlay);
}

void DebugDrawQueue::EndFrame()
{
    std::scoped_lock lock(m_Mutex);
    m_Active = false;
}

bool DebugDrawQueue::IsActive() const { return m_Active; }

void DebugDrawQueue::Flush(DebugLineRenderer& batch, CXMMATRIX view, CXMMATRIX proj)
{
    std::vector<LineCmd> depth, overlay;
    {
        std::scoped_lock lock(m_Mutex);

        depth.reserve(m_persistDepth.size() + m_frameDepth.size());
        overlay.reserve(m_persistOverlay.size() + m_frameOverlay.size());

        depth.insert(depth.end(), m_persistDepth.begin(), m_persistDepth.end());
        depth.insert(depth.end(), m_frameDepth.begin(), m_frameDepth.end());

        overlay.insert(overlay.end(), m_persistOverlay.begin(), m_persistOverlay.end());
        overlay.insert(overlay.end(), m_frameOverlay.begin(), m_frameOverlay.end());
    }

    if (!depth.empty())
    {
        batch.SetEnableDepth(true);
        batch.Begin(view, proj);

        for (const auto& l : depth)
            batch.Line(XMLoadFloat3(&l.a), XMLoadFloat3(&l.b), XMLoadFloat4(&l.color));

        batch.End();
    }

    if (!overlay.empty())
    {
        batch.SetEnableDepth(false);
        batch.Begin(view, proj);

        for (const auto& l : overlay)
            batch.Line(XMLoadFloat3(&l.a), XMLoadFloat3(&l.b), XMLoadFloat4(&l.color));

        batch.End();
    }
}

// -------------------------
// Record Functions
// -------------------------

void DebugDrawQueue::AddLine(FXMVECTOR a, FXMVECTOR b, FXMVECTOR color, DepthMode mode, float ttlSeconds)
{
    std::scoped_lock lock(m_Mutex);
    if (!m_Active)
        return;

    SelectList(mode, ttlSeconds).push_back(MakeCmd(a, b, color, ttlSeconds));
}

void DebugDrawQueue::AppendLocked(DepthMode mode, float ttlSeconds, const std::vector<LineCmd>& cmds)
{
    auto& list = SelectList(mode, ttlSeconds);
    list.insert(list.end(), cmds.begin(), cmds.end());
}

void DebugDrawQueue::AddAxis(FXMVECTOR origin, float scale, DepthMode mode, float ttlSeconds)
{
    BuildAndAppendLines(
        mode,
        ttlSeconds,
        dbg::details::AxisLines,
        [&](const LineCollector& emit) { dbg::Axis(emit, origin, scale); });
}

void DebugDrawQueue::AddRay(
    FXMVECTOR origin,
    FXMVECTOR direction,
    float     length,
    FXMVECTOR color,
    DepthMode mode,
    float     ttlSeconds)
{
    BuildAndAppendLines(
        mode,
        ttlSeconds,
        /*reserveCount=*/1,
        [&](const LineCollector& emit)
        {
            // Queue API: direction doesn't need to be normalized
            XMVECTOR unitDir = XMVector3Normalize(direction);
            dbg::Ray(emit, origin, unitDir, length, color);
        });
}

void DebugDrawQueue::AddAABB(
    FXMVECTOR minCorner,
    FXMVECTOR maxCorner,
    FXMVECTOR color,
    DepthMode mode,
    float     ttlSeconds)
{
    BuildAndAppendLines(
        mode,
        ttlSeconds,
        dbg::details::AABBEdges,
        [&](const LineCollector& emit) { dbg::AABB(emit, minCorner, maxCorner, color); });
}

void DebugDrawQueue::AddAABBCenterExtents(
    FXMVECTOR center,
    FXMVECTOR extents,
    FXMVECTOR color,
    DepthMode mode,
    float     ttlSeconds)
{
    BuildAndAppendLines(
        mode,
        ttlSeconds,
        dbg::details::AABBEdges,
        [&](const LineCollector& emit) { dbg::AABBCenterExtents(emit, center, extents, color); });
}

void DebugDrawQueue::AddGrid(
    float              halfSize,
    int                divisions,
    DirectX::FXMVECTOR origin,
    DirectX::FXMVECTOR unitPlaneNormal,
    DirectX::FXMVECTOR color,
    DepthMode          mode,
    float              ttlSeconds)
{
    divisions = std::max(divisions, 1);

    // (divisions+1) lines in each direction
    const size_t reserveCount = 2 * (static_cast<size_t>(divisions) + 1);

    BuildAndAppendLines(
        mode,
        ttlSeconds,
        reserveCount,
        [&](const LineCollector& emit) { dbg::Grid(emit, halfSize, divisions, origin, unitPlaneNormal, color); });
}

void DebugDrawQueue::AddGridXZ(
    float     halfSize,
    int       divisions,
    float     y,
    FXMVECTOR color,
    DepthMode mode,
    float     ttlSeconds)
{
    divisions = std::max(divisions, 1);

    // (divisions+1) lines in each direction
    const size_t reserveCount = 2 * (static_cast<size_t>(divisions) + 1);

    BuildAndAppendLines(
        mode,
        ttlSeconds,
        reserveCount,
        [&](const LineCollector& emit) { dbg::GridXZ(emit, halfSize, divisions, y, color); });
}

void DebugDrawQueue::AddCircle(
    FXMVECTOR center,
    FXMVECTOR unitPlaneNormal,
    float     radius,
    int       segments,
    FXMVECTOR color,
    DepthMode mode,
    float     ttlSeconds)
{
    segments                  = std::max(segments, 3);
    const size_t reserveCount = size_t(segments);

    BuildAndAppendLines(
        mode,
        ttlSeconds,
        reserveCount,
        [&](const LineCollector& emit) { dbg::Circle(emit, center, unitPlaneNormal, radius, segments, color); });
}

void DebugDrawQueue::AddArc(
    FXMVECTOR center,
    FXMVECTOR planeNormal,
    FXMVECTOR planeStartDirection,
    float     radius,
    float     angleRadians,
    int       segments,
    FXMVECTOR color,
    DepthMode mode,
    float     ttlSeconds)
{
    segments                  = std::max(segments, 1);
    const size_t reserveCount = static_cast<size_t>(segments);

    BuildAndAppendLines(
        mode,
        ttlSeconds,
        reserveCount,
        [&](const LineCollector& emit)
        { dbg::Arc(emit, center, planeNormal, planeStartDirection, radius, angleRadians, segments, color); });
}

void DebugDrawQueue::AddSphere(
    FXMVECTOR center,
    float     radius,
    int       segments,
    FXMVECTOR color,
    DepthMode mode,
    float     ttlSeconds)
{
    segments                  = std::max(segments, 3);
    const size_t reserveCount = static_cast<size_t>(segments) * 3;

    BuildAndAppendLines(
        mode,
        ttlSeconds,
        reserveCount,
        [&](const LineCollector& emit) { dbg::Sphere(emit, center, radius, segments, color); });
}

void DebugDrawQueue::AddCone(
    FXMVECTOR apex,
    FXMVECTOR unitAxisDirection,
    float     angleRadians,
    float     length,
    int       segments,
    FXMVECTOR color,
    DepthMode mode,
    float     ttlSeconds)
{
    segments = std::max(segments, 3);

    // base ring: segments, spokes: segments
    const size_t reserveCount = static_cast<size_t>(segments) * 2;

    BuildAndAppendLines(
        mode,
        ttlSeconds,
        reserveCount,
        [&](const LineCollector& emit)
        { dbg::Cone(emit, apex, unitAxisDirection, angleRadians, length, segments, color); });
}

void DebugDrawQueue::AddCapsule(
    FXMVECTOR pointA,
    FXMVECTOR pointB,
    float     radius,
    int       segments,
    FXMVECTOR color,
    DepthMode mode,
    float     ttlSeconds)
{
    // want at least 6 segments to have something nearing a circle
    segments = std::max(segments, 6);

    // Estimate:
    // - 2 circles: 2*segments
    // - 4 long cage lines: CapsuleExtraLongs
    // - 4 arcs at segments/2 each: 2*segments
    const size_t reserveCount = static_cast<size_t>(segments) * 4 + dbg::details::CapsuleExtraLongs;

    BuildAndAppendLines(
        mode,
        ttlSeconds,
        reserveCount,
        [&](const LineCollector& emit) { dbg::Capsule(emit, pointA, pointB, radius, segments, color); });
}

void DebugDrawQueue::AddFrustumFromInvViewProj(CXMMATRIX invViewProj, FXMVECTOR color, DepthMode mode, float ttlSeconds)
{
    BuildAndAppendLines(
        mode,
        ttlSeconds,
        dbg::details::FrustumEdges,
        [&](const LineCollector& emit) { dbg::FrustumFromInvViewProj(emit, invViewProj, color); });
}

void DebugDrawQueue::AddCylinder(
    FXMVECTOR base,
    FXMVECTOR normal,
    float     height,
    float     radius,
    int       segments,
    FXMVECTOR color,
    DepthMode mode,
    float     ttlSeconds)
{
    segments = std::max(segments, 3);

    // 2 circles (2*segments) + vertical cage lines (segments) = 3*segments
    const size_t reserveCount = static_cast<size_t>(segments) * 3;

    BuildAndAppendLines(
        mode,
        ttlSeconds,
        reserveCount,
        [&](const LineCollector& emit) { dbg::Cylinder(emit, base, normal, height, radius, segments, color); });
}

// -------------------------
// Global binding
// -------------------------

void DebugDrawQueue::BindGlobal(DebugDrawQueue* instance) { s_Global.store(instance, std::memory_order_release); }

void DebugDrawQueue::UnbindGlobal(DebugDrawQueue* instance)
{
    DebugDrawQueue* cur = s_Global.load(std::memory_order_acquire);
    if (cur == instance)
        s_Global.store(nullptr, std::memory_order_release);
}

void DebugDrawQueue::UnbindGlobalForce() { s_Global.store(nullptr, std::memory_order_release); }

DebugDrawQueue* DebugDrawQueue::TryGlobal() { return s_Global.load(std::memory_order_acquire); }

DebugDrawQueue& DebugDrawQueue::Global()
{
    DebugDrawQueue* ptr = TryGlobal();

    if (!ptr)
        throw std::runtime_error("DebugDrawQueue::Global() used before DebugDrawQueue::BindGlobal()");

    return *ptr;
}

std::vector<DebugDrawQueue::LineCmd>& DebugDrawQueue::SelectList(DepthMode mode, float ttlSeconds)
{
    const bool persistent = ttlSeconds > 0.0f;

    if (mode == DepthMode::Overlay)
        return persistent ? m_persistOverlay : m_frameOverlay;

    return persistent ? m_persistDepth : m_frameDepth;
}
