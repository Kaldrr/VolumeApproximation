#pragma once

#include <ApproximationLibInterface/ApproximationLibInterface_global.h>

#include <array>
#include <cmath>
#include <type_traits>

#if defined(__CUDACC__)
#define VECTOR_ATTRIBUTES __host__ __device__
#else
#define VECTOR_ATTRIBUTES
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

namespace VolumeApproximation
{

template <typename T>
// requires std::is_same<T, float> || std::is_same<T, double>
class Vector3
{
public:
	VECTOR_ATTRIBUTES constexpr Vector3() noexcept;
	VECTOR_ATTRIBUTES constexpr Vector3(T x, T y, T z) noexcept;

	VECTOR_ATTRIBUTES constexpr float& operator[](int i) noexcept;
	VECTOR_ATTRIBUTES constexpr float operator[](int i) const noexcept;

	VECTOR_ATTRIBUTES [[nodiscard]] static constexpr float dotProduct(Vector3 v1,
	    Vector3 v2) noexcept;
	VECTOR_ATTRIBUTES [[nodiscard]] static constexpr Vector3 crossProduct(Vector3 v1,
	    Vector3 v2) noexcept;
	VECTOR_ATTRIBUTES [[nodiscard]] constexpr Vector3 normalized() const noexcept;
	VECTOR_ATTRIBUTES [[nodiscard]] constexpr float length() const noexcept;

	VECTOR_ATTRIBUTES constexpr float x() const noexcept;
	VECTOR_ATTRIBUTES constexpr float y() const noexcept;
	VECTOR_ATTRIBUTES constexpr float z() const noexcept;

	VECTOR_ATTRIBUTES constexpr friend inline Vector3 operator+(Vector3 v1, Vector3 v2) noexcept
	{
		return Vector3(v1.m_Values[0] + v2.m_Values[0], v1.m_Values[1] + v2.m_Values[1],
		    v1.m_Values[2] + v2.m_Values[2]);
	}

	VECTOR_ATTRIBUTES constexpr friend inline Vector3 operator-(Vector3 v1, Vector3 v2) noexcept
	{
		return Vector3(v1.m_Values[0] - v2.m_Values[0], v1.m_Values[1] - v2.m_Values[1],
		    v1.m_Values[2] - v2.m_Values[2]);
	}

	VECTOR_ATTRIBUTES constexpr friend inline Vector3 operator*(float factor,
	    Vector3 vector) noexcept
	{
		return Vector3(vector.m_Values[0] * factor, vector.m_Values[1] * factor,
		    vector.m_Values[2] * factor);
	}

	VECTOR_ATTRIBUTES constexpr friend inline Vector3 operator*(Vector3 vector,
	    float factor) noexcept
	{
		return Vector3(vector.m_Values[0] * factor, vector.m_Values[1] * factor,
		    vector.m_Values[2] * factor);
	}

	VECTOR_ATTRIBUTES constexpr friend inline Vector3 operator*(Vector3 v1, Vector3 v2) noexcept
	{
		return Vector3(v1.m_Values[0] * v2.m_Values[0], v1.m_Values[1] * v2.m_Values[1],
		    v1.m_Values[2] * v2.m_Values[2]);
	}

	VECTOR_ATTRIBUTES constexpr friend inline Vector3 operator-(Vector3 vector) noexcept
	{
		return Vector3(-vector.m_Values[0], -vector.m_Values[1], -vector.m_Values[2]);
	}

private:
	VECTOR_ATTRIBUTES [[nodiscard]] constexpr static bool FuzzyIsZero(float f) noexcept;

private:
	std::array<T, 3> m_Values;
};

using Vector3F = Vector3<float>;
using Vector3D = Vector3<double>;

template <typename T>
VECTOR_ATTRIBUTES inline constexpr Vector3<T>::Vector3() noexcept : m_Values{}
{
}

template <typename T>
VECTOR_ATTRIBUTES inline constexpr Vector3<T>::Vector3(T x, T y, T z) noexcept : m_Values{x, y, z}
{
}

template <typename T>
VECTOR_ATTRIBUTES inline constexpr float& Vector3<T>::operator[](int i) noexcept
{
	return m_Values[i];
}

template <typename T>
VECTOR_ATTRIBUTES inline constexpr float Vector3<T>::operator[](int i) const noexcept
{
	return m_Values[i];
}

template <typename T>
VECTOR_ATTRIBUTES inline constexpr float Vector3<T>::dotProduct(Vector3 v1, Vector3 v2) noexcept
{
	return v1.m_Values[0] * v2.m_Values[0] + v1.m_Values[1] * v2.m_Values[1] +
	       v1.m_Values[2] * v2.m_Values[2];
}

template <typename T>
VECTOR_ATTRIBUTES inline constexpr Vector3<T> Vector3<T>::crossProduct(Vector3 v1,
    Vector3 v2) noexcept
{
	return Vector3(v1.m_Values[1] * v2.m_Values[2] - v1.m_Values[2] * v2.m_Values[1],
	    v1.m_Values[2] * v2.m_Values[0] - v1.m_Values[0] * v2.m_Values[2],
	    v1.m_Values[0] * v2.m_Values[1] - v1.m_Values[1] * v2.m_Values[0]);
}

template <typename T>
VECTOR_ATTRIBUTES inline constexpr Vector3<T> Vector3<T>::normalized() const noexcept
{
	const float len = length();
	return FuzzyIsZero(len - 1.0f) ? *this
	       : FuzzyIsZero(len)      ? Vector3()
	                          : Vector3(m_Values[0] / len, m_Values[1] / len, m_Values[2] / len);
}

template <typename T>
VECTOR_ATTRIBUTES inline constexpr float Vector3<T>::length() const noexcept
{
	return sqrt(m_Values[0] * m_Values[0] + m_Values[1] * m_Values[1] + m_Values[2] * m_Values[2]);
}

template <typename T>
VECTOR_ATTRIBUTES inline constexpr float Vector3<T>::x() const noexcept
{
	return m_Values[0];
}

template <typename T>
VECTOR_ATTRIBUTES inline constexpr float Vector3<T>::y() const noexcept
{
	return m_Values[1];
}

template <typename T>
VECTOR_ATTRIBUTES inline constexpr float Vector3<T>::z() const noexcept
{
	return m_Values[2];
}

template <typename T>
VECTOR_ATTRIBUTES inline constexpr bool Vector3<T>::FuzzyIsZero(float f) noexcept
{
	return std::abs(f) <= 0.00001f;
}

} // namespace VolumeApproximation

#ifdef _MSC_VER
#pragma warning(pop)
#endif
