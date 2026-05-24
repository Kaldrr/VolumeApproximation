#pragma once

#include <ApproximationLib/IVolumeApproximation.hpp>
#include <ApproximationLib/Vector3.hpp>

#ifdef __CUDACC__
#define FUNCTION_ATTRIBUTES __host__ __device__
#else
#define FUNCTION_ATTRIBUTES
#endif


// TODO: This needs to be in header file for now
// CMake compiles files by extension, .cpp -> normal C++
// .cu -> forwards to NVCC/CUDA compiler
// If it was in .cu we would need a CUDA compiler even with CUDA disabled
// The workaround would be some CMake magic to forward to correct compiler, based on CUDA no matter the extension
namespace VolumeApproximation
{
///
/// @param rayOrigin Point from which we are shooting the ray. This is the point we
/// want to check if it's inside the object or not.
/// @param rayVector Ray shot from outer point, to a point that may be inside the
/// object.
/// @param triangle Triangle we are checking intersection against.
/// @return true if the ray intersects the triangle.
[[nodiscard]] FUNCTION_ATTRIBUTES inline bool RayIntersection(
    const Vector3F& rayOrigin,
    const Vector3F& rayVector,
    const Triangle& triangle) noexcept
{
	constexpr float Epsilon = 0.0000001F;

	const Vector3F edge1 = triangle[1] - triangle[0];
	const Vector3F edge2 = triangle[2] - triangle[0];
	const Vector3F h     = Vector3F::CrossProduct(rayVector, edge2);
	const float a        = Vector3F::DotProduct(edge1, h);

	if (-Epsilon < a && a < Epsilon)
	{
		return false;
	}

	const float f    = 1.0F / a;
	const Vector3F s = rayOrigin - triangle[0];
	const float u    = f * Vector3F::DotProduct(s, h);
	if (u < 0.0F || u > 1.0F)
	{
		return false;
	}

	const Vector3F q = Vector3F::CrossProduct(s, edge1);
	const float v    = f * Vector3F::DotProduct(rayVector, q);
	if (0.0F > v || (u + v) > 1.0F)
	{
		return false;
	}

	const float t = f * Vector3F::DotProduct(edge2, q);
	return t > Epsilon;
}
} // namespace VolumeApproximation