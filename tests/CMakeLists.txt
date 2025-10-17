cmake_minimum_required(VERSION 3.14)
cmake_policy(VERSION 3.14)
project(Z80 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -march=native")

include(FetchContent)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
  z80_tests
  GIT_REPOSITORY https://github.com/SingleStepTests/z80.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(z80_tests)

add_executable(json-tests
    json-tests.cpp
)
set_target_properties(json-tests PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_link_libraries(json-tests PRIVATE nlohmann_json::nlohmann_json)
target_compile_definitions(json-tests PRIVATE Z80_TESTS_DIR="${z80_tests_SOURCE_DIR}/v1")
target_include_directories(json-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(zex-tests
    zex-tests.cpp
)
set_target_properties(zex-tests PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_link_libraries(zex-tests PRIVATE nlohmann_json::nlohmann_json)
target_include_directories(zex-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

option(ENABLE_PGO "Enable Profile-Guided Optimization (PGO)" OFF)
if(ENABLE_PGO)
    if(NOT PGO_MODE)
        message(FATAL_ERROR "PGO_MODE must be set to 'GENERATE' or 'USE' when ENABLE_PGO is ON.")
    endif()
    if("${PGO_MODE}" STREQUAL "GENERATE")
        message(STATUS "PGO: Configuration for Instrumentation (GENERATE profile)")
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            set(PGO_FLAGS "-fprofile-generate")
        endif()
    elseif("${PGO_MODE}" STREQUAL "USE")
        message(STATUS "PGO: Configuration for Optimization (USE profile)")
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            set(PGO_FLAGS "-fprofile-use" "-Wno-missing-profile") 
        endif()
    else()
        message(FATAL_ERROR "Unknown PGO_MODE: ${PGO_MODE}. Use 'GENERATE' or 'USE'.")
    endif()
    target_compile_options(json-tests PUBLIC ${PGO_FLAGS})
    target_link_options(json-tests PUBLIC ${PGO_FLAGS})
    target_compile_options(zex-tests PUBLIC ${PGO_FLAGS})
    target_link_options(zex-tests PUBLIC ${PGO_FLAGS})
endif()