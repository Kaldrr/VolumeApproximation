#pragma once

#include <ApproximationLib/ApproximationLib_global.h>

#include <vector>
#include <array>

#include <QtGui/QVector3D>

struct QTAPPROXLIB_EXPORT ApproximationPoint
{
	enum class PointStatus : std::uint8_t
	{
		Inside,
		Outside
	};
	QVector3D m_point{};
	PointStatus m_status{PointStatus::Outside};
};

class QTAPPROXLIB_EXPORT ApproximationResult
{
public:
	std::vector<ApproximationPoint> m_points;
	double m_volume{0.0};

	QVector3D m_minExtent;
	QVector3D m_maxExtent;
};

using Triangle = std::array<QVector3D, 3>;

class QTAPPROXLIB_EXPORT IVolumeApproximationStrategy
{
public:
	virtual ApproximationResult ComputeVolume(const std::vector<Triangle>& triangles,
	    const std::vector<QVector3D>& randomPoints,
	    const QVector3D& minimumExtent,
	    const QVector3D& maximumExtent) = 0;
};