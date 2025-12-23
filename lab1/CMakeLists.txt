cmake_minimum_required(VERSION 3.10)
project(LexAnalyzerProject LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 根据构建类型设置不同的编译选项
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

# 设置所有构建类型的公共选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic-errors")

# 为不同构建类型设置特定选项
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O0 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O0 -g -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -O0 -DNDEBUG")

# 创建静态库，包含 DFA 类的实现
add_library(DFALib STATIC DFA.cpp)

# 设置头文件包含目录
target_include_directories(DFALib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 创建第一个可执行文件（dfa_test）
add_executable(dfa_test dfa_test.cpp)
target_link_libraries(dfa_test DFALib)

# 创建第二个可执行文件（C_LexAnalysis_mainProcess）
add_executable(lex_analysis C_LexAnalysis_mainProcess.cpp)
target_link_libraries(lex_analysis DFALib)

# 将关键字文件复制到运行目录，便于运行时直接找到
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/c_keys.txt
               ${CMAKE_BINARY_DIR}/bin/c_keys.txt COPYONLY)

# 为每个目标设置输出目录
set_target_properties(dfa_test lex_analysis PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# 为 Release 构建设置更激进的优化选项（可选）
target_compile_options(dfa_test PRIVATE
    $<$<CONFIG:Release>:-ffast-math>
)
target_compile_options(lex_analysis PRIVATE
    $<$<CONFIG:Release>:-ffast-math>
)

# 提供构建类型的说明
message(STATUS "当前构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "调试构建: 使用 -g -O0")
message(STATUS "发布构建: 使用 -O3")
message(STATUS "带调试信息的发布构建: 使用 -O2 -g")
message(STATUS "最小体积发布构建: 使用 -Os")