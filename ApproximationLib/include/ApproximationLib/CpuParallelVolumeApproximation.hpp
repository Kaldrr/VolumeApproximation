#pragma once

#include <ApproximationLib/ICpuVolumeApproximation.hpp>

namespace VolumeApproximation::Impl
{

class APPROXIMATIONLIB_EXPORT [[nodiscard]] CpuParallelVolumeApproximation final
    : public ICpuVolumeApproximation
{
public:
	CpuParallelVolumeApproximation() = default;

	[[nodiscard]] ApproximationResult ComputeVolume(
	    std::span<const Triangle> triangles,
	    std::span<const Vector3F> randomPoints,
	    const Vector3F& minimumExtent,
	    const Vector3F& maximumExtent) override;
};
} // namespace VolumeApproximation::Impl