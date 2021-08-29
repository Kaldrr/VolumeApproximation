find_package(Qt6 COMPONENTS Core Widgets 3DExtras Concurrent REQUIRED)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)


set(HEADER_FILES
    include/UserInterface/MainWindow.h
    include/UserInterface/DisplayWidget.h
    include/UserInterface/SceneContainer.h
)

set(SOURCE_FILES
    main.cpp
    MainWindow.cpp MainWindow.ui
    DisplayWidget.cpp DisplayWidget.ui
    SceneContainer.cpp SceneContainer.ui
)

add_executable(VolumeApproximation
    ${HEADER_FILES}
    ${SOURCE_FILES}
)

target_link_libraries(VolumeApproximation PUBLIC ApproximationLib Qt6::Core Qt6::Widgets Qt6::3DExtras Qt6::Concurrent)
target_compile_options(VolumeApproximation PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror -march=native>
)
target_include_directories(VolumeApproximation PUBLIC include)
