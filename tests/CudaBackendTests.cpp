// Only compiled when CUDA_ENABLED (see tests/CMakeLists.txt). Tests skip themselves
// when no usable GPU/driver is present: that is the runtime-detection requirement.
#include "TestHelpers.hpp"

#include <ApproximationLib/ApproximationHelper.hpp>
#include <ApproximationLib/CpuParallelVolumeApproximation.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <memory>

using TestHelpers::VolumeScale;
using VolumeApproximation::ApproximationExecutor;
using VolumeApproximation::Vector3F;

namespace
{
[[nodiscard]] std::unique_ptr<VolumeApproximation::IVolumeApproximation>
MakeCudaBackend()
{
	auto cuda{ VolumeApproximation::CreateApproximationStrategy(
		ApproximationExecutor::Cuda) };
	return cuda;
}
} // namespace

TEST(CudaBackend, AvailabilityCheckDoesNotThrow)
{
	const auto cuda{ MakeCudaBackend() };
	ASSERT_NE(cuda, nullptr);
	EXPECT_NO_THROW(
	    { [[maybe_unused]] const bool available = cuda->IsAvailable(); });
}

TEST(CudaBackend, CubeVolume)
{
	const auto cuda{ MakeCudaBackend() };
	if (!cuda->IsAvailable())
	{
		GTEST_SKIP() << "No usable CUDA device";
	}
	const Vector3F lo{ -2.F, -2.F, -2.F };
	const Vector3F hi{ 2.F, 2.F, 2.F };
	const auto cube{ TestHelpers::MakeBox({ -1.F, -1.F, -1.F },
	                                      { 1.F, 1.F, 1.F }) };
	const auto points{ TestHelpers::RandomPoints(200'000, lo, hi) };

	const auto result{ cuda->ComputeVolume(cube, points, lo, hi) };

	const double expected{ 8.0 * VolumeScale };
	EXPECT_NEAR(result.Volume, expected, expected * 0.03);
}

TEST(CudaBackend, MatchesCpuBackend)
{
	const auto cuda{ MakeCudaBackend() };
	if (!cuda->IsAvailable())
	{
		GTEST_SKIP() << "No usable CUDA device";
	}
	const Vector3F lo{ -0.5F, -0.5F, -0.5F };
	const Vector3F hi{ 1.5F, 1.5F, 1.5F };
	const auto tetra{ TestHelpers::MakeUnitTetrahedron() };
	const auto points{ TestHelpers::RandomPoints(100'000, lo, hi, 777U) };

	VolumeApproximation::Impl::CpuParallelVolumeApproximation cpu{};
	const auto cpuResult{ cpu.ComputeVolume(tetra, points, lo, hi) };
	const auto gpuResult{ cuda->ComputeVolume(tetra, points, lo, hi) };

	ASSERT_EQ(gpuResult.Points.size(), cpuResult.Points.size());
	// Same points, same algorithm: classifications should agree except for points
	// within float rounding of a face (device code may fuse multiply-adds)
	std::size_t mismatches{ 0 };
	for (std::size_t i{ 0 }; i < cpuResult.Points.size(); ++i)
	{
		if (cpuResult.Points[i].Status != gpuResult.Points[i].Status)
		{
			++mismatches;
		}
	}
	EXPECT_LE(mismatches, cpuResult.Points.size() / 1'000)
	    << "classification differs too often";
	EXPECT_NEAR(gpuResult.Volume, cpuResult.Volume, cpuResult.Volume * 0.01);
}
