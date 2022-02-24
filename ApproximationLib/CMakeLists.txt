find_package(Qt6 COMPONENTS Core Gui 3DCore REQUIRED)

set(HEADER_FILES
    include/ApproximationLib/ApproximationLib_global.h
    include/ApproximationLib/VolumeApproximator.h
)

set(SOURCE_FILES
    VolumeApproximator.cpp
)

add_library(ApproximationLib SHARED
    ${HEADER_FILES}
    ${SOURCE_FILES}
)

target_link_libraries(ApproximationLib PRIVATE Qt6::Core Qt6::Gui Qt6::3DCore)
target_compile_options(ApproximationLib PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror -march=native>
)
target_include_directories(ApproximationLib PUBLIC include)
target_compile_definitions(ApproximationLib PRIVATE APPROXIMATION_LIBRARY)
