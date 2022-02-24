#pragma once

#include <ApproximationLib/ApproximationLib_global.h>

#include <vector>

#include <QtGui/QVector3D>

namespace Qt3DCore
{
class QGeometry;
class QAttribute;
} // namespace Qt3DCore

struct QTAPPROXLIB_EXPORT ApproximationPoint
{
	enum class PointStatus : std::uint8_t
	{
		Inside,
		Outside
	};
	QVector3D m_point{};
	PointStatus m_status{PointStatus::Outside};
};

class QTAPPROXLIB_EXPORT ApproximationResult
{
public:
	std::vector<ApproximationPoint> m_points;
	double m_volume{0.0};

	QVector3D m_minExtent;
	QVector3D m_maxExtent;
};

using Triangle = std::array<QVector3D, 3>;

class QTAPPROXLIB_EXPORT VolumeApproximator
{
public:
	VolumeApproximator(const Qt3DCore::QGeometry& geometry);

	ApproximationResult getVolume(int sampleSize);

private:
	ApproximationPoint computePoint(const QVector3D& origin,
	    const QVector3D& pointPosition,
	    const std::vector<Triangle>& meshTriangles) const;

	std::vector<Triangle> getGeometryTriangles() const;
	std::vector<QVector3D> generateRandomPoints(int count,
	    const QVector3D& minExtent,
	    const QVector3D& maxExtent) const;

	static std::vector<QVector3D> getVertices(const Qt3DCore::QAttribute& vertexAttribute);
	static std::vector<ushort> getIndices(const Qt3DCore::QAttribute& indexAttribute);

private:
	const Qt3DCore::QGeometry* const m_geometry{};
};
