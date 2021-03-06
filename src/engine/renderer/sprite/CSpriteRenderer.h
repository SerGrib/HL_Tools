#ifndef ENGINE_SHARED_SPRITE_CSPRITERENDERER_H
#define ENGINE_SHARED_SPRITE_CSPRITERENDERER_H

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "engine/shared/renderer/DrawConstants.h"

#include "engine/shared/renderer/sprite/ISpriteRenderer.h"

namespace sprite
{
struct msprite_t;

class CSpriteRenderer final : public ISpriteRenderer
{
private:
	static const float DEFAULT_FRAMERATE;

public:
	CSpriteRenderer();
	~CSpriteRenderer();

	void DrawSprite( const CSpriteRenderInfo* pRenderInfo, const renderer::DrawFlags_t flags ) override;

	void DrawSprite2D( const float flX, const float flY, const float flWidth, const float flHeight, const msprite_t* pSprite, const renderer::DrawFlags_t flags = renderer::DrawFlag::NONE ) override;

	void DrawSprite2D( const float flX, const float flY, const msprite_t* pSprite, const float flScale = 1, const renderer::DrawFlags_t flags = renderer::DrawFlag::NONE ) override;

	void DrawSprite2D( const C2DSpriteRenderInfo* pRenderInfo, const renderer::DrawFlags_t flags = renderer::DrawFlag::NONE ) override;

private:

	void DrawSprite( const glm::vec3& vecOrigin, const glm::vec2& vecSize, 
					 const msprite_t* pSprite, const float flFrame, 
					 const renderer::DrawFlags_t flags, const sprite::Type::Type* pTypeOverride = nullptr, const sprite::TexFormat::TexFormat* pTexFormatOverride = nullptr );

private:
	CSpriteRenderer( const CSpriteRenderer& ) = delete;
	CSpriteRenderer& operator=( const CSpriteRenderer& ) = delete;
};
}

#endif //ENGINE_SHARED_SPRITE_CSPRITERENDERER_H