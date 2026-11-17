#include "TestHelpers.hpp"

#include <ApproximationLib/ICpuVolumeApproximation.hpp>
#include <ApproximationLib/PointCalculations.hpp>

#include <gtest/gtest.h>

using VolumeApproximation::PointStatus;
using VolumeApproximation::RayIntersection;
using VolumeApproximation::Triangle;
using VolumeApproximation::Vector3F;
using VolumeApproximation::Impl::ICpuVolumeApproximation;

namespace
{
// Triangle in the z = 0 plane
const Triangle Flat{ Vector3F{ 0.F, 0.F, 0.F }, Vector3F{ 1.F, 0.F, 0.F },
                     Vector3F{ 0.F, 1.F, 0.F } };
} // namespace

TEST(RayIntersection, HitsThroughInterior)
{
	EXPECT_TRUE(RayIntersection({ 0.25F, 0.25F, 1.F }, { 0.F, 0.F, -1.F }, Flat));
}

TEST(RayIntersection, MissesOutsideTriangle)
{
	EXPECT_FALSE(RayIntersection({ 0.9F, 0.9F, 1.F }, { 0.F, 0.F, -1.F }, Flat));
}

TEST(RayIntersection, IgnoresTriangleBehindRayOrigin)
{
	EXPECT_FALSE(RayIntersection({ 0.25F, 0.25F, 1.F }, { 0.F, 0.F, 1.F }, Flat));
}

TEST(RayIntersection, ParallelRayDoesNotHit)
{
	EXPECT_FALSE(RayIntersection({ -1.F, 0.25F, 0.F }, { 1.F, 0.F, 0.F }, Flat));
}

TEST(PointClassification, CubeInsideAndOutside)
{
	const auto cube{ TestHelpers::MakeBox({ -1.F, -1.F, -1.F },
	                                      { 1.F, 1.F, 1.F }) };
	const Vector3F outside{ -6.F, -1.F, -1.F }; // what the backends use: minExtent
	                                            // - (5, 0, 0)

	EXPECT_EQ(
	    ICpuVolumeApproximation::ComputePoint({ 0.1F, 0.2F, 0.3F }, outside, cube)
	        .Status,
	    PointStatus::Inside);
	EXPECT_EQ(
	    ICpuVolumeApproximation::ComputePoint({ 1.5F, 0.2F, 0.3F }, outside, cube)
	        .Status,
	    PointStatus::Outside);
	EXPECT_EQ(
	    ICpuVolumeApproximation::ComputePoint({ 0.1F, -3.F, 0.3F }, outside, cube)
	        .Status,
	    PointStatus::Outside);
}
