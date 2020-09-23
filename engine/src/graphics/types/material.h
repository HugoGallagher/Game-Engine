#pragma once

#include "maths/types/vector3.h"

namespace engine
{
	struct material
	{
		vector3 col;

		material() {}
		material(vector3 c) : col(c) {}
	};
}