#include "Method.h"
#include "Type.h"

namespace slc {

	Type Method::GetReturnType() const
	{ 
		return Type(mMethod->return_type); 
	}
}