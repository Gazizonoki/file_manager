cmake_minimum_required(VERSION 3.15)

project(file_manager)

set(CMAKE_C_STANDARD 11 REQUIRED)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

add_library(events events.c)
add_executable(list main.c)

target_link_libraries(list events ${CURSES_LIBRARIES})
