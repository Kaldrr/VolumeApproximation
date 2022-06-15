#pragma once

#include <ApproximationLibInterface/ApproximationLibInterface_global.h>
#include <ApproximationLibInterface/Vector3.h>

#include <array>
#include <vector>
#include <cstdint>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

namespace VolumeApproximation
{
enum class PointStatus : std::uint8_t
{
	Inside,
	Outside
};

class APPROXIMATIONLIB_INTERFACE_EXPORT ApproximationPoint
{
public:
	Vector3F m_point{};
	PointStatus m_status{PointStatus::Outside};
};

class APPROXIMATIONLIB_INTERFACE_EXPORT ApproximationResult
{
public:
	std::vector<ApproximationPoint> m_points;
	double m_volume{0.0};

	Vector3F m_minExtent;
	Vector3F m_maxExtent;
};

using Triangle = std::array<Vector3F, 3>;

class APPROXIMATIONLIB_INTERFACE_EXPORT IVolumeApproximationStrategy
{
public:
	IVolumeApproximationStrategy() = default;
	virtual ~IVolumeApproximationStrategy() = default;

	IVolumeApproximationStrategy(const IVolumeApproximationStrategy&) = delete;
	IVolumeApproximationStrategy(IVolumeApproximationStrategy&&) = delete;
	IVolumeApproximationStrategy& operator=(const IVolumeApproximationStrategy&) = delete;
	IVolumeApproximationStrategy& operator=(IVolumeApproximationStrategy&&) = delete;

	virtual bool IsAvialiable() const = 0;

	virtual ApproximationResult ComputeVolume(const std::vector<Triangle>& triangles,
	    const std::vector<Vector3F>& randomPoints,
	    const Vector3F& minimumExtent,
	    const Vector3F& maximumExtent) = 0;
};
} // namespace VolumeApproximation

#ifdef _MSC_VER
#pragma warning(pop)
#endif
