cmake_minimum_required(VERSION 3.21)

project(VolumeApproximation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

set (CMAKE_INCLUDE_CURRENT_DIR ON)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(ApproximationLib)
add_subdirectory(UserInterface)
