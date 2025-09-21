#pragma once

#include <ApproximationLib/IVolumeApproximation.hpp>
#include <ApproximationLib/Vector3.hpp>

#include <vector>

#include <QQuick3DGeometry>

namespace Helpers
{
[[nodiscard]]
std::vector<VolumeApproximation::Vector3F> GenerateRandomPoints(
    std::size_t count,
    const VolumeApproximation::Vector3F& minExtent,
    const VolumeApproximation::Vector3F& maxExtent);

[[nodiscard]]
std::vector<VolumeApproximation::Triangle> ParseTrianglesFromGeometry(
    const QQuick3DGeometry& geometry);
} // namespace Helpers