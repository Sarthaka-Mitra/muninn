cmake_minimum_required(VERSION 3.16)
project(muninn VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# So #include "something.h" works from anywhere in the project
include_directories(include)

# Automatically picks up all .cpp files in src/
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(muninn ${SOURCES})

