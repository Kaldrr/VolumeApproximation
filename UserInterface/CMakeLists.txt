find_package(
  Qt6
      COMPONENTS Core Quick Concurrent Quick3D QuickControls2
  REQUIRED)
find_package(fmt REQUIRED)
find_package(assimp REQUIRED)

qt_standard_project_setup(REQUIRES 6.9)

set(HEADER_FILES
        SceneController.hpp CustomGeometry.hpp
)

set(SOURCE_FILES
    main.cpp
        SceneController.cpp CustomGeometry.cpp
)

qt_add_executable(VolumeApproximation ${HEADER_FILES}
                  ${SOURCE_FILES})

qt_add_qml_module(VolumeApproximation
        URI UserInterface
        VERSION 1.0
        QML_FILES ui/MainWindow.qml
        SOURCES
        SceneController.hpp SceneController.cpp

        # Compile qml files (listed in QML_FILES) to C++ using qmltc and add these
        # files to the application binary:3
        # Currently fails with QtQuick.Controls :(
        # https://doc.qt.io/qt-6/qtqml-qml-type-compiler.html#known-limitations
        #ENABLE_TYPE_COMPILER
        #NO_GENERATE_EXTRA_QMLDIRS
)

target_link_libraries(
  VolumeApproximation PRIVATE
        ApproximationLib
        VolumeApproximation_project_warnings
        VolumeApproximation_project_options
        assimp::assimp
        fmt::fmt
        Qt6::Core Qt6::Concurrent
        Qt6::Quick3D Qt6::Quick Qt6::QuickControls2)
target_include_directories(VolumeApproximation PUBLIC
        include
)

if(WIN32)
  add_custom_command(
          TARGET VolumeApproximation
          POST_BUILD
          COMMAND
          ${CMAKE_COMMAND} $<$<CXX_COMPILER_ID:MSVC>:-E> copy
          $<TARGET_RUNTIME_DLLS:VolumeApproximation>
          $<TARGET_FILE_DIR:VolumeApproximation>
          COMMAND_EXPAND_LISTS)

  get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
  get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
  find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

  add_custom_command(
    TARGET VolumeApproximation
    POST_BUILD
    COMMAND
      "${CMAKE_COMMAND}" -E env PATH="${_qt_bin_dir}"
      "${WINDEPLOYQT_EXECUTABLE}" "$<$<CONFIG:Debug>:--debug>"
      "$<$<CONFIG:Release>:--release>" "--no-compiler-runtime"
      "$<TARGET_FILE:VolumeApproximation>"
    COMMENT "Running windeployqt...")
endif(WIN32)
