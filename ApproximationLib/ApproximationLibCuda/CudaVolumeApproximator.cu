#include <ApproximationLibCuda/CudaVolumeApproximator.h>

#include <cuda_runtime.h>
#include <device_launch_parameters.h>

#include <thrust/copy.h>
#include <thrust/device_make_unique.h>
#include <thrust/device_vector.h>
#include <thrust/memory.h>
#include <thrust/transform.h>

#include <memory>

#include <fmt/core.h>



namespace
{
__global__ void ComputePoint(const thrust::device_ptr<const float> triangles,
    const std::size_t trianglesSize,
    const thrust::device_ptr<const float> points,
    const std::size_t pointsSize,
    const thrust::device_ptr<const float> minExtent,
    const thrust::device_ptr<const float> maxExtent)
{
	printf("Triangles size: %f, points size: %f, minExtent: {%f,%f,%f}, maxExtent: {%f,%f,%f}\n",
	    static_cast<float>(trianglesSize), static_cast<float>(pointsSize),
	    static_cast<float>(minExtent[0]), static_cast<float>(minExtent[1]),
	    static_cast<float>(minExtent[2]), static_cast<float>(maxExtent[0]),
	    static_cast<float>(maxExtent[1]), static_cast<float>(maxExtent[2]));
}
} // namespace

namespace VolumeApproximation::Impl
{

bool CudaVolumeApproximator::IsAvialiable() const noexcept
{
	// Should be avialiable if we have at least one device
	int devicesCount{};
	if (cudaGetDeviceCount(&devicesCount) != cudaError_t::cudaSuccess)
	{
		return false;
	}
	return devicesCount > 0;
}

ApproximationResult CudaVolumeApproximator::ComputeVolume(const std::vector<Triangle>& triangles,
    const std::vector<Vector3F>& randomPoints,
    const Vector3F& minimumExtent,
    const Vector3F& maximumExtent)
{
	// const thrust::device_vector<float> gpuTriangles{triangles};
	// const thrust::device_vector<float> gpuPoints{randomPoints};
	// const thrust::device_vector<float> gpuMinExtent{minimumExtent, minimumExtent + 3};
	// const thrust::device_vector<float> gpuMaxExtent{maximumExtent, maximumExtent + 3};

	// ComputePoint<<<1, 1>>>(gpuTriangles.data(), gpuTriangles.size(), gpuPoints.data(),
	//     gpuPoints.size(), gpuMinExtent.data(), gpuMaxExtent.data());

	return {};
}
} // namespace VolumeApproximation::Impl