cmake_minimum_required(VERSION 3.10)
project(Z80Tools)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create the executable target
add_executable(Z80Tool Z80Tool.cpp)

# Add the parent directory to include paths to find Z80.h, Z80Analyze.h, etc.
target_include_directories(Z80Tool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)