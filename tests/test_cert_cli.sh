#!/bin/sh
set -eu

tmpdir="${TMPDIR:-/tmp}"
cert="$tmpdir/trap_small_$$.jsonl"
missing="$tmpdir/trap_small_missing_$$.jsonl"
log="$tmpdir/trap_small_missing_$$.log"

cleanup() {
    rm -f "$cert" "$missing" "$log"
}
trap cleanup EXIT HUP INT TERM

common="--theta 1.0496 --prec 70 --form centered --serial --flat-area-cert --pair-eq-cert"

./fence_validate $common --wfloor 0.5 --out "$cert" >/dev/null
./fence_validate --verify "$cert" $common >/dev/null

sed '1d' "$cert" > "$missing"
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
