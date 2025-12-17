#include "IRenderable.h"

#include <glad/glad.h>

namespace sl {

	void IRenderable::BindTexture( uint32_t slot ) const
	{
		glBindTextureUnit( slot, GetTextureID() );
	};
} // namespace sl