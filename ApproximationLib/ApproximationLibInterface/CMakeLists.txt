set(HEADER_FILES
    include/ApproximationLibInterface/IVolumeApproximationStrategy.h
    include/ApproximationLibInterface/Vector3.h 
    include/ApproximationLibInterface/ApproximationLibInterface_global.h)

set(SOURCE_FILES
    IVolumeApproximationStrategy.cpp
)

add_library(ApproximationLibInterface 
    SHARED
        ${HEADER_FILES}
        ${SOURCE_FILES}
)
target_include_directories(ApproximationLibInterface 
    PUBLIC
        include
)
target_compile_definitions(ApproximationLibInterface 
    PRIVATE
        APPROXIMATION_LIBRARY_INTERFACE
)