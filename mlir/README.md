# Gazprea → MLIR compiler

Compiles a subset of [Gazprea](https://cmput415.github.io/415-docs/gazprea/) to
native code through MLIR.

```
source → ANTLR lexer/parser (grammar/Gazprea.g4) → AST → MLIRGen → func/arith/cf/memref
       → (lowering passes) → LLVM dialect → LLVM IR → clang + runtime.c → exe
```

## Build

Requires an LLVM/MLIR install (tested with Homebrew `llvm`), CMake ≥ 3.20 and
Java. CMake downloads the ANTLR 4.13.2 tool jar and C++ runtime on first
configure (network needed) and generates the lexer/parser from the grammar.

```
P=/opt/homebrew/opt/llvm
cmake -S . -B build -DMLIR_DIR=$P/lib/cmake/mlir -DLLVM_DIR=$P/lib/cmake/llvm \
      -DCMAKE_CXX_COMPILER=$P/bin/clang++
cmake --build build
```

## Use

```
build/gazc --emit-mlir prog.gaz             # high-level MLIR
build/gazc --emit-llvm prog.gaz -o p.ll     # LLVM IR (default)
build/gazc -O2 --emit-llvm prog.gaz -o p.ll # LLVM-optimized IR
scripts/gazrun.sh prog.gaz prog && ./prog
tests/run_tests.sh
```

`main` is `procedure main() returns integer`; its result is the exit code.
Compile errors print `SyntaxError`, `TypeError` or `SymbolError` with a line
number and exit non-zero; runtime errors (e.g. division by zero) print
`RuntimeError` from the runtime library.

## Supported

- Types `integer` (i32), `real` (f32), `boolean`, `character`, and `tuple(...)`
  (nested, with `t.1` access, literals, `==`/`!=`, unpacking `a, b = t;`,
  implicit and explicit member-wise casts, tuple parameters and results)
- Global and block-scoped `typealias` declarations, including aliases of aliases
  and supported aggregate types, with lexical shadowing
- Dependency-ordered immutable globals with compile-time evaluation, type
  inference, tuple values, casts, checked arithmetic, and local shadowing
- `const`/`var` (a typed declaration with no qualifier is `const`), inferred
  declarations, `as<T>(e)` casts
- Operators `or xor and == != < > <= >= + - * / % ^ not` with Gazprea precedence;
  integer overflow, division by zero and `0^n` (n ≤ 0) raise `MathError`
- `if`/`else`, `loop`, `loop while`, `loop … while (c);`, `loop i in lo..hi`,
  `break`, `continue`
- Functions (block or `= expr` body) and procedures with `const` (default) and
  `var` (by-reference) parameters, prototypes, `call` statements, the
  procedure-call position rules, and the aliasing check
- Streams: `expr -> std_output;` (string literals allowed), `lvalue <- std_input;`,
  `stream_state(std_input)`
- Spec error names: `SyntaxError SymbolError TypeError AssignError MainError
  ReturnError StatementError CallError DefinitionError AliasingError
  LiteralError IndexError MathError`, reported as `Name on line N: message`

## Not yet implemented

Structs, arrays/vectors/strings/matrices, generators and domain expressions,
`by`/`||`/`**`, `format`/`length`/`rows`/`columns`/`reverse`,
string values (string literals work only as stream output).

## Tests

`tests/run_tests.sh` builds and runs every `tests/*.gaz` that has a `.out` file.
Every native test build passes `-O2` through both the compiler's LLVM pipeline
and the final Clang link, as required by the language specification.

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
