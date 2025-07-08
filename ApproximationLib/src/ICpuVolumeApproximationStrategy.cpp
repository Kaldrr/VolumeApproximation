#include <ApproximationLib/CpuParallelVolumeApproximation.hpp>

namespace VolumeApproximation::Impl
{
ApproximationPoint ICpuVolumeApproximation::ComputePoint(
    const Vector3F& origin,
    const Vector3F& pointPosition,
    const std::span<const Triangle> meshTriangles) noexcept
{
	constexpr float Epsilon = 0.0000001F;

	const Vector3F rayVector    = (origin - pointPosition).Normalized();
	std::size_t trianglesPassed = 0;

	for (const Triangle& triangle : meshTriangles)
	{
		const Vector3F edge1 = triangle[1] - triangle[0];
		const Vector3F edge2 = triangle[2] - triangle[0];
		const Vector3F h     = Vector3F::CrossProduct(rayVector, edge2);
		const float a        = Vector3F::DotProduct(edge1, h);

		if (-Epsilon < a && a < Epsilon)
		{
			continue;
		}

		const float f    = 1.0f / a;
		const Vector3F s = origin - triangle[0];
		const float u    = f * Vector3F::DotProduct(s, h);
		if (u < 0.0f || u > 1.0f)
		{
			continue;
		}

		const Vector3F q = Vector3F::CrossProduct(s, edge1);
		const float v    = f * Vector3F::DotProduct(rayVector, q);
		if (0.0F > v || (u + v) > 1.0F)
		{
			continue;
		}

		const float t = f * Vector3F::DotProduct(edge2, q);
		if (t > Epsilon)
		{
			++trianglesPassed;
		}
	}

	return ApproximationPoint{ .Point  = pointPosition,
		                       .Status = trianglesPassed % 2 == 0
		                                     ? PointStatus::Outside
		                                     : PointStatus::Inside };
}
} // namespace VolumeApproximation::Impl