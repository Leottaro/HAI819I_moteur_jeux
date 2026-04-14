cmake_minimum_required(VERSION 3.20.0)
project(stb)

# Define the stb library
add_library(stb
    stb_image.cpp
    stb_perlin.cpp
)

# Include directories
set(IMGUI_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${IMGUI_INCLUDE_DIRS})
