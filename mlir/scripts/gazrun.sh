#!/bin/bash
# Compile a .gaz file to a native executable: gazrun.sh <input.gaz> <output-exe>
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CLANG="${CLANG:-clang}"
"$ROOT/build/gazc" --emit-llvm "$1" -o "$2.ll"
"$CLANG" -Wno-override-module -O1 "$2.ll" "$ROOT/runtime/runtime.c" -lm -o "$2"
rm -f "$2.ll"
