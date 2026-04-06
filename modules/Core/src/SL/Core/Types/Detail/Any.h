#pragma once

#include "SL/Core/Common/Base.h"

namespace sl::detail {

	template < typename T >
	struct IsReferenceWrapper
	{
		static constexpr bool Value = false;
	};

	template < typename T >
	struct IsReferenceWrapper< std::reference_wrapper< T > >
	{
		static constexpr bool Value = true;
	};
} // namespace sl::detail
