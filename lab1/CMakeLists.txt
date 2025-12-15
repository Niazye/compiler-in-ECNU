cmake_minimum_required(VERSION 3.16)
project(compiler_lab1 LANGUAGES CXX)

# Use C++14 and keep default compiler flags from the environment (e.g., MinGW).
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to a debug build if not specified (helps gdb breakpoints).
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Place all runtime outputs in bin/ under the build tree.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Core library from DFA.cpp
add_library(dfa_lib
    DFA.cpp
)

target_include_directories(dfa_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Test executable linking against the DFA core
add_executable(dfa_test
    dfa_test.cpp
)

target_link_libraries(dfa_test
    PRIVATE
        dfa_lib
)

if(WIN32)
    target_link_libraries(dfa_test PRIVATE psapi)
endif()

# Optional: build main programs if needed
# add_executable(lex_main C_LexAnalysis_mainProcess.cpp)
# target_link_libraries(lex_main PRIVATE dfa_lib)

# add_executable(cons_main cons.c)
# target_link_libraries(cons_main PRIVATE dfa_lib)

# Incremental builds: CMake + your chosen build tool will only recompile
# targets whose sources changed. No extra configuration required.
