#include <ApproximationLibCpu/ICpuVolumeApproximationStrategy.h>

namespace VolumeApproximation::Impl
{
ApproximationPoint ICpuVolumeApproximationStrategy::computePoint(const Vector3F& origin,
    const Vector3F& pointPosition,
    const std::vector<Triangle>& meshTriangles)
{
	constexpr float Epsilon = 0.0000001f;

	const Vector3F rayVector = (origin - pointPosition).normalized();
	std::size_t trianglesPassed = 0;

	for (const Triangle& triangle : meshTriangles)
	{
		const Vector3F edge1 = triangle[1] - triangle[0];
		const Vector3F edge2 = triangle[2] - triangle[0];
		const Vector3F h = Vector3F::crossProduct(rayVector, edge2);
		const float a = Vector3F::dotProduct(edge1, h);

		if (-Epsilon < a && a < Epsilon)
		{
			continue;
		}

		const float f = 1.0f / a;
		const Vector3F s = origin - triangle[0];
		const float u = f * Vector3F::dotProduct(s, h);
		if (u < 0.0f || u > 1.0f)
		{
			continue;
		}

		const Vector3F q = Vector3F::crossProduct(s, edge1);
		const float v = f * Vector3F::dotProduct(rayVector, q);
		if (0.0f > v || (u + v) > 1.0f)
		{
			continue;
		}

		const float t = f * Vector3F::dotProduct(edge2, q);
		if (t > Epsilon)
		{
			++trianglesPassed;
		}
	}

	return {.m_point = pointPosition,
	    .m_status = trianglesPassed % 2 == 0 ? PointStatus::Outside : PointStatus::Inside};
}
} // namespace VolumeApproximation::Impl