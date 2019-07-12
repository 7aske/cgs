cmake_minimum_required(VERSION 3.14)
project(cgs C)

set(CMAKE_C_STANDARD 99)

add_executable(cgs cgs.c)
target_link_libraries(cgs pthread)