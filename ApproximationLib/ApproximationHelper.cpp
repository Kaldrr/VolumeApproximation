#include <ApproximationLib/ApproximationHelper.h>

#include <ApproximationLibCpu/ParallelAlgorithmVolumeApproximationStrategy.h>

#include <ApproximationLibCuda/CudaVolumeApproximator.h>

namespace VolumeApproximation
{
std::unique_ptr<IVolumeApproximationStrategy> CreateDefaultStrategy()
{
	return std::make_unique<Impl::ParallelAlgorithmVolumeApproximationStrategy>();
}

std::unique_ptr<IVolumeApproximationStrategy> CreateApproximationStrategy(
    ApproximationExecutor approximationExecutor)
{
	switch (approximationExecutor)
	{
		case ApproximationExecutor::Cuda:
			return std::make_unique<Impl::CudaVolumeApproximator>();
		case ApproximationExecutor::ParallelCpu:
			return std::make_unique<Impl::ParallelAlgorithmVolumeApproximationStrategy>();
	}
	return nullptr;
}
} // namespace VolumeApproximation