set(CUDA_ENABLED false CACHE BOOL "Enable CUDA backend")

add_subdirectory(ApproximationLibInterface)
add_subdirectory(ApproximationLibCpu)
if(CUDA_ENABLED)
    add_definitions(CUDA_ENABLED)
    add_subdirectory(ApproximationLibCuda)
endif()

set(HEADER_FILES
    include/ApproximationLib/ApproximationHelper.h
    include/ApproximationLib/ApproximationLib_global.h 
)

set(SOURCE_FILES
    ApproximationHelper.cpp
)

add_library(ApproximationLib 
    SHARED
        ${HEADER_FILES}
        ${SOURCE_FILES}
)
target_link_libraries(ApproximationLib 
    PUBLIC 
        ApproximationLibInterface
    PRIVATE
    #   ApproximationLibCuda
        ApproximationLibCpu
)
target_include_directories(ApproximationLib 
    PUBLIC
        include
)
target_compile_options(ApproximationLib 
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror -march=native>
)
target_compile_definitions(ApproximationLib 
    PRIVATE
        APPROXIMATION_LIBRARY
)
