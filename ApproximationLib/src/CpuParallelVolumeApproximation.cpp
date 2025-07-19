#include <ApproximationLib/CpuParallelVolumeApproximation.hpp>

#include <algorithm>
#include <execution>

namespace VolumeApproximation::Impl
{

ApproximationResult CpuParallelVolumeApproximation::ComputeVolume(
    const std::span<const Triangle> triangles,
    const std::span<const Vector3F> randomPoints,
    const Vector3F& minimumExtent,
    const Vector3F& maximumExtent)
{
	// Subtract random value from minExtent, to ensure the point we're shooting out
	// ray towards is out of the box that contains the entire object
	const Vector3F outsidePoint = minimumExtent - Vector3F{ 5.F, 0.F, 0.F };
	std::vector<ApproximationPoint> approximationPoints{};
	approximationPoints.resize(randomPoints.size());

	std::transform(std::execution::par_unseq, begin(randomPoints),
	               end(randomPoints), begin(approximationPoints),
	               [&](const Vector3F& randomPoint) {
		               return ComputePoint(randomPoint, outsidePoint, triangles);
	               });

	return ApproximationResult{ .Points    = std::move(approximationPoints),
		                        .Volume    = 0.0,
		                        .MinExtent = minimumExtent,
		                        .MaxExtent = maximumExtent };
}
} // namespace VolumeApproximation::Impl
