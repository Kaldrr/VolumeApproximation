#pragma once

#include <ApproximationLibCpu/ApproximationLib_global.h>

#include <ApproximationLibInterface/IVolumeApproximationStrategy.h>

namespace VolumeApproximation::Impl
{

class APPROXLIBCPU_EXPORT ICpuVolumeApproximationStrategy : public IVolumeApproximationStrategy
{
public:
	virtual ~ICpuVolumeApproximationStrategy() = default;
	bool IsAvialiable() const noexcept override { return true; };

protected:
	static ApproximationPoint computePoint(const Vector3F& origin,
	    const Vector3F& pointPosition,
	    const std::vector<Triangle>& meshTriangles);
};
} // namespace VolumeApproximation::Impl