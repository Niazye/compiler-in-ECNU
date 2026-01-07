cmake_minimum_required(VERSION 3.10)
project(Lab2Parser LANGUAGES CXX)

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

# 另一个入口：LLparserMain.cpp（可在 LLparser.h/LL.cpp 中实现你的逻辑）
add_executable(llparser
    LLparserMain.cpp
)
target_link_libraries(llparser PRIVATE ll)

# 可选：单文件版本也保留
add_executable(example Example.cpp)

foreach(tgt IN ITEMS llparser example)
    set_target_properties(${tgt} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    )
endforeach()

target_compile_options(llparser PRIVATE
    $<$<CONFIG:Release>:-ffast-math>
)

target_compile_options(example PRIVATE
    $<$<CONFIG:Release>:-ffast-math>
)

message(STATUS "当前构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "调试构建: 使用 -g -O0")
message(STATUS "发布构建: 使用 -O3")
message(STATUS "带调试信息的发布构建: 使用 -O2 -g")
message(STATUS "最小体积发布构建: 使用 -Os")
