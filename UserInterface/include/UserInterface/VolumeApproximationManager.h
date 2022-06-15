#pragma once

#include <ApproximationLib/ApproximationHelper.h>

#include <vector>

#include <QtCore/QObject>

#include <QtGui/QVector3D>

namespace Qt3DCore
{
class QGeometry;
class QAttribute;
} // namespace Qt3DCore

class VolumeApproximationManager : public QObject
{
    Q_OBJECT

public:
	VolumeApproximationManager();
	explicit VolumeApproximationManager(QObject* parent);

public slots:
	void geometryVolumeRequested(const Qt3DCore::QGeometry* geometry, int sampleSize);
	void SetStrategy(VolumeApproximation::ApproximationExecutor strategy);

signals:
	void approximationFinished(const VolumeApproximation::ApproximationResult& approximationResult);

private:
	static std::vector<VolumeApproximation::Triangle> getGeometryTriangles(
	    const Qt3DCore::QGeometry& geometry);
	static std::vector<VolumeApproximation::Vector3F> generateRandomPoints(int count,
	    const VolumeApproximation::Vector3F& minExtent,
	    const VolumeApproximation::Vector3F& maxExtent);

	static std::vector<VolumeApproximation::Vector3F> getVertices(
	    const Qt3DCore::QAttribute& vertexAttribute);
	static std::vector<ushort> getIndices(const Qt3DCore::QAttribute& indexAttribute);

private:
	std::unique_ptr<VolumeApproximation::IVolumeApproximationStrategy>
	    m_VolumeApproximationStrategy{};
};
