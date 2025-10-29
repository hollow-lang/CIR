################################################################
################################################################
## THIS SHIT IS ONLY HERE SO I CAN HAVE INTELLISENSE IN CLION ##
################################################################
################################################################
cmake_minimum_required(VERSION 4.0)
project(CIR)

set(CMAKE_CXX_STANDARD 23)

add_executable(CIR main.cpp
        core/cir.h
        core/config.h)
