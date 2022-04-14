#pragma once

#include <ApproximationLibCpu/ICpuVolumeApproximationStrategy.h>

namespace VolumeApproximation::Impl
{

class APPROXLIBCPU_EXPORT ParallelAlgorithmVolumeApproximationStrategy
    : public ICpuVolumeApproximationStrategy
{
public:
	ParallelAlgorithmVolumeApproximationStrategy() = default;

	ApproximationResult ComputeVolume(const std::vector<Triangle>& triangles,
	    const std::vector<Vector3F>& randomPoints,
	    const Vector3F& minimumExtent,
	    const Vector3F& maximumExtent) override;
};
} // namespace VolumeApproximation::Impl