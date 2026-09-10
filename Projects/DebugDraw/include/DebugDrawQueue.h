#pragma once

#include <DirectXColors.h>
#include <DirectXMath.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

#include "DebugLineRenderer.h"

/**
 * @brief Immediate-mode draw list.
 *
 * @details Create once, register a pointer statically to make sure the life-time is managed.
 * Write draw statements anywhere and they will be recorded (up to a maximum of reserveLineSegments)
 * Present them at any time with a DebugLineRenderer and a Flush call.
 */

enum class DepthMode : uint8_t
{
    DepthRead, ///< depth test ON (occluded by scene)
    Overlay    ///< depth test OFF (always visible in 3D)
};

class DebugDrawQueue
{
  public:
    /// depth test mode settings

    /// data to record a single line segment with time-to-live
    struct LineCmd
    {
        DirectX::XMFLOAT3 a{};
        DirectX::XMFLOAT3 b{};
        DirectX::XMFLOAT4 color{ 1.0f, 1.0f, 1.0f, 1.0f };

        float ttl = 0.0f; ///< time-to-live in seconds, 0 draws once
    };

  public:
    /**
     * @brief Initialize, optionally with a reserve size to determine how many lines can be queued before a flush needs to happen.
     * @note Can be called at any time to resize the storage, it should be thread safe and will not interrupt recording or flushing in progress.
     * If the reserve size is smaller than the current number of recorded lines, the excess lines will be dropped on the next flush.
     */
    void Initialize(size_t reserveLineSegments = 4096);

    // Gate recording; clears per-frame lists; ages persistent lists.
    void BeginFrame(float dtSeconds);
    void EndFrame();
    bool IsActive() const;

    /// @brief Call when want to render recorded lines with a camera view and projection.
    /// @param batch instance of DebugLineRenderer to handle the drawing
    /// @param view camera view matrix
    /// @param proj camera projection matrix
    void Flush(DebugLineRenderer& batch, DirectX::CXMMATRIX view, DirectX::CXMMATRIX proj);

    // ========================================================================
    // Immediate-mode record API
    // ========================================================================

    /// Add a single line between points a and b to the queue
    void AddLine(
        DirectX::FXMVECTOR a,
        DirectX::FXMVECTOR b,
        DirectX::FXMVECTOR color,
        DepthMode          mode       = DepthMode::DepthRead,
        float              ttlSeconds = 0.0f);

    /// Draw an axis gizmo at the given point. Axes are orthonormalized and scaled to world-units by scale parameter
    void AddAxis(
        DirectX::FXMVECTOR origin,
        float              scale      = 1.0f,
        DepthMode          mode       = DepthMode::DepthRead,
        float              ttlSeconds = 0.0f);
    /**
     * @brief Draw a line from a given origin in a given direction by a given length in world units.
     * @param origin Start of the ray
     * @param direction direction vector of the ray, does not need to be normalized
     * @param length length of the ray in world units
     * @param color color of the line
     * @param mode
     * @param ttlSeconds time-to-live in seconds, after which the line will be removed from the queue
     */
    void AddRay(
        DirectX::FXMVECTOR origin,
        DirectX::FXMVECTOR direction,
        float              length,
        DirectX::FXMVECTOR color,
        DepthMode          mode       = DepthMode::DepthRead,
        float              ttlSeconds = 0.0f);

    /**
     * @brief Draw an axis-aligned bounding box defined by min and max points
     * @param minCorner minimum corner point
     * @param maxCorner maximum corner point
     * @param color color of the box
     * @param mode depth mode
     * @param ttlSeconds time-to-live in seconds
     */
    void AddAABB(
        DirectX::FXMVECTOR minCorner,
        DirectX::FXMVECTOR maxCorner,
        DirectX::FXMVECTOR color,
        DepthMode          mode       = DepthMode::DepthRead,
        float              ttlSeconds = 0.0f);

    /**
     * @brief Draw an axis-aligned bounding box defined by a center point and extents in each direction.
     * @param center center point of the box
     * @param extents extents in each direction from the center to the faces of the box (half-size)
     * @param color color of the box
     * @param mode depth mode
     * @param ttlSeconds time-to-live in seconds, defaults to 0
     */
    void AddAABBCenterExtents(
        DirectX::FXMVECTOR center,
        DirectX::FXMVECTOR extents,
        DirectX::FXMVECTOR color,
        DepthMode          mode       = DepthMode::DepthRead,
        float              ttlSeconds = 0.0f);

