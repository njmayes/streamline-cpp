#include "Method.h"
#include "Type.h"

namespace slc {

	Type Method::GetReturnType() const
	{
		return Type( mMethod->return_type );
	}

	std::vector< Type > Method::GetArgumentTypes() const
	{
		return mMethod->arguments |
			   std::views::transform( []( const auto& arg ) { return Type( arg ); } ) |
			   std::ranges::to< std::vector >();
	}
} // namespace slc