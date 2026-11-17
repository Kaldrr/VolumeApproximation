#include <ApproximationLib/Vector3.hpp>

#include <gtest/gtest.h>

using VolumeApproximation::Vector3F;

TEST(Vector3, DotProduct)
{
	EXPECT_FLOAT_EQ(Vector3F::DotProduct({ 1.F, 2.F, 3.F }, { 4.F, -5.F, 6.F }),
	                12.F);
	EXPECT_FLOAT_EQ(Vector3F::DotProduct({ 1.F, 0.F, 0.F }, { 0.F, 1.F, 0.F }),
	                0.F);
}

TEST(Vector3, CrossProductFollowsRightHandRule)
{
	const Vector3F z{ Vector3F::CrossProduct({ 1.F, 0.F, 0.F },
	                                         { 0.F, 1.F, 0.F }) };
	EXPECT_FLOAT_EQ(z.X(), 0.F);
	EXPECT_FLOAT_EQ(z.Y(), 0.F);
	EXPECT_FLOAT_EQ(z.Z(), 1.F);
}

TEST(Vector3, NormalizedHasUnitLength)
{
	const Vector3F n{ Vector3F{ 3.F, 4.F, 12.F }.Normalized() };
	EXPECT_NEAR(n.Length(), 1.F, 1e-6F);
	EXPECT_NEAR(n.X(), 3.F / 13.F, 1e-6F);
}

TEST(Vector3, NormalizedZeroVectorStaysZero)
{
	const Vector3F n{ Vector3F{}.Normalized() };
	EXPECT_FLOAT_EQ(n.Length(), 0.F);
}

TEST(Vector3, Arithmetic)
{
	const Vector3F v{ Vector3F{ 1.F, 2.F, 3.F } + Vector3F{ 1.F, 1.F, 1.F } };
	EXPECT_FLOAT_EQ(v.Z(), 4.F);
	EXPECT_FLOAT_EQ((v * 2.F).Y(), 6.F);
	EXPECT_FLOAT_EQ((-v).X(), -2.F);
}
