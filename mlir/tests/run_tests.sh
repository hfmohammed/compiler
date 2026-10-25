#!/bin/bash
# For each tests/*.gaz: compile, run (stdin from .in if present), compare stdout
# with .out. Programs whose .out starts with "ERR" must fail to compile.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"
pass=0; fail=0
for src in "$ROOT"/tests/*.gaz; do
  name="$(basename "$src" .gaz)"
  exp="$ROOT/tests/$name.out"
  [ -f "$exp" ] || continue
  if head -c 3 "$exp" | grep -q '^ERR'; then
    if "$ROOT/build/gazc" "$src" -o /dev/null 2>/dev/null; then
      echo "FAIL $name (expected compile error)"; fail=$((fail+1))
    else echo "ok   $name"; pass=$((pass+1)); fi
    continue
  fi
  if ! "$ROOT/scripts/gazrun.sh" "$src" "$TMP/$name" 2>"$TMP/err"; then
    echo "FAIL $name (compile)"; cat "$TMP/err"; fail=$((fail+1)); continue
  fi
  inp=/dev/null; [ -f "$ROOT/tests/$name.in" ] && inp="$ROOT/tests/$name.in"
  "$TMP/$name" <"$inp" >"$TMP/out" 2>/dev/null
  if cmp -s "$TMP/out" "$exp"; then echo "ok   $name"; pass=$((pass+1))
  else echo "FAIL $name"; diff "$TMP/out" "$exp" | head -5; fail=$((fail+1)); fi
done
rm -rf "$TMP"
echo "$pass passed, $fail failed"
[ $fail -eq 0 ]
