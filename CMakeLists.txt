cmake_minimum_required(VERSION 3.6)
project(compiler)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")

set(SOURCE_FILES compile.c compile.c)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ".")

add_executable(compiler ${SOURCE_FILES})