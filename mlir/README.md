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

## Tests

`tests/run_tests.sh` builds and runs every `tests/*.gaz` that has a `.out` file.

| Prefix | What it checks |
|---|---|
| (none) | language coverage: operators, precedence, casts, scopes, control flow, functions, I/O |
| `err_*` | compile errors; `.out` is `ERR <Kind>` and the error kind must match |
| `rt_*` | runtime errors: exit code 1, message in `.stderr`, earlier output still flushed |
| `edge_*` | defined behaviour at limits (`INT_MIN / -1`, negative powers, inf/NaN) |
| `mem_*` | stack safety: loop-body declarations do not grow the stack, deep recursion, many locals |
| `perf_*` | compute-heavy programs (20 s timeout each); expected values computed independently |

Optional per-test files: `.in` (stdin), `.code` (expected exit code, default 0),
`.stderr` (substring the runtime error must contain).

Note: `real` is 32-bit, so e.g. summing 0.5 stops growing at 2^23, and
`integer` arithmetic wraps on overflow.
