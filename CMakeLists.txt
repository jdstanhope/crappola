cmake_minimum_required(VERSION 3.10)
project(crappola C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags for macOS
if(APPLE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(COMPILER_SOURCES
    src/main.c
    src/lexer.c
    src/parser.c
    src/preprocessor.c
    src/codegen.c
    src/linker.c
)

# Main executable
add_executable(crappola ${COMPILER_SOURCES})

# Installation
install(TARGETS crappola DESTINATION bin)
