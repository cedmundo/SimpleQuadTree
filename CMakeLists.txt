cmake_minimum_required(VERSION 3.26)
project(SimpleQuadTree C)

set(CMAKE_C_STANDARD 23)

add_subdirectory(vendor)
add_executable(SimpleQuadTree main.c)
target_link_libraries(SimpleQuadTree raylib)