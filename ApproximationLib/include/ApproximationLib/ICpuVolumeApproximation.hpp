#pragma once

#include <ApproximationLib/ApproximationLib_global.hpp>

#include <ApproximationLib/IVolumeApproximation.hpp>

namespace VolumeApproximation::Impl
{

class APPROXIMATIONLIB_EXPORT [[nodiscard]] ICpuVolumeApproximation
    : public IVolumeApproximation
{
public:
	[[nodiscard]] bool IsAvailable() const noexcept final { return true; }

	[[nodiscard]] static ApproximationPoint ComputePoint(
	    const Vector3F& origin,
	    const Vector3F& pointPosition,
	    std::span<const Triangle> meshTriangles) noexcept;
};
} // namespace VolumeApproximation::Impl