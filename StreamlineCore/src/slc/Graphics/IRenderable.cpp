#include "IRenderable.h"

#include <glad/glad.h>

namespace slc {

	void IRenderable::BindTexture( uint32_t slot ) const
	{
		glBindTextureUnit( slot, GetTextureID() );
	};
} // namespace slc