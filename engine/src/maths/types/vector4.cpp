#include "pch.h"

#include "vector4.h"

namespace engine
{
	std::ostream& operator<<(std::ostream& s, const vector4& v) { return s << v.x << ", " << v.y << ", " << v.z << ", " << v.w; }
}