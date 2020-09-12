#pragma once

#include "pch.h"

namespace engine
{
	class vector3
	{
	public:
		float x, y, z;

		vector3() : x(0), y(0), z(0) {}
		vector3(float xp, float yp, float zp) : x(xp), y(yp), z(zp) {}

		inline vector3 operator+(vector3& v) { return vector3(this->x + v.x, this->y + v.y, this->z + v.z); }
		inline vector3 operator-(vector3& v) { return vector3(this->x - v.x, this->y - v.y, this->z - v.z); }
		inline vector3 operator*(vector3& v) { return vector3(this->x * v.x, this->y * v.y, this->z * v.z); }
		inline vector3 operator/(vector3& v) { return vector3(this->x / v.x, this->y / v.y, this->z / v.z); }

		inline vector3 operator+(float f) { return vector3(this->x + f, this->y + f, this->z + f); }
		inline vector3 operator-(float f) { return vector3(this->x - f, this->y - f, this->z - f); }
		inline vector3 operator*(float f) { return vector3(this->x * f, this->y * f, this->z * f); }
		inline vector3 operator/(float f) { return vector3(this->x / f, this->y / f, this->z / f); }

		inline void normalise(vector3& v);

		inline vector3 cross(vector3& v1, vector3& v2);
	};
}