    /**
     * @brief Draw a grid on a plane defined by an origin and a normal, with the given half-size and number of divisions. The grid will be centered at the origin, and extend in both directions along the plane up to the half-size. The number of divisions determines how many lines are drawn across the grid, with more divisions resulting in a denser grid. The color, depth mode, and time-to-live can also be specified for the grid lines.
     * @param halfSize
     * @param divisions 
     * @param origin 
     * @param unitPlaneNormal 
     * @param color 
     * @param mode 
     * @param ttlSeconds 
     */
    void AddGrid(
        float              halfSize,
        int                divisions,
        DirectX::FXMVECTOR origin,
        DirectX::FXMVECTOR unitPlaneNormal,
        DirectX::FXMVECTOR color,
        DepthMode          mode       = DepthMode::DepthRead,
        float              ttlSeconds = 0.0f);
    /**
     * @brief Draw a grid on the XZ plane centered at the origin, with the given half-size and number of divisions.
     * @param halfSize
     * @param divisions
     * @param y
     * @param color
     * @param mode
     * @param ttlSeconds
     */
    void AddGridXZ(
        float              halfSize,
        int                divisions,
        float              y,
        DirectX::FXMVECTOR color,
        DepthMode          mode       = DepthMode::DepthRead,
        float              ttlSeconds = 0.0f);

    void AddCircle(
        DirectX::FXMVECTOR center,
        DirectX::FXMVECTOR unitPlaneNormal,
        float              radius,
        int                segments,
        DirectX::FXMVECTOR color,
        DepthMode          mode       = DepthMode::DepthRead,
        float              ttlSeconds = 0.0f);

    void AddArc(
        DirectX::FXMVECTOR center,
        DirectX::FXMVECTOR unitPlaneNormal,
        DirectX::FXMVECTOR unitStartDirectionInPlane,
        float              radius,
        float              angleRadians,
        int                segments,
        DirectX::FXMVECTOR color,
        DepthMode          mode       = DepthMode::DepthRead,
        float              ttlSeconds = 0.0f);

    void AddSphere(
        DirectX::FXMVECTOR center,
        float              radius,
        int                segments,
        DirectX::FXMVECTOR color,
        DepthMode          mode       = DepthMode::DepthRead,
        float              ttlSeconds = 0.0f);

    void AddCone(
        DirectX::FXMVECTOR apex,
        DirectX::FXMVECTOR unitAxisDirection,
        float              angleRadians,
        float              length,
        int                segments,
        DirectX::FXMVECTOR color,
        DepthMode          mode       = DepthMode::DepthRead,
        float              ttlSeconds = 0.0f);

    void AddCapsule(
        DirectX::FXMVECTOR pointA,
        DirectX::FXMVECTOR pointB,
        float              radius,
        int                segments,
        DirectX::FXMVECTOR color,
        DepthMode          mode       = DepthMode::DepthRead,
        float              ttlSeconds = 0.0f);

    void AddFrustumFromInvViewProj(
        DirectX::CXMMATRIX invViewProj,
        DirectX::FXMVECTOR color,
        DepthMode          mode       = DepthMode::DepthRead,
        float              ttlSeconds = 0.0f);

    void AddCylinder(
        DirectX::FXMVECTOR base,
        DirectX::FXMVECTOR normal,
        float              height,
        float              radius,
        int                segments,
        DirectX::FXMVECTOR color,
        DepthMode          mode       = DepthMode::DepthRead,
        float              ttlSeconds = 0.0f);

    // global binding

    /// @brief Binds the specified DebugDrawQueue instance as the global instance.
    /// @param instance Pointer to the DebugDrawQueue instance to set as global.
    static void BindGlobal(DebugDrawQueue* instance);

    /// @brief Unbinds the specified DebugDrawQueue instance from the global context.
    /// @param instance Pointer to the DebugDrawQueue instance to unbind from the global context.
    static void UnbindGlobal(DebugDrawQueue* instance);

    /// @brief Unbinds any DebugDrawQueue instance from the global context, if one is currently bound.
    static void UnbindGlobalForce();

    /// @brief Attempts to retrieve the currently bound global DebugDrawQueue instance
    /// @return Pointer to the currently bound global DebugDrawQueue instance, or nullptr if no instance is currently bound
    static DebugDrawQueue* TryGlobal();

    /// @brief Retrieves the currently bound global DebugDrawQueue instance
    static DebugDrawQueue& Global();

  private:
    inline static std::atomic<DebugDrawQueue*> s_Global{ nullptr }; ///< global instance pointer, can be set and retrieved atomically without locks

    /// @brief Get the list of line commands for the given depth mode
    /// @param mode depth mode to select the list for, depth testing on or off
    /// @param ttlSeconds time-to-live in seconds for the lines in the list
    /// @return Reference to the vector of LineCmd for the given depth mode, with the given TTL. Lines added to this list will be flushed until their TTL expires.
    /// @note Must be called under lock, is not thread safe
    std::vector<LineCmd>& SelectList(DepthMode mode, float ttlSeconds);

