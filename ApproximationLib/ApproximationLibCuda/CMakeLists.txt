find_package(fmt REQUIRED)

enable_language(CUDA)
if(NOT DEFINED CMAKE_CUDA_STANDARD)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
endif()

set(HEADER_FILES
    include/ApproximationLibCuda/ApproximationLibCuda_global.h
    include/ApproximationLibCuda/CudaVolumeApproximator.h
)

set(SOURCE_FILES
    CudaVolumeApproximator.cu
)

add_library(ApproximationLibCuda 
    SHARED
        ${HEADER_FILES}
        ${SOURCE_FILES}
)

target_link_libraries(ApproximationLibCuda 
    PUBLIC 
        fmt::fmt 
        ApproximationLibInterface
)
# Setting this in cuda breaks the compiler...
#target_compile_options(ApproximationLib PRIVATE
#  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
#  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror -march=native>
#)
target_include_directories(ApproximationLibCuda 
    PUBLIC
        include
    PRIVATE  
        ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}
)
target_compile_definitions(ApproximationLibCuda 
    PRIVATE 
        APPROXIMATION_LIBRARY_CUDA
)
set_target_properties(ApproximationLibCuda 
    PROPERTIES 
        CUDA_SEPARABLE_COMPILATION ON
)
