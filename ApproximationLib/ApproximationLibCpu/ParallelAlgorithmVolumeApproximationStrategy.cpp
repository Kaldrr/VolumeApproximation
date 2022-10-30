#include <ApproximationLibCpu/ParallelAlgorithmVolumeApproximationStrategy.h>

#include <algorithm>
#include <execution>
#include <version>

#pragma warning(push)
#pragma warning(disable : 4067)
#ifdef __has_cpp_attribute(__cpp_lib_execution)
#define USE_PARALLEL_EXECUTION
#else
#include <thread>
#include <functional>
#endif
#pragma warning(pop)

namespace VolumeApproximation::Impl
{

ApproximationResult ParallelAlgorithmVolumeApproximationStrategy::ComputeVolume(
    const std::vector<Triangle>& triangles,
    const std::vector<Vector3F>& randomPoints,
    const Vector3F& minimumExtent,
    const Vector3F& maximumExtent)
{
	// Substract random value from minExtent, to ensure point we're shooting out ray towards
	// is outside of the box that contains the entire object
	const Vector3F outsidePoint = minimumExtent - Vector3F{1.f, 2.f, 3.f};
	std::vector<ApproximationPoint> approximationPoints{};
	approximationPoints.resize(randomPoints.size());

#ifdef USE_PARALLEL_EXEUCTION
	std::transform(std::execution::par_unseq, begin(randomPoints), end(randomPoints),
	    begin(approximationPoints), [&](const Vector3F& randomPoint) {
		    return computePoint(randomPoint, outsidePoint, triangles);
	    });
#else
	const unsigned int cpuCount = std::thread::hardware_concurrency();
	std::vector<std::thread> workerThreads{};
	workerThreads.reserve(cpuCount);

	const auto functor = [&](const std::size_t startIndex, const std::size_t workSize) {
		const std::size_t endIndex = std::min(startIndex + workSize, size(randomPoints) - 1);
		for (std::size_t index = startIndex; index < endIndex; ++index)
		{
			approximationPoints[index] = computePoint(randomPoints[index], outsidePoint, triangles);
		}
	};

	const std::lldiv_t divResult = std::lldiv(size(approximationPoints), cpuCount);
	const std::size_t workSize = divResult.quot + divResult.rem ? 1 : 0;
	for (unsigned int i = 0; i < cpuCount; ++i)
	{
		const std::size_t startIndex = workSize * i;
		workerThreads.emplace_back(functor, startIndex, workSize);
	}

	std::for_each(begin(workerThreads), end(workerThreads), std::mem_fn(&std::thread::join));

#endif

	return ApproximationResult{.m_points = std::move(approximationPoints),
	    .m_volume = 0.0,
	    .m_minExtent = minimumExtent,
	    .m_maxExtent = maximumExtent};
}
} // namespace VolumeApproximation::Impl
