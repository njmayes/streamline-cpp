#include "Property.h"
#include "Type.h"

namespace slc {

	Type Property::GetType() const
	{
		return mProperty->prop_type;
	}
}