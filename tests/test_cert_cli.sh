#!/bin/sh
set -eu

tmpdir="${TMPDIR:-/tmp}"
cert="$tmpdir/trap_small_$$.jsonl"
refine="$tmpdir/trap_small_refine_$$.jsonl"
full="$tmpdir/trap_small_full_$$.jsonl"
missing="$tmpdir/trap_small_missing_$$.jsonl"
log="$tmpdir/trap_small_missing_$$.log"
wrong="$tmpdir/trap_small_wrong_$$.log"
assemble_log="$tmpdir/trap_small_assemble_$$.log"
full_verify_log="$tmpdir/trap_small_full_verify_$$.log"

cleanup() {
    rm -f "$cert" "$refine" "$full" "$missing" "$log" "$wrong" \
          "$assemble_log" "$full_verify_log"
}
trap cleanup EXIT HUP INT TERM

common="--theta 1.0496 --prec 70 --form centered --serial --flat-area-cert --pair-eq-cert"

./fence_validate $common --wfloor 0.5 --out "$cert" >/dev/null
if ! grep -q '"schema":3,"theta":"1.0496"' "$cert"; then
    echo "certificate metadata does not record schema-3 exact theta" >&2
    exit 1
fi
./fence_validate --verify "$cert" --prec 70 --serial >/dev/null
./fence_validate --verify "$cert" --theta 1.049600 --prec 70 --serial >/dev/null

./fence_validate $common --wfloor 0.25 --refine "$cert" --out "$refine" >/dev/null
./fence_validate --assemble "$cert" "$refine" --out "$full" >"$assemble_log" 2>&1
./fence_validate --verify "$full" --prec 70 --serial >"$full_verify_log" 2>&1
if ! grep -q 'AUDIT PASSED' "$full_verify_log"; then
    echo "assembled certificate did not verify" >&2
    cat "$assemble_log" >&2
    cat "$full_verify_log" >&2
    exit 1
fi

if ./fence_validate --verify "$cert" --theta 1.50 --prec 70 --serial >"$wrong" 2>&1; then
    echo "wrong-theta certificate unexpectedly verified" >&2
    exit 1
fi

if ! grep -q 'FAIL\[meta\]' "$wrong"; then
    echo "wrong-theta verifier failure did not report FAIL[meta]" >&2
    cat "$wrong" >&2
    exit 1
fi

sed '2d' "$cert" > "$missing"
if ./fence_validate --verify "$missing" $common >"$log" 2>&1; then
    echo "missing-leaf certificate unexpectedly verified" >&2
    exit 1
fi

if ! grep -q 'FAIL\[cover\]' "$log"; then
    echo "missing-leaf verifier failure did not report FAIL[cover]" >&2
    cat "$log" >&2
    exit 1
fi

echo "certificate CLI regression tests passed"
