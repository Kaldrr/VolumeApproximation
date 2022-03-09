#pragma once

#include <ApproximationLib/ApproximationLib_global.h>
#include <ApproximationLib/IVolumeApproximationStrategy.h>

#include <vector>

#include <QtGui/QVector3D>

namespace Qt3DCore
{
class QGeometry;
class QAttribute;
} // namespace Qt3DCore

class QTAPPROXLIB_EXPORT VolumeApproximationManager
{
public:
	VolumeApproximationManager(const Qt3DCore::QGeometry& geometry);
	ApproximationResult getVolume(int sampleSize);

private:
	std::vector<Triangle> getGeometryTriangles() const;
	std::vector<QVector3D> generateRandomPoints(int count,
	    const QVector3D& minExtent,
	    const QVector3D& maxExtent) const;

	static std::vector<QVector3D> getVertices(const Qt3DCore::QAttribute& vertexAttribute);
	static std::vector<ushort> getIndices(const Qt3DCore::QAttribute& indexAttribute);
private:
	const Qt3DCore::QGeometry* const m_Geometry{};
	std::unique_ptr<IVolumeApproximationStrategy> m_VolumeApproximationStrategy{};
};
