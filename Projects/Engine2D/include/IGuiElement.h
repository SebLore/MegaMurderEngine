#pragma once


class IGuiElement
{
public:
	virtual ~IGuiElement() noexcept = default;
	virtual void Draw() = 0;

	virtual size_t GetSpriteCount() const = 0;

private:

};
