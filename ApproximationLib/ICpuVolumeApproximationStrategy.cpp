#include <ApproximationLib/ICpuVolumeApproximationStrategy.h>

ApproximationPoint ICpuVolumeApproximationStrategy::computePoint(const QVector3D& origin,
    const QVector3D& pointPosition,
    const std::vector<Triangle>& meshTriangles)
{
	constexpr float Epsilon = 0.0000001f;

	const QVector3D rayVector = (origin - pointPosition).normalized();
	std::size_t trianglesPassed = 0;

	for (const Triangle& triangle : meshTriangles)
	{
		const QVector3D edge1 = triangle[1] - triangle[0];
		const QVector3D edge2 = triangle[2] - triangle[0];
		const QVector3D h = QVector3D::crossProduct(rayVector, edge2);
		const float a = QVector3D::dotProduct(edge1, h);

		if (-Epsilon < a && a < Epsilon)
		{
			continue;
		}

		const float f = 1.0f / a;
		const QVector3D s = origin - triangle[0];
		const float u = f * QVector3D::dotProduct(s, h);
		if (u < 0.0f || u > 1.0f)
		{
			continue;
		}

		const QVector3D q = QVector3D::crossProduct(s, edge1);
		const float v = f * QVector3D::dotProduct(rayVector, q);
		if (0.0f > v || (u + v) > 1.0f)
		{
			continue;
		}

		const float t = f * QVector3D::dotProduct(edge2, q);
		if (t > Epsilon)
		{
			++trianglesPassed;
		}
	}

	return {.m_point = pointPosition,
	    .m_status = trianglesPassed % 2 == 0 ? ApproximationPoint::PointStatus::Outside
	                                         : ApproximationPoint::PointStatus::Inside};
}
