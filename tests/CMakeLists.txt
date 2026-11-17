find_package(GTest)
if(NOT GTest_FOUND)
  message(WARNING "GoogleTest not found: tests are disabled. Install it, or pass -DBUILD_TESTING=OFF to silence this.")
  return()
endif()
include(GoogleTest)

add_executable(
  ApproximationLibTests
  TestHelpers.hpp
  Vector3Tests.cpp
  RayIntersectionTests.cpp
  CpuBackendTests.cpp
  # Same sanitizer runtime defaults as the app (ASan + CUDA needs protect_shadow_gap=0)
  ${PROJECT_SOURCE_DIR}/UserInterface/SanitizerDefaults.cpp)

if(CUDA_ENABLED)
  # Plain C++ test code that calls the CUDA backend through the library;
  # it skips itself at runtime when no GPU is usable
  target_sources(ApproximationLibTests PRIVATE CudaBackendTests.cpp)
endif()

target_link_libraries(
  ApproximationLibTests
  PRIVATE ApproximationLib
          GTest::gtest_main
          VolumeApproximation_project_options
          VolumeApproximation_project_warnings
          VolumeApproximation_sanitizers)

# PRE_TEST: list tests when ctest runs, not at build time (keeps builds working when
# the test binary can't run on the build machine, e.g. under TSan or without a GPU)
gtest_discover_tests(ApproximationLibTests DISCOVERY_MODE PRE_TEST)
