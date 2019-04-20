
cmake_minimum_required(VERSION 3.9)
project(test1)
add_executable(test main.cpp)

#specified compiler
SET(CMAKE_C_COMPILER  "/usr/local/bin/gcc")
SET(CMAKE_CXX_COMPILER "/usr/local/bin/g++")
