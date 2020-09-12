#pragma once

#include "pch.h"

namespace engine
{
	class vector2
	{
	public:
		float x, y;

		vector2() : x(0), y(0) {}
		vector2(float xp, float yp) : x(xp), y(yp) {}

		inline vector2 operator+(vector2& v) { return vector2(this->x + v.x, this->y + v.y); }
		inline vector2 operator-(vector2& v) { return vector2(this->x - v.x, this->y - v.y); }
		inline vector2 operator*(vector2& v) { return vector2(this->x * v.x, this->y * v.y); }
		inline vector2 operator/(vector2& v) { return vector2(this->x / v.x, this->y / v.y); }
		
		inline vector2 operator+(float f) { return vector2(this->x + f, this->y + f); }
		inline vector2 operator-(float f) { return vector2(this->x - f, this->y - f); }
		inline vector2 operator*(float f) { return vector2(this->x * f, this->y * f); }
		inline vector2 operator/(float f) { return vector2(this->x / f, this->y / f); }

		inline void normalise(vector2& v);

		inline float magnitude();
		inline float dot(vector2& v);
		inline float angle(vector2& v);
	};
}