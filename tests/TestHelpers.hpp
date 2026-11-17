#pragma once

#include <ApproximationLib/IVolumeApproximation.hpp>
#include <ApproximationLib/Vector3.hpp>

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace TestHelpers
{
using VolumeApproximation::Triangle;
using VolumeApproximation::Vector3F;

// Backends report the volume scaled by 1'000'000 (cubic units -> 1e6 sub-units,
// e.g. m^3 -> cm^3), see Cpu/CudaVolumeApproximation::ComputeVolume.
constexpr double VolumeScale{ 1'000'000.0 };

/// Axis-aligned box as 12 triangles (2 per face)
[[nodiscard]] inline std::vector<Triangle> MakeBox(const Vector3F& lo,
                                                   const Vector3F& hi)
{
	// 8 corners, index bits: x = 1, y = 2, z = 4
	const auto corner = [&](const int i) {
		return Vector3F{ (i & 1) != 0 ? hi.X() : lo.X(),
		                 (i & 2) != 0 ? hi.Y() : lo.Y(),
		                 (i & 4) != 0 ? hi.Z() : lo.Z() };
	};
	constexpr int Faces[6][4]{
		{ 0, 1, 3, 2 }, // z = lo
		{ 4, 5, 7, 6 }, // z = hi
		{ 0, 1, 5, 4 }, // y = lo
		{ 2, 3, 7, 6 }, // y = hi
		{ 0, 2, 6, 4 }, // x = lo
		{ 1, 3, 7, 5 }, // x = hi
	};
	std::vector<Triangle> triangles{};
	for (const auto& face : Faces)
	{
		triangles.push_back({ corner(face[0]), corner(face[1]), corner(face[2]) });
		triangles.push_back({ corner(face[0]), corner(face[2]), corner(face[3]) });
	}
	return triangles;
}

/// Tetrahedron (0,0,0), (1,0,0), (0,1,0), (0,0,1): volume 1/6
[[nodiscard]] inline std::vector<Triangle> MakeUnitTetrahedron()
{
	const Vector3F o{ 0.F, 0.F, 0.F };
	const Vector3F x{ 1.F, 0.F, 0.F };
	const Vector3F y{ 0.F, 1.F, 0.F };
	const Vector3F z{ 0.F, 0.F, 1.F };
	return { { o, y, x }, { o, x, z }, { o, z, y }, { x, y, z } };
}

/// Deterministic uniform points in [lo, hi], so test results are reproducible
[[nodiscard]] inline std::vector<Vector3F> RandomPoints(
    const std::size_t count,
    const Vector3F& lo,
    const Vector3F& hi,
    const std::uint32_t seed = 12345U)
{
	std::mt19937 rng{ seed };
	std::uniform_real_distribution<float> rx{ lo.X(), hi.X() };
	std::uniform_real_distribution<float> ry{ lo.Y(), hi.Y() };
	std::uniform_real_distribution<float> rz{ lo.Z(), hi.Z() };
	std::vector<Vector3F> points{};
	points.reserve(count);
	for (std::size_t i{ 0 }; i < count; ++i)
	{
		points.emplace_back(rx(rng), ry(rng), rz(rng));
	}
	return points;
}
} // namespace TestHelpers
