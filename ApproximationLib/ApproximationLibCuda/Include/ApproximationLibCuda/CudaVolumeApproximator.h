#pragma once

#include <ApproximationLibCuda/ApproximationLibCuda_global.h>

#include <ApproximationLibInterface/IVolumeApproximationStrategy.h>

namespace VolumeApproximation
{
namespace Impl
{

class APPROXLIBCUDA_EXPORT CudaVolumeApproximator : public IVolumeApproximationStrategy
{
public:
	CudaVolumeApproximator() = default;

	bool IsAvialiable() const override;
	ApproximationResult ComputeVolume(const std::vector<Triangle>& triangles,
	    const std::vector<Vector3F>& randomPoints,
	    const Vector3F& minimumExtent,
	    const Vector3F& maximumExtent) override;
};
} // namespace Impl
} // namespace VolumeApproximation