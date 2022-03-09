#pragma once

#include <ApproximationLib/ICpuVolumeApproximationStrategy.h>

class QTAPPROXLIB_EXPORT ParallelAlgorithmVolumeApproximationStrategy
    : public ICpuVolumeApproximationStrategy
{
public:
	ParallelAlgorithmVolumeApproximationStrategy() = default;

	ApproximationResult ComputeVolume(const std::vector<Triangle>& triangles,
	    const std::vector<QVector3D>& randomPoints,
	    const QVector3D& minimumExtent,
	    const QVector3D& maximumExtent) override;
};