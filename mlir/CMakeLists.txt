cmake_minimum_required(VERSION 3.20)
project(gazc LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(MLIR REQUIRED CONFIG)
message(STATUS "Using MLIRConfig.cmake in: ${MLIR_DIR}")

include_directories(${LLVM_INCLUDE_DIRS} ${MLIR_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)
add_definitions(${LLVM_DEFINITIONS})

add_library(gazrt STATIC runtime/runtime.c)

add_executable(gazc
  src/Lexer.cpp
  src/main.cpp)
