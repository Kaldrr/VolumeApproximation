#include "TestHelpers.hpp"

#include <ApproximationLib/ApproximationHelper.hpp>
#include <ApproximationLib/CpuParallelVolumeApproximation.hpp>

#include <gtest/gtest.h>

#include <algorithm>

using TestHelpers::VolumeScale;
using VolumeApproximation::Vector3F;
using VolumeApproximation::Impl::CpuParallelVolumeApproximation;

namespace
{
constexpr std::size_t PointCount{ 200'000 };

// Sampling box larger than the mesh: the inside fraction is well below 1, so
// the estimate is actually Monte Carlo. Std. error of the fraction p is
// sqrt(p(1-p)/N); 3% relative tolerance is > 5 sigma for these cases.
constexpr double RelativeTolerance{ 0.03 };
} // namespace

TEST(CpuBackend, IsAlwaysAvailable)
{
	EXPECT_TRUE(CpuParallelVolumeApproximation{}.IsAvailable());
}

TEST(CpuBackend, CubeVolume)
{
	const Vector3F lo{ -2.F, -2.F, -2.F };
	const Vector3F hi{ 2.F, 2.F, 2.F };
	const auto cube{ TestHelpers::MakeBox({ -1.F, -1.F, -1.F },
	                                      { 1.F, 1.F, 1.F }) }; // V = 8
	const auto points{ TestHelpers::RandomPoints(PointCount, lo, hi) };

	CpuParallelVolumeApproximation backend{};
	const auto result{ backend.ComputeVolume(cube, points, lo, hi) };

	const double expected{ 8.0 * VolumeScale };
	EXPECT_NEAR(result.Volume, expected, expected * RelativeTolerance);
}

TEST(CpuBackend, TetrahedronVolume)
{
	const Vector3F lo{ -0.5F, -0.5F, -0.5F };
	const Vector3F hi{ 1.5F, 1.5F, 1.5F };
	const auto tetra{ TestHelpers::MakeUnitTetrahedron() }; // V = 1/6
	const auto points{ TestHelpers::RandomPoints(PointCount, lo, hi) };

	CpuParallelVolumeApproximation backend{};
	const auto result{ backend.ComputeVolume(tetra, points, lo, hi) };

	const double expected{ VolumeScale / 6.0 };
	// p = (1/6)/8 is small, so the relative error is larger: allow 6% (~5 sigma)
	EXPECT_NEAR(result.Volume, expected, expected * 2.0 * RelativeTolerance);
}

TEST(CpuBackend, ResultKeepsPointsAndExtents)
{
	const Vector3F lo{ -2.F, -2.F, -2.F };
	const Vector3F hi{ 2.F, 2.F, 2.F };
	const auto cube{ TestHelpers::MakeBox({ -1.F, -1.F, -1.F },
	                                      { 1.F, 1.F, 1.F }) };
	const auto points{ TestHelpers::RandomPoints(1'000, lo, hi) };

	CpuParallelVolumeApproximation backend{};
	const auto result{ backend.ComputeVolume(cube, points, lo, hi) };

	ASSERT_EQ(result.Points.size(), points.size());
	EXPECT_FLOAT_EQ(result.Points[0].Point.X(), points[0].X());
	EXPECT_FLOAT_EQ(result.MinExtent.X(), lo.X());
	EXPECT_FLOAT_EQ(result.MaxExtent.Z(), hi.Z());
	EXPECT_TRUE(std::ranges::any_of(result.Points, [](const auto& p) {
		return p.Status == VolumeApproximation::PointStatus::Inside;
	}));
}

TEST(Factory, ParallelCpuStrategy)
{
	const auto strategy{ VolumeApproximation::CreateApproximationStrategy(
		VolumeApproximation::ApproximationExecutor::ParallelCpu) };
	ASSERT_NE(strategy, nullptr);
	EXPECT_TRUE(strategy->IsAvailable());
}

TEST(Factory, DefaultStrategyIsAlwaysUsable)
{
	// Hard requirement: with or without CUDA (compiled in or present at runtime),
	// the default strategy must be available
	const auto strategy{ VolumeApproximation::CreateDefaultStrategy() };
	ASSERT_NE(strategy, nullptr);
	EXPECT_TRUE(strategy->IsAvailable());
}

TEST(Factory, ExecutorNamesListParallelCpu)
{
	EXPECT_TRUE(std::ranges::any_of(
	    VolumeApproximation::ExecutorNames, [](const auto& pair) {
		    return pair.first ==
			       VolumeApproximation::ApproximationExecutor::ParallelCpu;
	    }));
}
