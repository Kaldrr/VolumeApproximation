find_package(fmt REQUIRED)

set(CUDA_ENABLED
    ON
    CACHE BOOL "Enable CUDA backend")

if(CUDA_ENABLED)
  # Ugly hack :( but CUDA complains about gcc-15 for now
  set(CMAKE_CUDA_HOST_COMPILER "g++-14")
  if(NOT DEFINED CMAKE_CUDA_STANDARD)
    set(CMAKE_CUDA_STANDARD 20)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
  endif()
  set(CUDA_SOURCES src/CudaVolumeApproximation.cu
                   include/ApproximationLib/CudaVolumeApproximation.hpp)
endif()

set(HEADER_FILES
    include/ApproximationLib/ApproximationHelper.hpp
    include/ApproximationLib/ApproximationLib_global.hpp
    include/ApproximationLib/IVolumeApproximation.hpp
    include/ApproximationLib/Vector3.hpp
    include/ApproximationLib/CudaVolumeApproximation.hpp
    include/ApproximationLib/CpuParallelVolumeApproximation.hpp
    include/ApproximationLib/ICpuVolumeApproximation.hpp)

set(SOURCE_FILES
    src/ApproximationHelper.cpp src/ICpuVolumeApproximationStrategy.cpp
    src/CpuParallelVolumeApproximation.cpp)

add_library(ApproximationLib SHARED ${HEADER_FILES} ${SOURCE_FILES}
                                    ${CUDA_SOURCES})
target_link_cuda(ApproximationLib)
# TODO: project_options seems to be not working with CUDA :(
# Warnings are ok, but VolumeApproximation_project_options not so much
target_link_libraries(
  ApproximationLib PRIVATE tbb fmt::fmt VolumeApproximation_project_warnings
        )
target_include_directories(ApproximationLib PUBLIC include)
target_compile_definitions(ApproximationLib PRIVATE APPROXIMATION_LIBRARY)
if(CUDA_ENABLED)
  set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --expt-relaxed-constexpr")
  target_compile_definitions(ApproximationLib PUBLIC CUDA_ENABLED)
  set_target_properties(ApproximationLib PROPERTIES CUDA_ARCHITECTURES "89")
  target_include_directories(ApproximationLib
                             PUBLIC ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES})
endif()
