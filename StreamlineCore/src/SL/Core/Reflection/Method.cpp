#include "Method.h"
#include "Type.h"

namespace sl {

	Type Method::GetReturnType() const
	{
		if ( not mMethod->return_type.has_value() )
			return Type( nullptr ); // void

		return Type( mMethod->return_type->base );
	}

	std::vector< Type > Method::GetArgumentTypes() const
	{
		return mMethod->arguments |
			   std::views::transform( []( const auto& arg ) { return Type( arg.base ); } ) |
			   std::ranges::to< std::vector >();
	}
} // namespace sl