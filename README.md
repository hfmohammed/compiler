# Gazprea ARM64 Code Generator

This project implements the code generation backend for a Gazprea style programming language. It takes an abstract syntax tree produced by the parser and emits ARM64 (AArch64) assembly. The design focuses on correctness, explicit stack management, and clear control flow rather than aggressive optimization.

The generator supports expressions, scoped variables, control flow, loops, functions, procedures, and function calls following the ARM64 calling convention.

## Overview

The compilation pipeline is structured as follows:

Source Code -> Tokenizer -> Parser -> AST -> Code Generator -> ARM64 Assembly -> Native Binary

The generated assembly is written to `output.s`.

## Features

* Two pass code generation for accurate stack sizing
* Stack based allocation for locals and temporaries
* Lexical scoping with shadowing
* Functions and procedures with parameters
* Function calls using ARM64 argument registers
* Safe handling of nested function calls
* Control flow including if, else, loops, break, and continue
* ARM64 compliant stack frames and alignment

## Two Pass Code Generation

Each function and the main program are generated using two passes.

### Pass One: Frame Size Calculation

In the first pass, no assembly is emitted. The generator walks the AST and tracks local variable usage and temporary stack usage during expression evaluation. This pass determines the total stack frame size needed. The final frame size is aligned to 16 bytes as required by the ARM64 ABI.

### Pass Two: Assembly Emission

In the second pass, the generator emits the function prologue, parameter storage, body instructions, and function epilogue. This ensures all stack accesses are valid and correctly aligned.

## Stack Frame Layout

Each function uses a fixed stack frame with a frame pointer stored in x29.

The frame layout is:

* &emsp; Saved frame pointer and return address
* &emsp; Local variables and parameters
* &emsp; Temporary values used during expression evaluation

All variables are accessed using fixed offsets from the frame pointer.

## Function Parameters

### Calling Convention

The first eight integer or pointer arguments are passed in registers x0 through x7.

### Parameter Storage

On function entry, each parameter is assigned a stack slot and the incoming register value is stored into that slot. After this step, parameters behave exactly like local variables.

### Parameter Access

When a parameter is referenced, it is loaded from its stack slot using its frame pointer relative offset. This unifies parameter and local variable access.

## Function Calls

Arguments are evaluated from left to right. To safely support nested function calls, arguments are temporarily spilled to the stack before the call. After all arguments are evaluated, they are reloaded into x0 through x7 and the function is called. This prevents nested calls from overwriting argument registers.

## Functions and Procedures

The generator distinguishes between two kinds of routines.

Functions return a value in x0 and must explicitly return or use an expression body.

Procedures do not return a value. They may omit a return statement. If no return is encountered, the generator automatically emits a function epilogue.

## Control Flow

Supported control flow constructs include:
* if, else if, else
* while loops
* post predicated loops
* infinite loops
* break and continue

Each loop maintains its own label context to ensure correct jump targets.

## Build System

This project uses a Makefile to drive the full compiler pipeline.

The Makefile performs the following steps:

1. Compiles the compiler implementation
2. Runs the compiler on a Gazprea source file
3. Generates ARM64 assembly in `output.s`
4. Assembles and links the generated assembly
5. Executes the resulting binary

### Makefile Targets

* `make` or `make run` Builds the compiler, runs it on the test input, assembles the output, and executes the program
* `make compile` Compiles the compiler source into an executable
* `make link` Runs the compiler on the test Gazprea file to produce `output.s`
* `make run_asm` Assembles and runs an existing `output.s` file without recompiling the compiler
* `make clean` Removes generated binaries

The Makefile is intended for rapid iteration and debugging during compiler development.

## Files

* `src/tokenization.hpp` Token definitions and lexical utilities
* `src/parser.hpp` AST definitions and parsing logic
* `src/generator.hpp` ARM64 code generation backend
* `src/main.cpp` Compiler entry point
* `src/example.gaz` Example and test file
* `Makefile` Build and execution automation
* `grammar.md` defines grammar

## Documentation

- 📘 [Language Grammar](grammar.md)
- 🧠 Parser & AST Design (coming soon)

## Limitations and Future Work

* Support for more than eight function arguments
* Struct layout and member access
* Type driven code generation
* Register allocation beyond scratch registers
* Optimization passes such as constant folding and dead code elimination

## Summary

This project implements a clear and explicit ARM64 code generator with proper stack discipline, function handling, and control flow. It is designed to be easy to debug and extend, making it a solid foundation for further compiler development.
