cmake_minimum_required(VERSION 3.5)

project(LisIf_mon LANGUAGES C)

add_executable(LisIf_mon main.c)

target_link_libraries(LisIf_mon rt)
