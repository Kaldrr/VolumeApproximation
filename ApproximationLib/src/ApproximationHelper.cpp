#include <ApproximationLib/ApproximationHelper.hpp>

#include <ApproximationLib/CpuParallelVolumeApproximation.hpp>

#ifdef CUDA_ENABLED
#include <ApproximationLib/CudaVolumeApproximation.hpp>
#endif

namespace VolumeApproximation
{
std::unique_ptr<IVolumeApproximation> CreateDefaultStrategy()
{
#ifdef CUDA_ENABLED
	return CreateApproximationStrategy(ApproximationExecutor::Cuda);
#else
	return std::make_unique<Impl::CpuParallelVolumeApproximation>();
#endif
}

std::unique_ptr<IVolumeApproximation> CreateApproximationStrategy(
    const ApproximationExecutor approximationExecutor)
{
	switch (approximationExecutor)
	{
#ifdef CUDA_ENABLED
		case ApproximationExecutor::Cuda:
			return std::make_unique<Impl::CudaVolumeApproximation>();
#endif
		case ApproximationExecutor::ParallelCpu:
			return std::make_unique<Impl::CpuParallelVolumeApproximation>();
	}
	std::unreachable();
}
} // namespace VolumeApproximation
