#pragma once

#include <ApproximationLib/ApproximationLib_global.hpp>

#include <ApproximationLib/IVolumeApproximation.hpp>

namespace VolumeApproximation::Impl
{

class APPROXIMATIONLIB_EXPORT [[nodiscard]] CudaVolumeApproximation final
    : public IVolumeApproximation
{
public:
	CudaVolumeApproximation() = default;

	[[nodiscard]] bool IsAvailable() const noexcept override;
	[[nodiscard]] ApproximationResult ComputeVolume(
	    std::span<const Triangle> triangles,
	    std::span<const Vector3F> randomPoints,
	    const Vector3F& minimumExtent,
	    const Vector3F& maximumExtent) override;
};
} // namespace VolumeApproximation::Impl
