cmake_minimum_required(VERSION 3.20)
project(gazc LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(MLIR REQUIRED CONFIG)
message(STATUS "Using MLIRConfig.cmake in: ${MLIR_DIR}")

# ---- ANTLR: tool jar + C++ runtime, fetched at configure time ----
find_package(Java REQUIRED COMPONENTS Runtime)
set(ANTLR_VERSION 4.13.2)
set(ANTLR_JAR ${CMAKE_BINARY_DIR}/antlr-${ANTLR_VERSION}-complete.jar)
if(NOT EXISTS ${ANTLR_JAR})
  file(DOWNLOAD https://www.antlr.org/download/antlr-${ANTLR_VERSION}-complete.jar
       ${ANTLR_JAR} SHOW_PROGRESS)
endif()

include(FetchContent)
set(ANTLR_BUILD_CPP_TESTS OFF CACHE BOOL "" FORCE)
set(ANTLR_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(DISABLE_WARNINGS ON CACHE BOOL "" FORCE)
FetchContent_Declare(antlr4
  URL https://github.com/antlr/antlr4/archive/refs/tags/${ANTLR_VERSION}.tar.gz
  SOURCE_SUBDIR runtime/Cpp)
FetchContent_MakeAvailable(antlr4)

set(GEN_DIR ${CMAKE_BINARY_DIR}/generated)
set(GEN_SRCS ${GEN_DIR}/GazpreaLexer.cpp ${GEN_DIR}/GazpreaParser.cpp)
add_custom_command(
  OUTPUT ${GEN_SRCS}
  COMMAND ${Java_JAVA_EXECUTABLE} -jar ${ANTLR_JAR} -Dlanguage=Cpp -no-listener
          -no-visitor -Xexact-output-dir -o ${GEN_DIR}
          ${CMAKE_SOURCE_DIR}/grammar/Gazprea.g4
  DEPENDS ${CMAKE_SOURCE_DIR}/grammar/Gazprea.g4
  COMMENT "Generating lexer/parser from Gazprea.g4")

include_directories(${LLVM_INCLUDE_DIRS} ${MLIR_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/include ${GEN_DIR}
  ${antlr4_SOURCE_DIR}/runtime/Cpp/runtime/src)
add_definitions(${LLVM_DEFINITIONS})

add_library(gazrt STATIC runtime/runtime.c)

add_executable(gazc
  ${GEN_SRCS}
  src/AstBuilder.cpp
  src/MLIRGen.cpp
  src/Lowering.cpp
  src/main.cpp)
# Generated ANTLR code is not warning-clean under -Wall.
set_source_files_properties(${GEN_SRCS} PROPERTIES COMPILE_FLAGS -w)

target_link_libraries(gazc PRIVATE
  antlr4_static
  MLIRArithDialect
  MLIRControlFlowDialect
  MLIRFuncDialect
  MLIRMemRefDialect
  MLIRIR
  MLIRPass
  MLIRLLVMDialect
  MLIRArithToLLVM
  MLIRControlFlowToLLVM
  MLIRFuncToLLVM
  MLIRMemRefToLLVM
  MLIRReconcileUnrealizedCasts
  MLIRBuiltinToLLVMIRTranslation
  MLIRLLVMToLLVMIRTranslation
  MLIRTargetLLVMIRExport)
