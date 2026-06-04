#include <ApproximationLib/CudaVolumeApproximation.hpp>

#include <ApproximationLib/PointCalculations.hpp>

#include <cuda_runtime.h>
#include <device_launch_parameters.h>

#include <thrust/device_vector.h>
#include <thrust/host_vector.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <utility>

#include <fmt/core.h>

#if __has_cpp_attribute(cpp_impl_reflection)
#if __cpp_impl_reflection >= 202506L
#include <meta>
#define REFLECTION
#endif
#endif

namespace
{
#ifdef REFLECTION
template <typename T>
    requires std::is_enum_v<T>
[[nodiscard]] constexpr std::string_view EnumValue(const T val) noexcept
{
	template for (constexpr auto e :
	              std::define_static_array(std::meta::enumerators_of(^^T)))
	{
		if (val == [:e:])
			return std::meta::identifier_of(e);
	}

	return "<unknown>";
}
#else
template <typename T>
    requires std::is_enum_v<T>
[[nodiscard]] constexpr auto EnumValue(const T val) noexcept
{
	return static_cast<std::underlying_type_t<T>>(val);
}
#endif

__device__ bool IsPointInside(
    const VolumeApproximation::Vector3F& origin,
    const VolumeApproximation::Vector3F& pointPosition,
    const thrust::device_ptr<const VolumeApproximation::Triangle> triangles,
    const std::size_t trianglesSize)
{
	const VolumeApproximation::Vector3F rayVector =
	    (origin - pointPosition).Normalized();
	std::size_t trianglesPassed = 0;

	for (std::size_t i{}; i < trianglesSize; ++i)
	{
		const VolumeApproximation::Triangle& triangle = triangles[i];
		if (VolumeApproximation::RayIntersection(origin, rayVector, triangle))
		{
			++trianglesPassed;
		}
	}
	return trianglesPassed % 2 == 1;
}

__global__ void ComputePoint(
    const thrust::device_ptr<const VolumeApproximation::Triangle> triangles,
    const std::size_t trianglesSize,
    const thrust::device_ptr<const VolumeApproximation::Vector3F> points,
    const std::size_t pointsSize,
    const VolumeApproximation::Vector3F outsidePoint,
    const thrust::device_ptr<bool> hitArray)
{
	const std::uint32_t i = (blockIdx.x * blockDim.x) + threadIdx.x;
	if (i < pointsSize)
	{
		const VolumeApproximation::Vector3F point = points[i];
		hitArray[i] =
		    IsPointInside(point, outsidePoint, triangles, trianglesSize);
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
		throw std::runtime_error(
		    fmt::format("CUDA error at {}:{} code={}({}) \"{}\"", file, line,
		                EnumValue(result), cudaGetErrorName(result), func));
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
		throw std::runtime_error{ "No CUDA devices found" };
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
