find_package(QT NAMES Qt6 COMPONENTS Core REQUIRED)
find_package(Qt6 COMPONENTS Core REQUIRED)


set(HEADER_FILES
    include/ApproximationLib/ApproximationLib_global.h
)

set(SOURCE_FILES
)

add_library(ApproximationLib SHARED
    ${HEADER_FILES}
    ${SOURCE_FILES}
)

target_link_libraries(ApproximationLib PRIVATE Qt6::Core)
target_compile_options(ApproximationLib PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror -march=native>
)
target_include_directories(ApproximationLib PUBLIC include)
target_compile_definitions(ApproximationLib PRIVATE APPROXIMATION_LIBRARY)
