#pragma once

#include <ApproximationLib/ApproximationLib_global.h>
#include <ApproximationLibInterface/IVolumeApproximationStrategy.h>

#include <memory>
#include <string_view>

namespace VolumeApproximation
{
enum class ApproximationExecutor
{
	Cuda,
	ParallelCpu,

	// Always leave it at the end
	NumberOfExecutors
};

using ExecutorNamePair = std::pair<ApproximationExecutor, std::string_view>;

constexpr std::string_view EnumToString(
    const VolumeApproximation::ApproximationExecutor approximationStrategy)
{
	using VolumeApproximation::ApproximationExecutor;

	switch (approximationStrategy)
	{
		case ApproximationExecutor::Cuda:
			return "CUDA";
		case ApproximationExecutor::ParallelCpu:
			return "Parallel CPU";
		default:
			std::unreachable();
	}
	return "";
}

constexpr inline std::array<const ExecutorNamePair,
    static_cast<std::size_t>(ApproximationExecutor::NumberOfExecutors)>
    ExecutorNames{
        ExecutorNamePair{ApproximationExecutor::Cuda, EnumToString(ApproximationExecutor::Cuda)},
        ExecutorNamePair{ApproximationExecutor::ParallelCpu,
            EnumToString(ApproximationExecutor::ParallelCpu)}};

std::unique_ptr<IVolumeApproximationStrategy> APPROXIMATIONLIB_EXPORT CreateDefaultStrategy();

std::unique_ptr<IVolumeApproximationStrategy> APPROXIMATIONLIB_EXPORT CreateApproximationStrategy(
    ApproximationExecutor approximationExecutor);

} // namespace VolumeApproximation
