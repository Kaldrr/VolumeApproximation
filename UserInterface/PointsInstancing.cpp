#include <PointsInstancing.hpp>

PointsInstancing::PointsInstancing()
    : PointsInstancing{ nullptr }
{
}

PointsInstancing::PointsInstancing(QQuick3DObject* parent)
    : QQuick3DInstancing{ parent }
{
}

void PointsInstancing::SetResult(
    const std::shared_ptr<const VolumeApproximation::ApproximationResult>& result)
{
	SetPoints(result->Points);
}

void PointsInstancing::SetPoints(
    const std::span<const VolumeApproximation::ApproximationPoint> points)
{
	m_InstanceBuffer.clear();

	constexpr QVector3D Scale{ 1.0F, 1.0F, 1.0F };
	constexpr QVector3D Rotation{ 0.0F, 0.0F, 0.0F };
	constexpr QColor InsideColor{ 0, 255, 0, 255 };  // Green
	constexpr QColor OutsideColor{ 255, 0, 0, 255 }; // Red

	m_InstanceCount = points.size();
	m_InstanceBuffer.reserve(
	    static_cast<qsizetype>(sizeof(InstanceTableEntry) * m_InstanceCount));
	for (const VolumeApproximation::ApproximationPoint& point : points)
	{
		const InstanceTableEntry entry = calculateTableEntry(
		    std::bit_cast<QVector3D>(point.Point), Scale, Rotation,
		    point.Status == VolumeApproximation::PointStatus::Inside
		        ? InsideColor
		        : OutsideColor);

		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		m_InstanceBuffer.append(reinterpret_cast<const char*>(&entry),
		                        sizeof(entry));
	}
	markDirty();
}

void PointsInstancing::ClearPoints()
{
	m_InstanceCount = 0UZ;
	m_InstanceBuffer.clear();
	markDirty();
}

QByteArray PointsInstancing::getInstanceBuffer(int* const instanceCount)
{
	*instanceCount = static_cast<int>(m_InstanceCount);
	return m_InstanceBuffer;
}