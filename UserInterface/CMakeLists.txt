find_package(Qt6 COMPONENTS Core Widgets 3DExtras Concurrent REQUIRED)
find_package(fmt REQUIRED)

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


qt_add_executable(VolumeApproximation
    MANUAL_FINALIZATION
    ${HEADER_FILES}
    ${SOURCE_FILES}
)

target_link_libraries(VolumeApproximation PUBLIC 
    ApproximationLib 
    Qt6::Core 
    Qt6::Widgets 
    Qt6::3DExtras 
    Qt6::Concurrent
    fmt::fmt
)
target_compile_options(VolumeApproximation PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror -march=native>
)
target_include_directories(VolumeApproximation PUBLIC include)

qt_finalize_executable(VolumeApproximation)
add_custom_command(TARGET VolumeApproximation POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:VolumeApproximation> $<TARGET_FILE_DIR:VolumeApproximation>
  COMMAND_EXPAND_LISTS
)

if (WIN32) 
get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

add_custom_command(TARGET VolumeApproximation POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E
        env PATH="${_qt_bin_dir}" "${WINDEPLOYQT_EXECUTABLE}"
            "$<$<CONFIG:Debug>:--debug>"
            "$<$<CONFIG:Release>:--release>"
            "--no-compiler-runtime"
            "$<TARGET_FILE:VolumeApproximation>"
    COMMENT "Running windeployqt..."
)
endif(WIN32)
