#include <ApproximationLib/ParallelAlgorithmVolumeApproximationStrategy.h>

#include <algorithm>
#include <execution>

ApproximationResult
ParallelAlgorithmVolumeApproximationStrategy::ComputeVolume(const std::vector<Triangle>& triangles,
    const std::vector<QVector3D>& randomPoints,
    const QVector3D& minimumExtent,
    const QVector3D& maximumExtent)
{
	// Substract random value from minExtent, to ensure point we're shooting out ray towards
	// is outside of the box that contains the entire object
	const QVector3D outsidePoint = minimumExtent - QVector3D{1.f, 2.f, 3.f};
	std::vector<ApproximationPoint> approximationPoints{};
	approximationPoints.resize(randomPoints.size());

	std::transform(std::execution::par_unseq, begin(randomPoints), end(randomPoints),
	    begin(approximationPoints), [&](const QVector3D& randomPoint) {
		    return computePoint(randomPoint, outsidePoint, triangles);
	    });

	return ApproximationResult{.m_points = std::move(approximationPoints),
	    .m_volume = 0.0,
	    .m_minExtent = minimumExtent,
	    .m_maxExtent = maximumExtent};
}