cmake_minimum_required(VERSION 3.5)

project(LisIf LANGUAGES C)

set(SOURCE_EXE main.c)
set(SOURCE_LIB Pstat.c Errors.c)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(Libs STATIC ${SOURCE_LIB})

add_executable(LisIf ${SOURCE_EXE})

target_link_libraries(LisIf PRIVATE Threads::Threads
                        Libs
                        rt)

