#include "Engine2D.h"

#include "RenderSprite.h"

using namespace DirectX;

Engine2D::Engine2D(const DX::RenderEngine& engine) { Initialize(engine); }

void Engine2D::Initialize(const DX::RenderEngine& engine)
{
    m_SpriteBatch = std::make_unique<SpriteBatch>(engine.GetContext());

    CreateSpriteFont(engine.GetDevice(), kDefaultSpriteFontPath);

    auto &viewport    = engine.GetViewport();
    m_ViewportWidth  = static_cast<uint32_t>(viewport.Width);
    m_ViewportHeight = static_cast<uint32_t>(viewport.Height);
}

void Engine2D::BeginBatch() const { m_SpriteBatch->Begin(SpriteSortMode_FrontToBack); }

void Engine2D::DrawSprites(const std::vector<RenderSprite>& sprites) const
{
    for (const auto& sprite : sprites)
    {
        if (!sprite.visible)
            continue;

        if (!sprite.texture)
            continue;

        RECT src = ToWinRect(sprite.srcRect);
        RECT dst = ToWinRect(sprite.dstRect);

        XMVECTOR tint{ sprite.tint.x, sprite.tint.y, sprite.tint.z, sprite.tint.w };

        XMFLOAT2 origin = {0, 0};

        m_SpriteBatch->Draw(
            sprite.texture.Get(),
            dst,
            &src,
            tint,
            sprite.transform.rotation,
            origin,
            SpriteEffects_None,
            sprite.layer);
    }
}

void Engine2D::DrawSprites(const std::vector<Sprite*>& sprites) const
{
    //for (const auto& s : sprites)
    //{
    //    if (!s->visible)
    //        continue;

    //    ID3D11ShaderResourceView* srv = nullptr;
    //    if (const auto& tex = context.resolveTexture(s->textureId))
    //        srv = tex->GetSRV();

    //    if (!srv)
    //        continue;

    //    RECT src = ToWinRect(s->srcRect);
    //    RECT dst = ToWinRect(s->dstRect);

    //    XMVECTOR tint{ s->tint.x, s->tint.y, s->tint.z, s->tint.w };

    //    XMFLOAT2 origin = { 0, 0 };

    //    m_SpriteBatch->Draw(srv, dst, &src, tint, s->transform.rotation, origin, SpriteEffects_None, s->layer);
    //}
}

void Engine2D::DrawSpriteTexts(const std::vector<SpriteText*>& spriteText) const
{
    for (const auto& st : spriteText)
        DrawSpriteText(*st);
}

void Engine2D::DrawSpriteText(const SpriteText& spriteText) const
{
    // get the centre of the text rather than upper left
    if (!spriteText.Visible || !m_SpriteFont)
        return;

    auto color = Colors::White;

    XMVECTOR origin = XMVectorScale(m_SpriteFont->MeasureString(spriteText.Text.c_str()), 0.5f);

    m_SpriteFont->DrawString(
        m_SpriteBatch.get(),
        spriteText.Text.c_str(),
        spriteText.Position,
        color,
        spriteText.Rotation,
        origin,
        spriteText.Size,
        DirectX::SpriteEffects_None,
        spriteText.Layer);
}

void Engine2D::DrawSpriteTexts(const std::vector<SpriteText>& spriteText) const
{
    for (const auto& st : spriteText)
        DrawSpriteText(st);
}

/// @brief Stops sprite batch which then renders all sprites then
/// restores the pipeline state to before 2D rendering started
void Engine2D::EndBatch() const { m_SpriteBatch->End(); }

void Engine2D::SetViewportSize(uint32_t width, uint32_t height)
{
    m_ViewportWidth  = width;
    m_ViewportHeight = height;
}
