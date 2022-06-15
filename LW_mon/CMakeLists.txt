cmake_minimum_required(VERSION 3.5)

project(LW_mon LANGUAGES C)

add_executable(LW_mon main.c)

target_link_libraries(LW_mon rt)
