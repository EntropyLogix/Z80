cmake_minimum_required(VERSION 3.14)
cmake_policy(VERSION 3.14)
project(Z80 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -march=native")

# Global options
option(ENABLE_COVERAGE "Enable code coverage generation" OFF)
if(ENABLE_COVERAGE)
    add_compile_options(-g --coverage)
    add_link_options(--coverage)
endif()

option(ENABLE_PGO "Enable Profile-Guided Optimization (PGO)" OFF)
if(ENABLE_PGO)
    if(NOT PGO_MODE)
        message(FATAL_ERROR "PGO_MODE must be set to 'GENERATE' or 'USE' when ENABLE_PGO is ON.")
    endif()
    if("${PGO_MODE}" STREQUAL "GENERATE")
        message(STATUS "PGO: Configuration for Instrumentation (GENERATE profile)")
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            add_compile_options(-fprofile-generate)
            add_link_options(-fprofile-generate)
        endif()
    elseif("${PGO_MODE}" STREQUAL "USE")
        message(STATUS "PGO: Configuration for Optimization (USE profile)")
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            add_compile_options(-fprofile-use -Wno-missing-profile)
            add_link_options(-fprofile-use -Wno-missing-profile)
        endif()
    else()
        message(FATAL_ERROR "Unknown PGO_MODE: ${PGO_MODE}. Use 'GENERATE' or 'USE'.")
    endif()
endif()

# Expose the include directory for all targets
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(tools)
add_subdirectory(tests)

# Install headers
install(DIRECTORY include/Z80 DESTINATION include)

# CPack configuration
set(CPACK_PACKAGE_NAME "Z80")
set(CPACK_PACKAGE_VENDOR "Adam Szulc")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Z80 CPU Emulator Core")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

include(CPack)