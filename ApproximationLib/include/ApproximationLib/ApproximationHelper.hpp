#pragma once

#include <ApproximationLib/ApproximationLib_global.hpp>
#include <ApproximationLib/IVolumeApproximation.hpp>

#include <memory>
#include <string_view>
#include <utility>

namespace VolumeApproximation
{
enum class ApproximationExecutor : std::uint8_t
{
#ifdef CUDA_ENABLED
	Cuda,
#endif
	ParallelCpu
};

[[nodiscard]] constexpr std::string_view EnumToString(
    const ApproximationExecutor approximationStrategy) noexcept
{
	using VolumeApproximation::ApproximationExecutor;

	switch (approximationStrategy)
	{
#ifdef CUDA_ENABLED
		case ApproximationExecutor::Cuda:
			return "CUDA";
#endif
		case ApproximationExecutor::ParallelCpu:
			return "Parallel CPU";
	}
	std::unreachable();
}

using ExecutorNamePair = std::pair<ApproximationExecutor, std::string_view>;
constexpr inline std::array ExecutorNames{
#ifdef CUDA_ENABLED
	ExecutorNamePair{ ApproximationExecutor::Cuda,
	                  EnumToString(ApproximationExecutor::Cuda) },
#endif
	ExecutorNamePair{ ApproximationExecutor::ParallelCpu,
	                  EnumToString(ApproximationExecutor::ParallelCpu) }
};

[[nodiscard]] std::unique_ptr<IVolumeApproximation> APPROXIMATIONLIB_EXPORT
CreateDefaultStrategy();

[[nodiscard]] std::unique_ptr<IVolumeApproximation> APPROXIMATIONLIB_EXPORT
CreateApproximationStrategy(ApproximationExecutor approximationExecutor);

} // namespace VolumeApproximation
