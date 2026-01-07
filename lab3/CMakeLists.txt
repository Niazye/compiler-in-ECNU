cmake_minimum_required(VERSION 3.10)
project(Lab3Parser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic-errors")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O0 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O0 -g -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -O0 -DNDEBUG")

# LR parser: single translation unit (+ header for IDE visibility)
add_executable(lrparser
    LRparserMain.cpp
    LRparser.h
)
target_include_directories(lrparser PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Keep the single-file example
add_executable(example Example.cpp)

foreach(tgt IN ITEMS lrparser example)
    set_target_properties(${tgt} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    )
endforeach()

target_compile_options(lrparser PRIVATE
    $<$<CONFIG:Release>:-ffast-math>
)

target_compile_options(example PRIVATE
    $<$<CONFIG:Release>:-ffast-math>
)

message(STATUS "Current build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Debug build: using -g -O0")
message(STATUS "Release build: using -O3")
message(STATUS "RelWithDebInfo build: using -O2 -g")
message(STATUS "MinSizeRel build: using -Os")
