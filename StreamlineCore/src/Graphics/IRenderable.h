#pragma once

#include <Common/Base.h>
#include <Types/Math.h>

namespace slc {

	class IRenderable : public RefCounted
	{
	public:
		virtual uint32_t GetTextureID() const = 0;
		void BindTexture(uint32_t slot = 0) const;
		virtual const Vector2* GetTextureCoords() const { return DefaultCoords; }

		bool operator==(const IRenderable& other) const { return GetTextureID() == other.GetTextureID(); }

	private:
		static constexpr Vector2 DefaultCoords[4] = { {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f} };
	};
}