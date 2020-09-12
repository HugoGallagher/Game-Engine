#pragma once

#include "pch.h"

namespace engine
{
	class vector4
	{
	public:
		float x, y, z, w;

		vector4() : x(0), y(0), z(0), w(0) {}
		vector4(float xp, float yp, float zp, float wp) : x(xp), y(yp), z(zp), w(wp) {}

		inline vector4 operator+(vector4& v) { return vector4(this->x + v.x, this->y + v.y, this->z + v.z, this->w + v.w); }
		inline vector4 operator-(vector4& v) { return vector4(this->x - v.x, this->y - v.y, this->z - v.z, this->w - v.w); }
		inline vector4 operator*(vector4& v) { return vector4(this->x * v.x, this->y * v.y, this->z * v.z, this->w * v.w); }
		inline vector4 operator/(vector4& v) { return vector4(this->x / v.x, this->y / v.y, this->z / v.z, this->w / v.w); }

		inline vector4 operator+(float f) { return vector4(this->x + f, this->y + f, this->z + f, this->w + f); }
		inline vector4 operator-(float f) { return vector4(this->x - f, this->y - f, this->z - f, this->w - f); }
		inline vector4 operator*(float f) { return vector4(this->x * f, this->y * f, this->z * f, this->w * f); }
		inline vector4 operator/(float f) { return vector4(this->x / f, this->y / f, this->z / f, this->w / f); }
	};
}