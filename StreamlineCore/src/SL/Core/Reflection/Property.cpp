#include "Property.h"
#include "Type.h"

namespace sl {

	Type Property::GetType() const
	{
		return mProperty->prop_type;
	}
} // namespace sl