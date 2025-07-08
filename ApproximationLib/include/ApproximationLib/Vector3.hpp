#pragma once

#include <ApproximationLib/ApproximationLib_global.hpp>

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
class [[nodiscard]] Vector3
{
public:
	VECTOR_ATTRIBUTES constexpr Vector3() noexcept;
	VECTOR_ATTRIBUTES constexpr Vector3(T x, T y, T z) noexcept;

	VECTOR_ATTRIBUTES constexpr float& operator[](std::size_t i) noexcept;
	VECTOR_ATTRIBUTES constexpr float operator[](std::size_t i) const noexcept;

	VECTOR_ATTRIBUTES [[nodiscard]] static constexpr float DotProduct(const Vector3& v1,
	    const Vector3& v2) noexcept;
	VECTOR_ATTRIBUTES [[nodiscard]] static constexpr Vector3 CrossProduct(const Vector3& v1,
	    const Vector3& v2) noexcept;
	VECTOR_ATTRIBUTES [[nodiscard]] constexpr Vector3 Normalized() const noexcept;
	VECTOR_ATTRIBUTES [[nodiscard]] constexpr float Length() const noexcept;

	VECTOR_ATTRIBUTES [[nodiscard]] constexpr float X() const noexcept;
	VECTOR_ATTRIBUTES [[nodiscard]] constexpr float Y() const noexcept;
	VECTOR_ATTRIBUTES [[nodiscard]] constexpr float Z() const noexcept;

	VECTOR_ATTRIBUTES constexpr friend Vector3 operator+(const Vector3& v1,
	    const Vector3& v2) noexcept
	{
		return Vector3(v1.m_Values[0] + v2.m_Values[0], v1.m_Values[1] + v2.m_Values[1],
		    v1.m_Values[2] + v2.m_Values[2]);
	}

	VECTOR_ATTRIBUTES constexpr friend Vector3 operator-(const Vector3& v1,
	    const Vector3& v2) noexcept
	{
		return Vector3(v1.m_Values[0] - v2.m_Values[0], v1.m_Values[1] - v2.m_Values[1],
		    v1.m_Values[2] - v2.m_Values[2]);
	}

	VECTOR_ATTRIBUTES constexpr friend Vector3 operator*(const float factor,
	    const Vector3& vector) noexcept
	{
		return Vector3(vector.m_Values[0] * factor, vector.m_Values[1] * factor,
		    vector.m_Values[2] * factor);
	}

	VECTOR_ATTRIBUTES constexpr friend Vector3 operator*(const Vector3& vector,
	    const float factor) noexcept
	{
		return Vector3(vector.m_Values[0] * factor, vector.m_Values[1] * factor,
		    vector.m_Values[2] * factor);
	}

	VECTOR_ATTRIBUTES constexpr friend Vector3 operator*(const Vector3& v1,
	    const Vector3& v2) noexcept
	{
		return Vector3(v1.m_Values[0] * v2.m_Values[0], v1.m_Values[1] * v2.m_Values[1],
		    v1.m_Values[2] * v2.m_Values[2]);
	}

	VECTOR_ATTRIBUTES constexpr friend Vector3 operator-(const Vector3& vector) noexcept
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
VECTOR_ATTRIBUTES constexpr Vector3<T>::Vector3() noexcept : m_Values{}
{
}

template <typename T>
VECTOR_ATTRIBUTES constexpr Vector3<T>::Vector3(T x, T y, T z) noexcept
    : m_Values{std::move(x), std::move(y), std::move(z)}
{
}

template <typename T>
VECTOR_ATTRIBUTES constexpr float& Vector3<T>::operator[](const std::size_t i) noexcept
{
	return m_Values[i];
}

template <typename T>
VECTOR_ATTRIBUTES constexpr float Vector3<T>::operator[](const std::size_t i) const noexcept
{
	return m_Values[i];
}

template <typename T>
VECTOR_ATTRIBUTES constexpr float Vector3<T>::DotProduct(const Vector3& v1,
    const Vector3& v2) noexcept
{
	return v1.m_Values[0] * v2.m_Values[0] + v1.m_Values[1] * v2.m_Values[1] +
	       v1.m_Values[2] * v2.m_Values[2];
}

template <typename T>
VECTOR_ATTRIBUTES constexpr Vector3<T> Vector3<T>::CrossProduct(const Vector3& v1,
    const Vector3& v2) noexcept
{
	return Vector3(v1.m_Values[1] * v2.m_Values[2] - v1.m_Values[2] * v2.m_Values[1],
	    v1.m_Values[2] * v2.m_Values[0] - v1.m_Values[0] * v2.m_Values[2],
	    v1.m_Values[0] * v2.m_Values[1] - v1.m_Values[1] * v2.m_Values[0]);
}

template <typename T>
VECTOR_ATTRIBUTES constexpr Vector3<T> Vector3<T>::Normalized() const noexcept
{
	const float len = Length();
	return FuzzyIsZero(len - 1.0f) ? *this
	       : FuzzyIsZero(len)      ? Vector3()
	                          : Vector3(m_Values[0] / len, m_Values[1] / len, m_Values[2] / len);
}

template <typename T>
VECTOR_ATTRIBUTES constexpr float Vector3<T>::Length() const noexcept
{
	return sqrt(m_Values[0] * m_Values[0] + m_Values[1] * m_Values[1] + m_Values[2] * m_Values[2]);
}

template <typename T>
VECTOR_ATTRIBUTES constexpr float Vector3<T>::X() const noexcept
{
	return m_Values[0];
}

template <typename T>
VECTOR_ATTRIBUTES constexpr float Vector3<T>::Y() const noexcept
{
	return m_Values[1];
}

template <typename T>
VECTOR_ATTRIBUTES constexpr float Vector3<T>::Z() const noexcept
{
	return m_Values[2];
}

template <typename T>
VECTOR_ATTRIBUTES constexpr bool Vector3<T>::FuzzyIsZero(const float f) noexcept
{
	return std::abs(f) <= 0.00001f;
}

} // namespace VolumeApproximation

#ifdef _MSC_VER
#pragma warning(pop)
#endif
