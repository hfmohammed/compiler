# Gazprea Compiler

A native-code compiler for the Gazprea programming language, built in modern
C++ on ANTLR, MLIR, and LLVM. The primary compiler implements a generated
frontend, typed AST, semantic analysis, multi-dialect MLIR generation, LLVM
lowering, runtime support, and an executable test harness.

The implementation follows the official
[CMPUT 415 Gazprea specification](https://cmput415.github.io/415-docs/gazprea/).

## Compiler pipeline

```text
Gazprea source
    ↓
ANTLR 4.13.2 lexer and parser
    ↓
Typed AST and semantic checks
    ↓
MLIR: func + arith + cf + memref
    ↓
5 LLVM dialect conversion passes
    ↓
LLVM IR and optional LLVM -O2 pipeline
    ↓
Clang + Gazprea runtime
    ↓
Native executable
```

## Highlights

- Generates native executables through MLIR and LLVM IR
- Runs LLVM's standard per-module `-O2` optimization pipeline
- Uses an ANTLR 4.13.2 grammar with generated C++ lexer and parser
- Supports integer, real, boolean, character, and nested tuple types
- Implements tuple literals, access, assignment, equality, unpacking, casts,
  parameters, and multi-value returns
- Supports global and block-scoped type aliases with alias chaining and lexical
  shadowing
- Evaluates immutable globals at compile time in dependency order, including
  inferred types, tuple values, casts, comparisons, and checked arithmetic
- Implements functions, procedures, prototypes, by-reference `var` parameters,
  and compile-time aliasing checks
- Supports conditional, pre-test, post-test, infinite, and integer-range loops
- Enforces function purity, procedure call-position rules, const correctness,
  return-path coverage, and the Gazprea `main` contract
- Implements typed stream I/O and spec-compliant `stream_state(std_input)`
- Detects integer overflow, division by zero, invalid powers, invalid literals,
  and other runtime failures with named Gazprea errors

## Validation

The compiler is covered by **240 executable Gazprea programs**. Every positive
test is compiled to LLVM IR with `-O2`, linked to a native executable, run with
an optional input fixture, and checked against exact stdout, stderr, and exit
code expectations.

| Suite | Coverage |
|---|---|
| Language | Operators, casts, scopes, control flow, calls, tuples, aliases, globals, and I/O |
| Compile errors | 147 programs covering syntax, types, symbols, assignments, calls, returns, aliases, globals, and indexing |
| Runtime errors | 14 programs covering overflow, division, modulo, and invalid powers |
| Memory | 4 stress programs covering deep recursion, many locals, nested blocks, and loop allocation |
| Performance | 6 compute-heavy programs with independently checked outputs and 20-second timeouts |
| Edge cases | Integer boundaries, negative powers, floating-point infinity, and NaN |

The harness also inspects optimized LLVM IR to ensure `-O2` performs real
constant folding instead of being accepted as a no-op.

## Build

Requirements:

- CMake 3.20 or newer
- A matching LLVM and MLIR installation
- A C++17 compiler
- Java, used by the ANTLR code generator

On macOS with Homebrew LLVM:

```bash
LLVM_PREFIX=/opt/homebrew/opt/llvm

cmake -S mlir -B mlir/build \
  -DMLIR_DIR="$LLVM_PREFIX/lib/cmake/mlir" \
  -DLLVM_DIR="$LLVM_PREFIX/lib/cmake/llvm" \
  -DCMAKE_CXX_COMPILER="$LLVM_PREFIX/bin/clang++"

cmake --build mlir/build -j
```

CMake downloads the ANTLR 4.13.2 tool and C++ runtime during the first
configuration.

## Use

Emit high-level MLIR:

```bash
mlir/build/gazc --emit-mlir program.gaz
```

Emit LLVM IR:

```bash
mlir/build/gazc --emit-llvm program.gaz -o program.ll
```

Emit LLVM IR after the standard O2 pipeline:

```bash
mlir/build/gazc -O2 --emit-llvm program.gaz -o program.ll
```

Compile and run a native executable:

```bash
mlir/scripts/gazrun.sh program.gaz program
./program
```

Run the complete suite:

```bash
mlir/tests/run_tests.sh
```

## Repository layout

```text
mlir/
├── grammar/Gazprea.g4   ANTLR grammar
├── include/             AST, lowering, and compiler interfaces
├── src/AstBuilder.cpp   Parse-tree to typed AST conversion
├── src/MLIRGen.cpp      Semantic analysis and MLIR generation
├── src/Lowering.cpp     MLIR-to-LLVM lowering and optimization
├── runtime/runtime.c    I/O and checked arithmetic runtime
├── scripts/gazrun.sh    Native compilation driver
└── tests/               240 executable language programs
```

The repository root also retains the earlier direct ARM64 backend as a compact
record of the project's evolution. The MLIR implementation is the primary and
most complete compiler.

## Current scope

The compiler currently does not implement structs, arrays, vectors, strings as
first-class values, matrices, generators, domain expressions, or the
`length`/`rows`/`columns`/`reverse`/`format` built-ins. String literals are
supported for stream output. These limitations are explicit so unsupported
programs are not presented as working language coverage.
