#!/bin/bash
# Compile a .gaz file to a native executable: gazrun.sh <input.gaz> <output-exe>
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
if [ -z "${CLANG:-}" ]; then
  if [ -x /opt/homebrew/opt/llvm/bin/clang ]; then
    CLANG=/opt/homebrew/opt/llvm/bin/clang
  elif [ -x /usr/local/opt/llvm/bin/clang ]; then
    CLANG=/usr/local/opt/llvm/bin/clang
  else
    CLANG=clang
  fi
fi
"$ROOT/build/gazc" -O2 --emit-llvm "$1" -o "$2.ll"
"$CLANG" -Wno-override-module -O2 "$2.ll" "$ROOT/runtime/runtime.c" -lm -o "$2"
rm -f "$2.ll"
