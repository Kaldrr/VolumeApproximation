#pragma once

#include <ApproximationLib/IVolumeApproximationStrategy.h>

class QVector3D;

class QTAPPROXLIB_EXPORT ICpuVolumeApproximationStrategy : public IVolumeApproximationStrategy
{
public:
	virtual ~ICpuVolumeApproximationStrategy() = default;

protected:
	static ApproximationPoint computePoint(const QVector3D& origin,
	    const QVector3D& pointPosition,
	    const std::vector<Triangle>& meshTriangles);
};