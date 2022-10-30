#include <ApproximationLib/ApproximationHelper.h>

#include <ApproximationLibCpu/ParallelAlgorithmVolumeApproximationStrategy.h>

#ifdef CUDA_ENABLED
#include <ApproximationLibCuda/CudaVolumeApproximator.h>
#endif

namespace VolumeApproximation
{
std::unique_ptr<IVolumeApproximationStrategy> CreateDefaultStrategy()
{
	// return std::make_unique<Impl::ParallelAlgorithmVolumeApproximationStrategy>();
	return CreateApproximationStrategy(ApproximationExecutor::Cuda);
}

std::unique_ptr<IVolumeApproximationStrategy> CreateApproximationStrategy(
    ApproximationExecutor approximationExecutor)
{
	switch (approximationExecutor)
	{
#ifdef CUDA_ENABLED
		case ApproximationExecutor::Cuda:
			return std::make_unique<Impl::CudaVolumeApproximator>();
#endif
		case ApproximationExecutor::ParallelCpu:
			return std::make_unique<Impl::ParallelAlgorithmVolumeApproximationStrategy>();
		default:
			return nullptr;
	}
	return nullptr;
}
} // namespace VolumeApproximation
