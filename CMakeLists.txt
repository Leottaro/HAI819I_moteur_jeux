cmake_minimum_required(VERSION 3.20.0)
project(hai819i_moteur_jeux)
set(APP_VERSION_MAJOR 1)
set(APP_VERSION_MINOR 0)

set(CMAKE_CXX_STANDARD 23)
set(OpenGL_GL_PREFERENCE GLVND) # asked me to set this to GLVND or LEGACY
# set(CMAKE_VERBOSE_MAKEFILE 1) # If you want verbose

# target setting
set(APP_TARGET hai819i_moteur_jeux)
set(APP_TARGET_DEBUG ${APP_TARGET}_debug)
set(APP_TARGET_OPT ${APP_TARGET}_opt)
include_directories(${PROJECT_SOURCE_DIR} .)

# my src files
set(APP_SOURCES
    src/main.cpp

    src/WorldRenderer.hpp
    src/WorldRenderer.cpp

    src/World.hpp
    src/World.cpp

    src/Camera.hpp

    src/ECS/ECS.hpp
    src/ECS/Data.hpp
    src/ECS/Entity.hpp
    src/ECS/Component.hpp
    src/ECS/Systems.hpp

    src/Chunk.hpp
    src/Chunk.cpp

    src/Block.hpp
    src/objects/blocks.hpp

    src/objects/structures.hpp

    src/Transformation.hpp
    src/AABB.hpp

    src/Texture.hpp
    src/objects/textures.hpp

    src/Window.hpp

    src/GLGlobalContext.hpp
    src/ShaderProgram.hpp
    src/ShaderProgram.cpp

    src/Helpers.hpp
)

# resources
file(GLOB APP_RESOURCES "ressources/**/*")

add_executable(${APP_TARGET_DEBUG} ${APP_SOURCES} ${APP_RESOURCES})
add_executable(${APP_TARGET_OPT} ${APP_SOURCES} ${APP_RESOURCES})

target_compile_options(${APP_TARGET_DEBUG} PRIVATE -Wall -O0 -g3)
target_compile_definitions(${APP_TARGET_DEBUG} PRIVATE DEBUG)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${APP_TARGET_OPT} PRIVATE
        -Wall
        -O3
        # -Ofast
        -march=native
        -mtune=native
        # -flto
        -funroll-loops
        # -ffast-math
        -DNDEBUG
    )
    target_link_options(${APP_TARGET_OPT} PRIVATE)
else()
    target_compile_options(${APP_TARGET_DEBUG} PRIVATE -Wall -O3 -DNDEBUG)
endif()

# opengl
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})
target_link_libraries(${APP_TARGET_DEBUG} OpenGL::GL)
target_link_libraries(${APP_TARGET_OPT} OpenGL::GL)

# glew
add_subdirectory(external/glew/build/cmake)
include_directories(external/glew/include)
target_link_libraries(${APP_TARGET_DEBUG} glew)
target_link_libraries(${APP_TARGET_OPT} glew)

# glfw
add_subdirectory(external/glfw)
include_directories(external/glfw/include)
target_link_libraries(${APP_TARGET_DEBUG} glfw)
target_link_libraries(${APP_TARGET_OPT} glfw)

# imgui
add_subdirectory(external/imgui)
include_directories(external/imgui)
target_link_libraries(imgui PRIVATE glfw)
target_link_libraries(${APP_TARGET_DEBUG} imgui)
target_link_libraries(${APP_TARGET_OPT} imgui)

# glm
add_subdirectory(external/glm)
include_directories(external/glm)
target_link_libraries(${APP_TARGET_DEBUG} glm)
target_link_libraries(${APP_TARGET_OPT} glm)

# stb
add_subdirectory(external/stb)
include_directories(external/stb)
target_link_libraries(${APP_TARGET_DEBUG} stb)
target_link_libraries(${APP_TARGET_OPT} stb)
