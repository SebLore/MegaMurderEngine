/**
 * @file DebugRenderer.h
 *
 * @author Sebastian L
 *
 * Wrapper for DebugDrawQueue and DebugPrimitiveBatch.
 *
 * Put inside an Engine, call BeginFrame at the start each loop and end frame at the end, Flush whenever geometry should be drawn.
 * Only end frame does any drawing, so begin frame can be called outside the main engine's begin/end block. Call in the engine shutdown/destructor to free resources.
 * Flush needs a view and projection XMMATRIX, makes sense to use the active camera's.
 *
 * Usage:
 * [Start of frame] -> DebugRenderer.BeginFrame(dt);
 * [Draw collected geometry] -> DebugRenderer.Flush(viewMatrix, projMatrix);
 *  [End of frame]   -> DebugRenderer.EndFrame();
 *
 *
 */
#pragma once
#include "DebugDrawQueue.h"
#include "DebugLineRenderer.h"

class DebugRenderer
{
public:
    DebugRenderer(ID3D11Device* device, ID3D11DeviceContext* context, size_t reserveLines = 8192)
    {
        Initialize(device, context, reserveLines);
    }
    /// Destructor, calls UnbindGlobal internally for safety
    ~DebugRenderer()
    {
        // Safe to do even with shutdown called
        DebugDrawQueue::UnbindGlobal(&m_DrawQueue);
    }

    /**
     * @brief Initializes the system
     * @param device valid device
     * @param context valid device context
     * @param reserveLines maximum amount of lines stored in a single batch, calling Flush frees up stored lines
     */
    void Initialize(ID3D11Device* device, ID3D11DeviceContext* context, size_t reserveLines = 8192)
    {
        m_BatchRenderer.Initialize(device, context);
        m_DrawQueue.Initialize(reserveLines);

        // optional: expose global access to *the draw list*
        DebugDrawQueue::BindGlobal(&m_DrawQueue);
    }

    /// Unbind internal DebugDrawQueue from global access, safe to call multiple times or even if already unbound
    void Shutdown() { DebugDrawQueue::UnbindGlobal(&m_DrawQueue); }

    /// Clears per-frame lists and ages persistent ones. Call once at start of each frame
    void BeginFrame(float dt) { m_DrawQueue.BeginFrame(dt); }

    /// Stops recording draw calls until BeginFrame is called. Call once at end of each frame.
    void EndFrame() { m_DrawQueue.EndFrame(); }

    /// Draws all recorded geometry with view and proj matrix
    void Flush(DirectX::CXMMATRIX view, DirectX::CXMMATRIX proj) { m_DrawQueue.Flush(m_BatchRenderer, view, proj); }

    /// Flag to toggle depth testing when drawing batch, default is true (depth test on)
    void SetDepth(bool enabled) { m_BatchRenderer.SetEnableDepth(enabled); }

    // internal access for convenience or when the global DebugDrawQueue API isn't enough

    DebugDrawQueue& GetDraw() { return m_DrawQueue; }
    DebugLineRenderer& GetBatch() { return m_BatchRenderer; }

private:
    DebugLineRenderer m_BatchRenderer; ///< Handles the drawing
    DebugDrawQueue           m_DrawQueue; ///< Collects
};
