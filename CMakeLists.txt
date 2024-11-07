cmake_minimum_required(VERSION 3.29)
project(musicplayer C)

set(CMAKE_C_STANDARD 99)

# Define Raylib directories based on your setup
set(RAYLIB_INCLUDE_DIR "C:/raylib/raylib/src")
set(RAYLIB_LIB_DIR "C:/raylib/raylib/src")
set(COMPILER_DIR "C:/raylib/w64devkit/bin")

# Add Raylib include directory
include_directories(${RAYLIB_INCLUDE_DIR})

# Link Raylib library files
link_directories(${RAYLIB_LIB_DIR})

# Add executable
add_executable(musicplayer main.c)
target_link_libraries(musicplayer raylib)

# Add necessary libraries for Windows
if (WIN32)
    target_link_libraries(musicplayer winmm gdi32)
endif()