    /**
     * @brief Append a list of line commands to the appropriate storage based on the depth mode and TTL, with thread safety.
     * @param mode Depth mode to use when appending the lines.
     * @param ttlSeconds Time-to-live, in seconds, for the appended lines.
     * @param cmds Vector of line commands to append.
     * @note This is called by the public facing API after acquiring the lock, and is not thread safe itself.
     */
    void AppendLocked(DepthMode mode, float ttlSeconds, const std::vector<LineCmd>& cmds);

    /// @brief Helper to build a LineCmd from given parameters, converting from DirectXMath types to the internal storage types.
    /// @param a Starting point of the line
    /// @param b Ending point of the line
    /// @param color Color of the line
    /// @param ttlSeconds Time-to-live in seconds for the line
    /// @return Constructed LineCmd
    static LineCmd MakeCmd(DirectX::FXMVECTOR a, DirectX::FXMVECTOR b, DirectX::FXMVECTOR color, float ttlSeconds)
    {
        LineCmd cmd;
        DirectX::XMStoreFloat3(&cmd.a, a);
        DirectX::XMStoreFloat3(&cmd.b, b);
        DirectX::XMStoreFloat4(&cmd.color, color);
        cmd.ttl = ttlSeconds;
        return cmd;
    }

    /// Helper struct to collect line commands in a batch, passed to the user-provided build function in BuildAndAppendLines. It holds a reference to the output vector and the TTL for the lines, and provides an operator() to add lines to the output vector with the given parameters.
    struct LineCollector
    {
        std::vector<LineCmd>& out;
        float                 ttlSeconds;

        void operator()(DirectX::FXMVECTOR a, DirectX::FXMVECTOR b, DirectX::FXMVECTOR color) const
        {
            out.push_back(MakeCmd(a, b, color, ttlSeconds));
        }
    };

    /// @brief Builds a collection of line commands using a user-provided function and appends them with thread safety.
    /// @tparam BuildFn The type of the callable object used to build the line commands.
    /// @param mode The depth mode to use when appending the lines.
    /// @param ttlSeconds The time-to-live, in seconds, for the appended lines.
    /// @param reserveCount The number of line commands to reserve space for in the temporary collection.
    /// @param buildFn A callable object that receives a LineCollector and populates it with line commands.
    template <class BuildFn>
    void BuildAndAppendLines(DepthMode mode, float ttlSeconds, size_t reserveCount, BuildFn&& buildFn)
    {
        std::vector<LineCmd> tmp;
        tmp.reserve(reserveCount);

        LineCollector emit{ tmp, ttlSeconds };
        buildFn(emit);

        std::scoped_lock lock(m_Mutex);
        if (!m_Active)
            return;

        AppendLocked(mode, ttlSeconds, tmp);
    }

  private:
    mutable std::mutex m_Mutex; ///< used to ensure thread safety, prevent data races
    bool               m_Active = false; ///< set to true to record draw commands, when false no calls are recorded

    // one-frame
    std::vector<LineCmd> m_frameDepth;
    std::vector<LineCmd> m_frameOverlay;

    // persistent with TTL
    std::vector<LineCmd> m_persistDepth;
    std::vector<LineCmd> m_persistOverlay;
};

// macros for nicer handling, don't need to call DebugDrawQueue::Global() over and over

#define DBG_DRAW                    (DebugDrawQueue::Global())
#define DBG_DRAW_BEGIN(dt)          DBG_DRAW.BeginFrame(dt)
#define DBG_DRAW_END()              DBG_DRAW.EndFrame()
#define DBG_DRAW_FLUSH(batch, v, p) DBG_DRAW.Flush((batch), (v), (p))

#define DBG_DRAW_LINE(a, b, color)               DBG_DRAW.AddLine((a), (b), (color))
#define DBG_DRAW_RAY(origin, dir, length, color) DBG_DRAW.AddRay((origin), (dir), (length), (color))
#define DBG_DRAW_AXIS(origin, size)              DBG_DRAW.AddAxis((origin), (size))
#define DBG_DRAW_AABB(min, max, color)           DBG_DRAW.AddAABB((min), (max), (color))
#define DBG_DRAW_AABBCE(center, extents, color)  DBG_DRAW.AddAABBCenterExtents((center), (extents), (color))
#define DBG_DRAW_GRID(halfSize, divisions, origin, normal, color)                                                      \
    DBG_DRAW.AddGrid((halfSize), (divisions), (origin), (normal), (color))
