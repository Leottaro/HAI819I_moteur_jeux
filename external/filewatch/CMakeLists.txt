cmake_minimum_required(VERSION 3.20.0)
project(filewatch)

# Define the imgui library
add_library(filewatch
    FileWatch.hpp
    FileWatch.cpp
)

# Include directories
set(FILEWATCH_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${FILEWATCH_INCLUDE_DIRS})
