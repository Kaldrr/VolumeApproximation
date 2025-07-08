#pragma once

#include <ApproximationLib/ApproximationLib_global.hpp>

#include <ApproximationLib/Vector3.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace VolumeApproximation
{
enum class PointStatus : std::uint8_t
{
	Inside,
	Outside
};

class APPROXIMATIONLIB_EXPORT [[nodiscard]] ApproximationPoint
{
public:
	Vector3F Point;
	PointStatus Status{ PointStatus::Outside };
};

class APPROXIMATIONLIB_EXPORT [[nodiscard]] ApproximationResult
{
public:
	std::vector<ApproximationPoint> Points;
	double Volume{ 0.0 };

	Vector3F MinExtent;
	Vector3F MaxExtent;
};

using Triangle = std::array<Vector3F, 3>;

class APPROXIMATIONLIB_EXPORT [[nodiscard]] IVolumeApproximation
{
public:
	IVolumeApproximation() noexcept          = default;
	virtual ~IVolumeApproximation() noexcept = default;

	IVolumeApproximation(const IVolumeApproximation&)                = delete;
	IVolumeApproximation(IVolumeApproximation&&) noexcept            = delete;
	IVolumeApproximation& operator=(const IVolumeApproximation&)     = delete;
	IVolumeApproximation& operator=(IVolumeApproximation&&) noexcept = delete;

	[[nodiscard]] virtual bool IsAvailable() const noexcept = 0;
	[[nodiscard]] virtual ApproximationResult ComputeVolume(
	    std::span<const Triangle> triangles,
	    std::span<const Vector3F> randomPoints,
	    const Vector3F& minimumExtent,
	    const Vector3F& maximumExtent) = 0;
};
} // namespace VolumeApproximation
