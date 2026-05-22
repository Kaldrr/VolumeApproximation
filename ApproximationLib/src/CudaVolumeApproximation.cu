#include <ApproximationLib/CudaVolumeApproximation.hpp>

#include <cuda_runtime.h>
#include <device_launch_parameters.h>

#include <thrust/device_vector.h>
#include <thrust/host_vector.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <utility>

#include <fmt/core.h>

namespace
{
using namespace VolumeApproximation;

__device__ bool IsPointInside(const Vector3F& origin,
                              const Vector3F& pointPosition,
                              const thrust::device_ptr<const Triangle> triangles,
                              const std::size_t trianglesSize)
{
	constexpr float Epsilon = 0.0000001f;

	const Vector3F rayVector    = (origin - pointPosition).Normalized();
	std::size_t trianglesPassed = 0;

	for (std::size_t i = 0; i < trianglesSize; ++i)
	{
		const Triangle& triangle = triangles[i];

		const Vector3F edge1 = triangle[1] - triangle[0];
		const Vector3F edge2 = triangle[2] - triangle[0];
		const Vector3F h     = Vector3F::CrossProduct(rayVector, edge2);
		const float a        = Vector3F::DotProduct(edge1, h);

		if (-Epsilon < a && a < Epsilon)
		{
			continue;
		}

		const float f    = 1.0f / a;
		const Vector3F s = origin - triangle[0];
		const float u    = f * Vector3F::DotProduct(s, h);
		if (u < 0.0f || u > 1.0f)
		{
			continue;
		}

		const Vector3F q = Vector3F::CrossProduct(s, edge1);
		const float v    = f * Vector3F::DotProduct(rayVector, q);
		if (0.0f > v || (u + v) > 1.0f)
		{
			continue;
		}

		const float t = f * Vector3F::DotProduct(edge2, q);
		if (t > Epsilon)
		{
			++trianglesPassed;
		}
	}

	return trianglesPassed % 2 == 1;
}

__global__ void ComputePoint(const thrust::device_ptr<const Triangle> triangles,
                             const std::size_t trianglesSize,
                             const thrust::device_ptr<const Vector3F> points,
                             const std::size_t pointsSize,
                             const Vector3F outsidePoint,
                             const thrust::device_ptr<bool> hitArray)
{
	const std::uint32_t i = (blockIdx.x * blockDim.x) + threadIdx.x;
	if (i < pointsSize)
	{
		hitArray[i] =
		    IsPointInside(points[i], outsidePoint, triangles, trianglesSize);
	}
}

template <typename T>
void Check(const T result,
           char const* const func,
           const char* const file,
           int const line)
{
	if (result)
	{
		throw std::runtime_error(fmt::format(
		    "CUDA error at {}:{} code={}({}) \"{}\"", file, line,
		    static_cast<std::underlying_type_t<decltype(result)>>(result),
		    cudaGetErrorName(result), func));
	}
}
#define CHECK_CUDA_ERRORS(val) Check((val), #val, __FILE__, __LINE__)
} // namespace

namespace VolumeApproximation::Impl
{
bool CudaVolumeApproximation::IsAvailable() const noexcept
{
	// Should be available if we have at least one device
	int devicesCount{};
	if (const cudaError_t status = cudaGetDeviceCount(&devicesCount);
	    status != cudaError_t::cudaSuccess)
	{
		fmt::print("Failed to get cuda devices count: {}\n",
		           cudaGetErrorString(status));
		return false;
	}
	return devicesCount > 0;
}

ApproximationResult CudaVolumeApproximation::ComputeVolume(
    const std::span<const Triangle> triangles,
    const std::span<const Vector3F> randomPoints,
    const Vector3F& minimumExtent,
    const Vector3F& maximumExtent)
{
	int deviceCount{};
	CHECK_CUDA_ERRORS(cudaGetDeviceCount(&deviceCount));
	if (deviceCount == 0)
	{
		throw std::runtime_error{"No CUDA devices found"};
	}

	const thrust::device_vector<Triangle> deviceTriangles{ triangles.begin(),
		                                                   triangles.end() };
	const thrust::device_vector<Vector3F> deviceRandomPoints{ randomPoints.begin(),
		                                                      randomPoints.end() };
	thrust::device_vector<bool> pointsHitStatus{};
	pointsHitStatus.resize(randomPoints.size(), false);

	constexpr std::size_t ThreadsPerBlock = 1024;
	const std::size_t pointsToCompute     = randomPoints.size();
	const auto [quot, rem] =
	    std::lldiv(static_cast<std::int64_t>(pointsToCompute), ThreadsPerBlock);

	const dim3 blockDimensions{
		static_cast<unsigned int>(quot + (rem == 0 ? 0U : 1U)), 1U, 1U
	};
	constexpr dim3 ThreadDimensions{ 1024U, 1U, 1U };

	ComputePoint<<<blockDimensions, ThreadDimensions>>>(
	    deviceTriangles.data(), triangles.size(), deviceRandomPoints.data(),
	    randomPoints.size(), minimumExtent - Vector3F{ 5.0F, 0.0F, 0.0F },
	    pointsHitStatus.data());
	CHECK_CUDA_ERRORS(cudaGetLastError());
	CHECK_CUDA_ERRORS(cudaDeviceSynchronize());

	const thrust::host_vector<bool> hostPointsHitStatus{ pointsHitStatus };

	std::vector<ApproximationPoint> outPoints{};
	outPoints.reserve(pointsToCompute);
	for (std::size_t i = 0; i < pointsToCompute; ++i)
	{
		outPoints.push_back(ApproximationPoint{
		    .Point  = randomPoints[i],
		    .Status = hostPointsHitStatus[i] ? PointStatus::Inside
		                                     : PointStatus::Outside });
	}

	const double cubeVolume =
	    static_cast<double>(std::abs(maximumExtent.X() - minimumExtent.X())) *
	    static_cast<double>(std::abs(maximumExtent.Y() - minimumExtent.Y())) *
	    static_cast<double>(std::abs(maximumExtent.Z() - minimumExtent.Z())) *
	    1'000'000.0;

	const std::int64_t pointsHit = std::ranges::count(hostPointsHitStatus, true);

	const double finalVolume =
	    cubeVolume *
	    (static_cast<double>(pointsHit) / static_cast<double>(randomPoints.size()));

	return ApproximationResult{ .Points    = std::move(outPoints),
		                        .Volume    = finalVolume,
		                        .MinExtent = minimumExtent,
		                        .MaxExtent = maximumExtent };
}
} // namespace VolumeApproximation::Impl
