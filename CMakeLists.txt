cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(interpretator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(
    CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wno-unused-parameter -Wno-implicit-fallthrough"
)

file(GLOB sources src/*.cpp src/*.h)

add_executable(interpretator ${sources})