#define DBG_DRAW_GRIDXZ(halfSize, divisions, y, color) DBG_DRAW.AddGridXZ((halfSize), (divisions), (y), (color))
#define DBG_DRAW_CIRCLE(center, normal, radius, segments, color)                                                       \
    DBG_DRAW.AddCircle((center), (normal), (radius), (segments), (color))
#define DBG_DRAW_ARC(center, normal, startDir, radius, angle, segments, color)                                         \
    DBG_DRAW.AddArc((center), (normal), (startDir), (radius), (angle), (segments), (color))
#define DBG_DRAW_SPHERE(center, radius, segments, color) DBG_DRAW.AddSphere((center), (radius), (segments), (color))
#define DBG_DRAW_CONE(apex, dir, angle, length, segments, color)                                                       \
    DBG_DRAW.AddCone((apex), (dir), (angle), (length), (segments), (color))
#define DBG_DRAW_CAPSULE(pointA, pointB, radius, segments, color)                                                      \
    DBG_DRAW.AddCapsule((pointA), (pointB), (radius), (segments), (color))
#define DBG_DRAW_FRUSTUM(invViewProj, color) DBG_DRAW.AddFrustumFromInvViewProj((invViewProj), (color))
#define DBG_DRAW_CYLINDER(base, normal, height, radius, segments, color)                                               \
    DBG_DRAW.AddCylinder((base), (normal), (height), (radius), (segments), (color))

// with lifetime
#define DBG_DRAW_LINE_TTL(a, b, color, ttl) DBG_DRAW.AddLine((a), (b), (color), DepthMode::DepthRead, (ttl))
#define DBG_DRAW_RAY_TTL(origin, dir, length, color, ttl)                                                              \
    DBG_DRAW.AddRay((origin), (dir), (length), (color), DepthMode::DepthRead, (ttl))
#define DBG_DRAW_AXIS_TTL(origin, size, ttl)    DBG_DRAW.AddAxis((origin), (size), DepthMode::DepthRead, (ttl))
#define DBG_DRAW_AABB_TTL(min, max, color, ttl) DBG_DRAW.AddAABB((min), (max), (color), DepthMode::DepthRead, (ttl))
#define DBG_DRAW_AABBCE_TTL(center, extents, color, ttl)                                                               \
    DBG_DRAW.AddAABBCenterExtents((center), (extents), (color), DepthMode::DepthRead, (ttl))
#define DBG_DRAW_GRID_TTL(halfSize, divisions, origin, normal, color, ttl)                                             \
    DBG_DRAW.AddGrid((halfSize), (divisions), (origin), (normal), (color), DepthMode::DepthRead, (ttl))
#define DBG_DRAW_GRIDXZ_TTL(halfSize, divisions, y, color, ttl)                                                        \
    DBG_DRAW.AddGridXZ((halfSize), (divisions), (y), (color), DepthMode::DepthRead, (ttl))
#define DBG_DRAW_CIRCLE_TTL(center, normal, radius, segments, color, ttl)                                              \
    DBG_DRAW.AddCircle((center), (normal), (radius), (segments), (color), DepthMode::DepthRead, (ttl))
#define DBG_DRAW_ARC_TTL(center, normal, startDir, radius, angle, segments, color, ttl)                                \
    DBG_DRAW.AddArc((center), (normal), (startDir), (radius), (angle), (segments), (color), DepthMode::DepthRead, (ttl))
#define DBG_DRAW_SPHERE_TTL(center, radius, segments, color, ttl)                                                      \
    DBG_DRAW.AddSphere((center), (radius), (segments), (color), DepthMode::DepthRead, (ttl))
#define DBG_DRAW_CONE_TTL(apex, dir, angle, length, segments, color, ttl)                                              \
    DBG_DRAW.AddCone((apex), (dir), (angle), (length), (segments), (color), DepthMode::DepthRead, (ttl))
#define DBG_DRAW_CAPSULE_TTL(pointA, pointB, radius, segments, color, ttl)                                             \
    DBG_DRAW.AddCapsule((pointA), (pointB), (radius), (segments), (color), DepthMode::DepthRead, (ttl))
#define DBG_DRAW_FRUSTUM_TTL(invViewProj, color, ttl)                                                                  \
    DBG_DRAW.AddFrustumFromInvViewProj((invViewProj), (color), DepthMode::DepthRead, (ttl))
#define DBG_DRAW_CYLINDER_TTL(base, normal, height, radius, segments, color, ttl)                                      \
    DBG_DRAW.AddCylinder((base), (normal), (height), (radius), (segments), (color), DepthMode::DepthRead, (ttl))
