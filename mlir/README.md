# Gazprea → MLIR compiler

Compiles a subset of [Gazprea](https://cmput415.github.io/415-docs/gazprea/) to
native code through MLIR.

```
source → Lexer → Parser → AST → MLIRGen → func/arith/cf/memref
       → (lowering passes) → LLVM dialect → LLVM IR → clang + runtime.c → exe
```

## Build

Requires an LLVM/MLIR install (tested with Homebrew `llvm`) and CMake ≥ 3.20.

```
P=/opt/homebrew/opt/llvm
cmake -S . -B build -DMLIR_DIR=$P/lib/cmake/mlir -DLLVM_DIR=$P/lib/cmake/llvm \
      -DCMAKE_CXX_COMPILER=$P/bin/clang++
cmake --build build
```

## Use

```
build/gazc --emit-mlir prog.gaz          # high-level MLIR
build/gazc --emit-llvm prog.gaz -o p.ll  # LLVM IR (default)
scripts/gazrun.sh prog.gaz prog && ./prog
tests/run_tests.sh
```

`main` is `procedure main() returns integer`; its result is the exit code.
Compile errors print `SyntaxError`, `TypeError` or `SymbolError` with a line
number and exit non-zero; runtime errors (e.g. division by zero) print
`RuntimeError` from the runtime library.

## Supported

- Types `integer` (i32), `real` (f32), `boolean`, `character`; `const`/`var`,
  inferred declarations, integer → real promotion, `as<T>(e)` casts
- Operators `or xor and == != < > <= >= + - * / % ^ not` with Gazprea precedence
- `if`/`else`, `loop`, `loop while`, `loop … while (c);`, `break`, `continue`
- Functions (block or `= expr` body) and procedures, `call` statements
- `expr -> std_output;` and `x <- std_input;`

## Not yet implemented

Tuples, structs, arrays/vectors/strings/matrices, generators, typealias,
globals, prototypes, `-O2` flag handling.
