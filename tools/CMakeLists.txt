cmake_minimum_required(VERSION 3.15)
project(Z80Tools)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
  replxx
  GIT_REPOSITORY https://github.com/AmokHuginnsson/replxx.git
  GIT_TAG        release-0.0.4 
)

FetchContent_MakeAvailable(replxx)

add_executable(Z80Tool Z80Tool.cpp)
target_include_directories(Z80Tool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_include_directories(Z80Tool PRIVATE ${replxx_SOURCE_DIR}/include)
target_link_libraries(Z80Tool PRIVATE replxx)
