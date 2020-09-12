#include "pch.h"
#include "vector3.h"

namespace engine
{
	void vector3::normalise(vector3& v)
	{
		float mag = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
		v.x /= mag;
		v.y /= mag;
		v.z /= mag;
	}

	vector3 vector3::cross(vector3& v1, vector3& v2) // Eventually use SIMD
	{
		float tx = v1.y * v2.z - v1.z * v2.y;
		float ty = v1.z * v2.x - v1.x * v2.z;
		float tz = v1.x * v2.y - v1.y * v2.x;

		return vector3(tx, ty, tz);
	}

	std::ostream& operator<<(std::ostream& s, const vector3& v) { return s << v.x << ", " << v.y << ", " << v.z; }
}