cmake_minimum_required(VERSION 3.10)
project(CIR)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -flto")

add_executable(CIR main.cpp
        core/cir.h
        core/config.h)
