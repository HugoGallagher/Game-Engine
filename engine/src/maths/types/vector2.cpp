#include "pch.h"
#include "vector2.h"

namespace engine
{
	void vector2::normalise(vector2& v)
	{
		float mag = sqrtf(v.x * v.x + v.y * v.y);
		v.x /= mag;
		v.y /= mag;
	}

	float vector2::magnitude()
	{
		return sqrtf(this->x * this->x + this->y * this->y);
	}

	float vector2::dot(vector2& v)
	{
		return this->x * v.x + this->y * v.y;
	}

	float vector2::angle(vector2& v)
	{
		float ca = atan(this->y / this->x);
		float da = atan(v.y / v.x);

		return da - ca;
	}

	std::ostream& operator<<(std::ostream& s, const vector2& v) { return s << v.x << ", " << v.y; }
}