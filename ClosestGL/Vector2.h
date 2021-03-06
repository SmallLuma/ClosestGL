#pragma once
#include "Number.h"

namespace ClosestGL::Math
{
	/* 2维向量 */
	template<typename T>
	struct Vector2
	{
		T x, y;

		using BaseType = T;

		/* 求模 */
		constexpr T Length() const
		{
			return static_cast<T>(Sqrt(x * x + y * y));
		}
	};

	template<typename T>
	constexpr Vector2<T> operator + (const Vector2<T>& a, const Vector2<T>& b)
	{
		return Vector2<T>
		{
			a.x + b.x,
			a.y + b.y
		};
	}

	template<typename T>
	constexpr Vector2<T> operator - (const Vector2<T>& a, const Vector2<T>& b)
	{
		return Vector2<T>
		{
			a.x - b.x,
			a.y - b.y
		};
	}

	template<typename T>
	constexpr Vector2<T> operator * (const Vector2<T>& a, const T& b)
	{
		return Vector2<T>
		{
			a.x * b,
			a.y * b
		};
	}

	template<typename T>
	constexpr Vector2<T> operator / (const Vector2<T>& a, const T& b)
	{
		return Vector2<T>
		{
			a.x / b,
			a.y / b
		};
	}

	template<typename T>
	constexpr bool operator == (const Vector2<T>& a, const Vector2<T>& b)
	{
		return a.x == b.x && a.y == b.y;
	}

	/* 求2维向量点积 */
	template<typename T>
	constexpr T Dot(const Vector2<T>& a, const Vector2<T>& b)
	{
		return a.x * b.x + a.y * b.y;
	}
}