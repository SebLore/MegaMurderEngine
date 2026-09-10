
#pragma once

#include <cstdint>
#include <memory>

#include <RenderEngine/RenderEngine.hpp>

#include <SpriteBatch.h>
#include <SpriteFont.h>

#include <Common/Scene/Sprite.h>

#include "RenderSprite.h"
#include "SpriteText.h"

using Murder::Sprite;

class Engine2D
{
  public:
    Engine2D()                      = default;
    ~Engine2D()                     = default;
    Engine2D(Engine2D&&)            = default;
    Engine2D& operator=(Engine2D&&) = default;

    Engine2D(const DX::RenderEngine& engine);

    void Initialize(const DX::RenderEngine& engine);

    /// Drawing
    void BeginBatch() const;

    void DrawSprites(const std::vector<RenderSprite>& sprites) const;
    void DrawSprites(const std::vector<Sprite*>& sprites) const;
    void DrawSpriteTexts(const std::vector<SpriteText*>& spriteText) const;

    void DrawSpriteText(
        const SpriteText& spriteText) const; ///< cant be name DrawText since that is already a function in window
    void DrawSpriteTexts(const std::vector<SpriteText>& spriteText) const;
    void EndBatch() const;

    DirectX::SpriteBatch& GetSpriteBatch() const { return *m_SpriteBatch.get(); }
    DirectX::SpriteFont&  GetSpriteFont() const { return *m_SpriteFont.get(); }

    /// Setting viewport info
    void SetViewportSize(uint32_t width, uint32_t height);

    uint32_t GetViewportWidth() const noexcept { return m_ViewportWidth; }
    uint32_t GetViewportHeight() const noexcept { return m_ViewportHeight; }

    void CreateSpriteFont(ID3D11Device* device, const wchar_t* path)
    {
        m_SpriteFont = std::make_unique<DirectX::SpriteFont>(device, path);
    }

  private:
    static ::RECT ToWinRect(const Murder::RectI& rect) { return { rect.left, rect.top, rect.right, rect.bottom }; }

  private:
    /// Holding the SpriteBatch
    std::unique_ptr<DirectX::SpriteBatch> m_SpriteBatch;

    const wchar_t*                       kDefaultSpriteFontPath = L"../assets/sprites/spaceGrotesk.spritefont";
    std::unique_ptr<DirectX::SpriteFont> m_SpriteFont;

    // Stores the context for updating of pipeline snapshots
    uint32_t m_ViewportWidth  = 0;
    uint32_t m_ViewportHeight = 0;
};
