cmake_minimum_required(VERSION 3.10)
project(Z80Tools)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
add_executable(Z80Dump Z80Dump.cpp)
add_executable(Z80Asm Z80Asm.cpp)