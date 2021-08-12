cmake_minimum_required(VERSION 3.21)

project(VolumeApproximation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(ApproximationLib)
add_subdirectory(UserInterface)
