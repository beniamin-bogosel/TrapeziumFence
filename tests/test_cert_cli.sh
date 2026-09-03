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
invalid_cert="$tmpdir/trap_small_invalid_theta_$$.jsonl"
invalid_log="$tmpdir/trap_small_invalid_numeric_$$.log"
pair_seed="$tmpdir/trap_pair_seed_$$.jsonl"
pair_out="$tmpdir/trap_pair_output_$$.jsonl"
legacy_pair="$tmpdir/trap_pair_legacy_$$.jsonl"
legacy_roundtrip="$tmpdir/trap_pair_legacy_roundtrip_$$.jsonl"
dynamic="$tmpdir/trap_dynamic_$$.jsonl"
capped="$tmpdir/trap_capped_$$.jsonl"

cleanup() {
    rm -f "$cert" "$refine" "$full" "$missing" "$log" "$wrong" \
          "$assemble_log" "$full_verify_log" "$invalid_cert" "$invalid_log" \
          "$pair_seed" "$pair_out" "$legacy_pair" "$legacy_roundtrip" \
          "$dynamic" "$capped"
}
trap cleanup EXIT HUP INT TERM

expect_rejected() {
    description=$1
    shift
    if ./fence_validate "$@" >"$invalid_log" 2>&1; then
        echo "$description unexpectedly accepted" >&2
        cat "$invalid_log" >&2
        exit 1
    fi
}

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
if ! grep -q '"status":"certified"' "$full"; then
    echo "assembled regression certificate has no certified leaves" >&2
    cat "$assemble_log" >&2
    exit 1
fi
./fence_validate --verify "$full" --prec 70 --serial >"$full_verify_log" 2>&1
if ! grep -q 'AUDIT PASSED' "$full_verify_log"; then
    echo "assembled certificate did not verify" >&2
    cat "$assemble_log" >&2
    cat "$full_verify_log" >&2
    exit 1
fi

expect_rejected "negative theta" --theta -1 --eval 0 0 0 0
expect_rejected "zero theta" --theta 0 --eval 0 0 0 0
expect_rejected "non-finite theta" --theta inf --eval 0 0 0 0
expect_rejected "malformed wfloor" --wfloor 0.1junk --eval 0 0 0 0
expect_rejected "zero wfloor" --wfloor 0 --eval 0 0 0 0
expect_rejected "malformed precision" --prec 70junk --eval 0 0 0 0
expect_rejected "nonpositive precision" --prec 0 --eval 0 0 0 0
expect_rejected "nonpositive max precision" --maxprec 0 --eval 0 0 0 0
expect_rejected "inverted adaptive precision range" --prec 70 --maxprec 60 --adaptive --eval 0 0 0 0
expect_rejected "negative leaf cap" --max-leaves -1 --eval 0 0 0 0
expect_rejected "non-finite evaluation coordinate" --eval nan 0 0 0

sed 's/"theta":"1.0496"/"theta":"-1.0496"/' "$cert" > "$invalid_cert"
if ./fence_validate --verify "$invalid_cert" --prec 70 --serial >"$invalid_log" 2>&1; then
    echo "certificate with negative metadata theta unexpectedly verified" >&2
    exit 1
fi
if ! grep -q 'FAIL\[meta\]' "$invalid_log"; then
    echo "negative metadata theta failure did not report FAIL[meta]" >&2
    cat "$invalid_log" >&2
    exit 1
fi

sed 's/"theta":"1.0496"/"theta":"1e100001"/' "$cert" > "$invalid_cert"
if ./fence_validate --verify "$invalid_cert" --prec 70 --serial >"$invalid_log" 2>&1; then
    echo "certificate with pathological metadata exponent unexpectedly verified" >&2
    exit 1
fi
if ! grep -q 'FAIL\[meta\]' "$invalid_log"; then
    echo "pathological metadata exponent failure did not report FAIL[meta]" >&2
    cat "$invalid_log" >&2
    exit 1
fi

long_theta=12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678
sed "s/\"theta\":\"1.0496\"/\"theta\":\"$long_theta\"/" "$cert" > "$invalid_cert"
if ./fence_validate --verify "$invalid_cert" --prec 70 --serial >"$invalid_log" 2>&1; then
    echo "certificate with oversized metadata theta unexpectedly verified" >&2
    exit 1
fi
if ! grep -q 'FAIL\[meta\]' "$invalid_log"; then
    echo "oversized metadata theta failure did not report FAIL[meta]" >&2
    cat "$invalid_log" >&2
    exit 1
fi

# An uncapped root-leaf run exercises idle-worker termination with a dynamic
# OpenMP team (and remains valid in a serial build).  A timeout turns a
# regression into a bounded test failure rather than a hanging test suite.
if ! timeout 10s env OMP_DYNAMIC=TRUE OMP_NUM_THREADS=128 \
    ./fence_validate --theta 1.0496 --wfloor 1 --prec 70 --form centered \
    --out "$dynamic" >/dev/null; then
    echo "dynamic-team search did not terminate" >&2
    exit 1
fi
if [ "$(wc -l < "$dynamic")" -ne 2 ]; then
    echo "dynamic-team root-leaf run did not produce one leaf plus metadata" >&2
    exit 1
fi

# The debugging leaf cap must also be exact in serial and OpenMP builds.
./fence_validate --theta 1.0496 --wfloor 0.25 --prec 70 --form centered \
    --flat-area-cert --pair-eq-cert --max-leaves 5 --out "$capped" >/dev/null
if [ "$(wc -l < "$capped")" -ne 6 ]; then
    echo "--max-leaves 5 did not produce exactly five leaves plus metadata" >&2
    exit 1
fi

printf '%s\n' \
    '{"box":[[0.8,0.8],[0.6,0.6],[0.2,0.2],[0.7,0.7]],"status":"survivor","item":-1,"encl":[null,null],"prec":70}' \
    > "$pair_seed"
./fence_validate --theta 1.0496 --wfloor 1 --prec 70 --form natural --serial \
    --pair-eq-cert --refine "$pair_seed" --out "$pair_out" >/dev/null 2>"$invalid_log"
if ! grep -q '"status":"pair_incompatible"' "$pair_out"; then
    echo "new pair-equality exclusion status was not emitted" >&2
    cat "$invalid_log" >&2
    exit 1
fi
if ! grep -q '"status":"pair_incompatible".*"encl":\[null,null\]' "$pair_out"; then
    echo "pair-incompatible leaf wrote a misleading numerical enclosure" >&2
    cat "$pair_out" >&2
    exit 1
fi
sed 's/"status":"pair_incompatible"/"status":"nonoptimal"/' \
    "$pair_out" > "$legacy_pair"
if ! ./fence_validate --assemble "$legacy_pair" --out "$legacy_roundtrip" \
    >/dev/null 2>"$invalid_log"; then
    echo "legacy nonoptimal status was not accepted by the certificate parser" >&2
    cat "$invalid_log" >&2
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
