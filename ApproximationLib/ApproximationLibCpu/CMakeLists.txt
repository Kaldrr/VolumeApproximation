find_package(fmt REQUIRED)

set(CMAKE_CXX_STANDARD 23)

set(HEADER_FILES
    include/ApproximationLibCpu/ApproximationLib_global.h
    include/ApproximationLibCpu/ICpuVolumeApproximationStrategy.h
    include/ApproximationLibCpu/ParallelAlgorithmVolumeApproximationStrategy.h
)

set(SOURCE_FILES
    ICpuVolumeApproximationStrategy.cpp
    ParallelAlgorithmVolumeApproximationStrategy.cpp
)

add_library(ApproximationLibCpu 
    SHARED  
        ${HEADER_FILES}
        ${SOURCE_FILES}
)

target_link_libraries(ApproximationLibCpu 
    PUBLIC
        fmt::fmt 
        ApproximationLibInterface
)
target_compile_options(ApproximationLibCpu 
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror -march=native>
)
target_include_directories(ApproximationLibCpu 
    PUBLIC
        include
)
target_compile_definitions(ApproximationLibCpu 
    PRIVATE 
        APPROXIMATION_LIBRARY_CPU
)