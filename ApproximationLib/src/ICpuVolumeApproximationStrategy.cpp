#include <ApproximationLib/CpuParallelVolumeApproximation.hpp>

#include <ApproximationLib/PointCalculations.hpp>

namespace VolumeApproximation::Impl
{
ApproximationPoint ICpuVolumeApproximation::ComputePoint(
    const Vector3F& origin,
    const Vector3F& pointPosition,
    const std::span<const Triangle> meshTriangles) noexcept
{
	const Vector3F rayVector    = (origin - pointPosition).Normalized();
	std::size_t trianglesPassed = 0;

	for (const Triangle& triangle : meshTriangles)
	{
		if (RayIntersection(origin, rayVector, triangle))
		{
			++trianglesPassed;
		}
	}

	return ApproximationPoint{ .Point  = origin,
		                       .Status = trianglesPassed % 2 == 0
		                                     ? PointStatus::Outside
		                                     : PointStatus::Inside };
}
} // namespace VolumeApproximation::Impl