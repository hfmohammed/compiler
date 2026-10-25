#!/bin/bash
# For each tests/*.gaz: compile, run (stdin from .in if present), compare stdout
# with .out, and the exit code with .code (default 0). A .stderr file holds a
# substring the runtime error must contain. Programs whose .out is "ERR <Kind>" must fail to compile with that error kind.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"
pass=0; fail=0
for src in "$ROOT"/tests/*.gaz; do
  name="$(basename "$src" .gaz)"
  exp="$ROOT/tests/$name.out"
  [ -f "$exp" ] || continue
  if head -c 3 "$exp" | grep -q '^ERR'; then
    want="$(cut -d' ' -f2 "$exp")"
    if "$ROOT/build/gazc" "$src" -o /dev/null 2>"$TMP/err"; then
      echo "FAIL $name (expected compile error)"; fail=$((fail+1))
    elif ! head -1 "$TMP/err" | grep -q "^$want"; then
      echo "FAIL $name (expected $want, got: $(head -1 "$TMP/err"))"; fail=$((fail+1))
    else echo "ok   $name"; pass=$((pass+1)); fi
    continue
  fi
  if ! "$ROOT/scripts/gazrun.sh" "$src" "$TMP/$name" 2>"$TMP/err"; then
    echo "FAIL $name (compile)"; cat "$TMP/err"; fail=$((fail+1)); continue
  fi
  inp=/dev/null; [ -f "$ROOT/tests/$name.in" ] && inp="$ROOT/tests/$name.in"
  perl -e 'alarm 20; exec @ARGV' "$TMP/$name" <"$inp" >"$TMP/out" 2>"$TMP/rterr"
  code=$?
  want=0; [ -f "$ROOT/tests/$name.code" ] && want="$(cat "$ROOT/tests/$name.code")"
  if [ "$code" != "$want" ]; then
    echo "FAIL $name (exit $code, expected $want)"; fail=$((fail+1)); continue
  fi
  if [ -f "$ROOT/tests/$name.stderr" ] && ! grep -qF -f "$ROOT/tests/$name.stderr" "$TMP/rterr"; then
    echo "FAIL $name (stderr)"; fail=$((fail+1)); continue
  fi
  if cmp -s "$TMP/out" "$exp"; then echo "ok   $name"; pass=$((pass+1))
  else echo "FAIL $name"; diff "$TMP/out" "$exp" | head -5; fail=$((fail+1)); fi
done
rm -rf "$TMP"
echo "$pass passed, $fail failed"
[ $fail -eq 0 ]
