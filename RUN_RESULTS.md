# Recorded Results

Date: 2026-07-28

Note: current certificates are self-describing schema-3 files.  A JSONL
certificate begins with a metadata line recording exact decimal `theta`,
`form`, `half`, auxiliary flags, precision provenance, and the root box.
`--verify` reads those values from the file; passing `--theta`, `--form`, or
`--half` during verification is now only a consistency check.  Certificates
generated before schema 3 should be regenerated before auditing with the
current verifier.

Folder:

```sh
/home/beni/python/TrapeziumFence
```

Build:

```sh
make OMP=0
```

All runs used:

```sh
--theta 1.0496 --prec 70 --form centered --serial --flat-area-cert --pair-eq-cert
```

## Commands

```sh
./fence_validate --theta 1.0496 --wfloor 0.025 --prec 70 \
  --form centered --serial --flat-area-cert --pair-eq-cert \
  --out flat_pair_cert_w0025.jsonl --stats

./fence_validate --theta 1.0496 --wfloor 0.0125 --prec 70 \
  --form centered --serial --flat-area-cert --pair-eq-cert \
  --refine flat_pair_cert_w0025.jsonl \
  --out flat_pair_refine_w00125.jsonl --stats

./fence_validate --theta 1.0496 --wfloor 0.00625 --prec 70 \
  --form centered --serial --flat-area-cert --pair-eq-cert \
  --refine flat_pair_refine_w00125.jsonl \
  --out flat_pair_refine_w000625.jsonl --stats

./fence_validate --theta 1.0496 --wfloor 0.003125 --prec 70 \
  --form centered --serial --flat-area-cert --pair-eq-cert \
  --refine flat_pair_refine_w000625.jsonl \
  --out flat_pair_refine_w0003125.jsonl --stats

./fence_validate --theta 1.0496 --wfloor 0.0015625 --prec 70 \
  --form centered --serial --flat-area-cert --pair-eq-cert \
  --refine flat_pair_refine_w0003125.jsonl \
  --out flat_pair_refine_w00015625.jsonl --stats

./fence_validate --theta 1.0496 --wfloor 0.00078125 --prec 70 \
  --form centered --serial --flat-area-cert --pair-eq-cert \
  --refine flat_pair_refine_w00015625.jsonl \
  --out flat_pair_refine_w000078125.jsonl --stats

./fence_validate --theta 1.0496 --wfloor 0.000390625 --prec 70 \
  --form centered --serial --flat-area-cert --pair-eq-cert \
  --refine flat_pair_refine_w000078125.jsonl \
  --out flat_pair_refine_w0000390625.jsonl --stats
```

## Summary

| Run | wfloor | Time (s) | Leaves | Discarded | Certified | Flat area | Pair-incompatible | Survivors | Exact survivor volume |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Base | 0.025 | 79.139 | 215758 | 11555 | 116319 | 213 | 17884 | 69787 | 69787/4194304 |
| Refine 1 | 0.0125 | 117.585 | 360798 | 2967 | 282735 | 0 | 50065 | 25031 | 25031/67108864 |
| Refine 2 | 0.00625 | 35.091 | 119594 | 28 | 51386 | 0 | 55878 | 12302 | 6151/536870912 |
| Refine 3 | 0.003125 | 19.360 | 66134 | 0 | 17312 | 0 | 39816 | 9006 | 4503/8589934592 |
| Refine 4 | 0.0015625 | 14.399 | 49864 | 0 | 10580 | 0 | 31046 | 8238 | 4119/137438953472 |
| Refine 5 | 0.00078125 | 14.503 | 50504 | 0 | 9696 | 0 | 29790 | 11018 | 5509/2199023255552 |
| Refine 6 | 0.000390625 | 22.381 | 72276 | 0 | 12739 | 0 | 41012 | 18525 | 18525/70368744177664 |

Every certified leaf passed the exact rational comparison `f_hi <= theta`
during classification; the printed maximum is only a rounded-up diagnostic.

## Final Survivor Region

The last refinement file is:

```text
flat_pair_refine_w0000390625.jsonl
```

Analyzer command:

```sh
python3 analyze_cert.py flat_pair_refine_w0000390625.jsonl --theta 1.0496
```

Final survivor product rectangle:

```text
a1 in [1432/4096, 1500/4096] = [0.349609375, 0.3662109375]
a2 in [2835/4096, 2957/4096] = [0.692138671875, 0.721923828125]
b1 in [2596/4096, 2662/4096] = [0.6337890625, 0.64990234375]
b2 in [2835/4096, 2957/4096] = [0.692138671875, 0.721923828125]
```

These are the exact dyadic endpoints.  The earlier shortened decimal display
rounded some endpoints inward and was therefore not a valid containing
rectangle.

Survivor volume:

```text
18525/70368744177664 = 2.6325608359911712e-10 (approximately)
```

All 18525 survivors are certainly admissible.  Boundary-straddling survivor
volume is zero.

Sound reading: outside the survivor union, every admissible normalized
quadrilateral is either certified below `theta` or fails the opposite-pair
equality hypothesis of Proposition 17.  The latter exclusion is conditional:
the paper invokes it only after reducing to active-pair analytic cases.  Legacy
JSONL files encode pair-incompatible leaves with status `"nonoptimal"`.

## Strict Reference Competitor

For the explicit admissible symmetric trapezoid

```text
A_ref = (0.3582548434, 0.7071006812)
B_ref = (0.6417451566, 0.7071006812)
```

