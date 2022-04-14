#pragma once

#include <ApproximationLib/ApproximationLib_global.h>
#include <ApproximationLibInterface/IVolumeApproximationStrategy.h>

#include <memory>

namespace VolumeApproximation
{
enum class ApproximationExecutor
{
	Cuda,
	ParallelCpu
};

std::unique_ptr<IVolumeApproximationStrategy> APPROXIMATIONLIB_EXPORT CreateDefaultStrategy();

std::unique_ptr<IVolumeApproximationStrategy> APPROXIMATIONLIB_EXPORT CreateApproximationStrategy(
    ApproximationExecutor approximationExecutor);

} // namespace VolumeApproximation