# Regenerated Results

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

| Run | wfloor | Time (s) | Leaves | Discarded | Certified | Flat area | Nonoptimal | Survivors | Survivor volume |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Base | 0.025 | 79.139 | 215758 | 11555 | 116319 | 213 | 17884 | 69787 | <= 0.0166385 |
| Refine 1 | 0.0125 | 117.585 | 360798 | 2967 | 282735 | 0 | 50065 | 25031 | <= 0.000372991 |
| Refine 2 | 0.00625 | 35.091 | 119594 | 28 | 51386 | 0 | 55878 | 12302 | <= 1.14571e-05 |
| Refine 3 | 0.003125 | 19.360 | 66134 | 0 | 17312 | 0 | 39816 | 9006 | <= 5.24218e-07 |
| Refine 4 | 0.0015625 | 14.399 | 49864 | 0 | 10580 | 0 | 31046 | 8238 | <= 2.99697e-08 |
| Refine 5 | 0.00078125 | 14.503 | 50504 | 0 | 9696 | 0 | 29790 | 11018 | <= 2.5052e-09 |
| Refine 6 | 0.000390625 | 22.381 | 72276 | 0 | 12739 | 0 | 41012 | 18525 | <= 2.63256e-10 |

All certified-box self-audits reported `f_hi <= theta`.

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
a1 in [0.349609375, 0.3662109375]
a2 in [0.6921386719, 0.7219238281]
b1 in [0.6337890625, 0.6499023438]
b2 in [0.6921386719, 0.7219238281]
```

Survivor volume:

```text
2.63256083599e-10
```

All 18525 survivors are certainly admissible.  Boundary-straddling survivor
volume is zero.

Sound reading: outside the survivor union, every admissible normalized
quadrilateral is either certified below `theta` or certified nonoptimal by the
opposite-pair equality condition.

## Distance To T*

The additional distance analyzer computes diagonal-style Euclidean upper bounds
from each refinement's survivor rectangles to the reference trapezoid in the
public notation `D=(0,0)`, `C=(1,0)`, `A` left, `B` right:

```sh
python3 analyze_refinement_distances.py
```

The `4D product bound` column is the diagonal bound for the displayed product
rectangle `R_A x R_B`.  The `max leaf 4D bound` column is the tighter maximum
over the individual survivor leaves.

| file | survivors | A rectangle bound | B rectangle bound | 4D product bound | max leaf 4D bound |
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
verify: 799546 leaves  (discard 14550, certify 500767, flat_area 213, nonoptimal 265491, survivor 18525)  prec=70
verify: dyadic partition coverage of the full root box: PASSED
verify: worst re-checked certified f_hi = 1.0495999994684  (theta = 1.0496)
verify: 0 failures -> AUDIT PASSED
```
