#include <ApproximationLibCpu/ParallelAlgorithmVolumeApproximationStrategy.h>

#include <algorithm>
#include <execution>

namespace VolumeApproximation::Impl
{

ApproximationResult
ParallelAlgorithmVolumeApproximationStrategy::ComputeVolume(const std::vector<Triangle>& triangles,
    const std::vector<Vector3F>& randomPoints,
    const Vector3F& minimumExtent,
    const Vector3F& maximumExtent)
{
	// Substract random value from minExtent, to ensure point we're shooting out ray towards
	// is outside of the box that contains the entire object
	const Vector3F outsidePoint = minimumExtent - Vector3F{1.f, 2.f, 3.f};
	std::vector<ApproximationPoint> approximationPoints{};
	approximationPoints.resize(randomPoints.size());

	std::transform(std::execution::par_unseq, begin(randomPoints), end(randomPoints),
	    begin(approximationPoints), [&](const Vector3F& randomPoint) {
		    return computePoint(randomPoint, outsidePoint, triangles);
	    });

	return ApproximationResult{.m_points = std::move(approximationPoints),
	    .m_volume = 0.0,
	    .m_minExtent = minimumExtent,
	    .m_maxExtent = maximumExtent};
}
} // namespace VolumeApproximation::Impl