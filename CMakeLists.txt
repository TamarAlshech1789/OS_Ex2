cmake_minimum_required(VERSION 3.21)
project(Ex2)

set(CMAKE_CXX_STANDARD 14)

add_library(Ex2 library.cpp definitions.h uthreads.h uthreads.cpp)