The program parses those literals as binary64.  In exact C99 hexadecimal
notation their nontrivial coordinates are
`A_ref.x=0x1.6eda5b90257aep-2`,
`B_ref.x=0x1.4892d237ed429p-1`, and
`A_ref.y=B_ref.y=0x1.6a0919b977760p-1`.

a 160-bit Arb evaluation gives

```text
Phi(T_ref) = [1.0496858140905033070 +/- 1.24e-20],
```

and hence the rigorous lower bound
`Phi(T_ref) >= 1.04968581409050329 > 1.0496`.  This strict competitor bound is
needed to conclude that a quadrilateral certified at or below the threshold is
not a global maximizer; a statement only of the form `m^2(T_ref) > 1.10` would
not suffice because `1.0496^2 = 1.10166016`.

## Distance To T* (Diagnostic Only)

The additional distance analyzer computes ordinary floating-point
farthest-corner diagnostics from each refinement's survivor rectangles to the
displayed decimal reference trapezoid in the public notation `D=(0,0)`,
`C=(1,0)`, `A` left, `B` right:

```sh
python3 analyze_refinement_distances.py
```

The `4D product distance` column is the diagonal diagnostic for the displayed
product rectangle `R_A x R_B`.  The `max leaf 4D distance` column is the tighter
diagnostic over the individual survivor leaves.  These numbers are not
validated interval bounds and are not used by the proof.

| file | survivors | A distance (diagnostic) | B distance (diagnostic) | 4D product distance (diagnostic) | max leaf 4D distance (diagnostic) |
| --- | ---: | ---: | ---: | ---: | ---: |
| flat_pair_cert_w0025.jsonl | 69787 | 0.898853283 | 0.925578382 | 1.2902064 | 0.832882346 |
| flat_pair_refine_w00125.jsonl | 25031 | 0.61938547 | 0.647089329 | 0.895747152 | 0.640574016 |
| flat_pair_refine_w000625.jsonl | 12302 | 0.172000006 | 0.172000006 | 0.243244742 | 0.159888122 |
| flat_pair_refine_w0003125.jsonl | 9006 | 0.0761032378 | 0.0761032378 | 0.107626231 | 0.0706509998 |
| flat_pair_refine_w00015625.jsonl | 8238 | 0.0406977567 | 0.0406977567 | 0.0575553195 | 0.0457130935 |
| flat_pair_refine_w000078125.jsonl | 11018 | 0.0257506677 | 0.0257506677 | 0.0364169435 | 0.0324385554 |
| flat_pair_refine_w0000390625.jsonl | 18525 | 0.0172802155 | 0.0170411685 | 0.024269472 | 0.0237844268 |

## Full Certificate Assembly

The refinement files are partial.  Assemble them before whole-domain
verification:

```sh
./fence_validate --assemble \
  flat_pair_cert_w0025.jsonl \
  flat_pair_refine_w00125.jsonl \
  flat_pair_refine_w000625.jsonl \
  flat_pair_refine_w0003125.jsonl \
  flat_pair_refine_w00015625.jsonl \
  flat_pair_refine_w000078125.jsonl \
  flat_pair_refine_w0000390625.jsonl \
  --out flat_pair_full_w0000390625.jsonl
```

Assembly output:

```text
assembled flat_pair_refine_w00125.jsonl -> 506769 leaves
assembled flat_pair_refine_w000625.jsonl -> 601332 leaves
assembled flat_pair_refine_w0003125.jsonl -> 655164 leaves
assembled flat_pair_refine_w00015625.jsonl -> 696022 leaves
assembled flat_pair_refine_w000078125.jsonl -> 738288 leaves
assembled flat_pair_refine_w0000390625.jsonl -> 799546 leaves
assemble: wrote 799546 leaves to flat_pair_full_w0000390625.jsonl
```

Whole-domain verification:

```sh
./fence_validate --verify flat_pair_full_w0000390625.jsonl \
  --prec 70 --serial
```

Verified output:

```text
verify: 799546 leaves  (discard 14550, certify 500767, flat_area 213, pair_incompatible 265491, survivor 18525)  prec=70
verify: dyadic partition coverage of the full root box: PASSED
verify: worst re-checked certified f_hi = 1.0495999994684  (theta = 1.0496)
verify: 0 failures -> AUDIT PASSED
```

This is the repaired verifier's output.  The historical JSONL records spell
this status `nonoptimal`; the repaired verifier reports it as
`pair_incompatible`, meaning that the Proposition 17 equality intervals are
disjoint.  The displayed worst `f_hi` is a rounded binary64 diagnostic; every
accept/reject decision was made by comparing the Arb endpoint with the exact
rational threshold.

## Artifact Provenance

The historical assembled artifact has SHA-256

```text
58e3a555c83b54e90676062092eaa988da5bf247435949fa663a229921ad8000
```

Its recorded C source was unchanged since commit
`5bcd9694c66cbeab03515c624225f7398c74bc82`; the final-refinement results were
documented in commit `c6dcd2dcebffad89d8143352f719bd6bfa83877b`.
The certificate predates the present repair pass, but the repaired verifier has
rechecked all 799,546 leaves and the full dyadic cover successfully.  For
publication, archive the compressed assembled JSONL beside a tagged repaired
source revision and record which verifier commit performed the audit.
Regeneration is optional; it would canonicalize the legacy status spelling and
must receive its own artifact hash and source record.
