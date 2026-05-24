find_package(fmt REQUIRED)

set(CUDA_ENABLED
    OFF
    CACHE BOOL "Enable CUDA backend")

if(CUDA_ENABLED)
  if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES 89)
  endif()
  if(NOT DEFINED CMAKE_CUDA_STANDARD)
    set(CMAKE_CUDA_STANDARD 20)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
  endif()
  set(CUDA_SOURCES src/CudaVolumeApproximation.cu
                   include/ApproximationLib/CudaVolumeApproximation.hpp
  )
endif()

set(HEADER_FILES
    include/ApproximationLib/ApproximationHelper.hpp
    include/ApproximationLib/ApproximationLib_global.hpp
    include/ApproximationLib/IVolumeApproximation.hpp
    include/ApproximationLib/Vector3.hpp
    include/ApproximationLib/CudaVolumeApproximation.hpp
    include/ApproximationLib/CpuParallelVolumeApproximation.hpp
    include/ApproximationLib/ICpuVolumeApproximation.hpp
    include/ApproximationLib/PointCalculations.hpp
)

set(SOURCE_FILES
    src/ApproximationHelper.cpp src/ICpuVolumeApproximationStrategy.cpp
    src/CpuParallelVolumeApproximation.cpp
)

add_library(ApproximationLib SHARED ${HEADER_FILES} ${SOURCE_FILES}
                                    ${CUDA_SOURCES})
# VolumeApproximation_project_options seems to sometimes break CUDA builds
# causing functions to return weird out of memory errors
# works with current version of CUDA (13.0) + GCC (15.2.1 20250813)
# but in case of issues comment it out here and in UserInterface
target_link_libraries(
  ApproximationLib
  PUBLIC VolumeApproximation_project_warnings
         VolumeApproximation_project_options
  PRIVATE tbb fmt::fmt)
target_include_directories(ApproximationLib PUBLIC include)
target_compile_definitions(ApproximationLib PRIVATE APPROXIMATION_LIBRARY)
if(CUDA_ENABLED)
  target_link_cuda(ApproximationLib)
  set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --expt-relaxed-constexpr")
  target_compile_definitions(ApproximationLib PUBLIC CUDA_ENABLED)
  set_target_properties(ApproximationLib PROPERTIES CUDA_ARCHITECTURES "89")
  target_include_directories(ApproximationLib
                             PUBLIC ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES})
endif()
