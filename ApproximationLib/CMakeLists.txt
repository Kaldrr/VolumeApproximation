find_package(fmt REQUIRED)
find_package(TBB REQUIRED) # backs std::execution::par_unseq in libstdc++

if(CUDA_ENABLED AND NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
  # Must be set before the target is created (it initialises the target's
  # CUDA_ARCHITECTURES). Override with -DCMAKE_CUDA_ARCHITECTURES=<arch|native>
  set(CMAKE_CUDA_ARCHITECTURES 89)
endif()

add_library(ApproximationLib SHARED)

target_sources(
  ApproximationLib
  PRIVATE include/ApproximationLib/ApproximationHelper.hpp
          include/ApproximationLib/ApproximationLib_global.hpp
          include/ApproximationLib/IVolumeApproximation.hpp
          include/ApproximationLib/Vector3.hpp
          include/ApproximationLib/CudaVolumeApproximation.hpp
          include/ApproximationLib/CpuParallelVolumeApproximation.hpp
          include/ApproximationLib/ICpuVolumeApproximation.hpp
          include/ApproximationLib/PointCalculations.hpp
          src/ApproximationHelper.cpp
          src/ICpuVolumeApproximationStrategy.cpp
          src/CpuParallelVolumeApproximation.cpp)

target_include_directories(ApproximationLib PUBLIC include)
target_compile_definitions(ApproximationLib PRIVATE APPROXIMATION_LIBRARY)

# Project flags stay PRIVATE: consumers (UserInterface) link them on their own
target_link_libraries(
  ApproximationLib
  PRIVATE VolumeApproximation_project_options
          VolumeApproximation_project_warnings
          VolumeApproximation_sanitizers
          TBB::tbb
          fmt::fmt)

if(CUDA_ENABLED)
  target_sources(ApproximationLib PRIVATE src/CudaVolumeApproximation.cu)
  # project_options macro: enable_language(CUDA), PIC, separable compilation
  target_link_cuda(ApproximationLib)
  set_target_properties(ApproximationLib PROPERTIES CUDA_STANDARD 20 CUDA_STANDARD_REQUIRED ON)
  target_compile_options(ApproximationLib PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--expt-relaxed-constexpr>)
  # PUBLIC so ApproximationHelper.hpp exposes the same backend list to consumers
  target_compile_definitions(ApproximationLib PUBLIC CUDA_ENABLED)
endif()
