# Regenerated Results

Date: 2026-07-14

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
```

## Summary

| Run | wfloor | Time (s) | Leaves | Discarded | Certified | Flat area | Nonoptimal | Survivors | Survivor volume |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Base | 0.025 | 79.139 | 215758 | 11555 | 116319 | 213 | 17884 | 69787 | <= 0.0166385 |
| Refine 1 | 0.0125 | 117.585 | 360798 | 2967 | 282735 | 0 | 50065 | 25031 | <= 0.000372991 |
| Refine 2 | 0.00625 | 35.091 | 119594 | 28 | 51386 | 0 | 55878 | 12302 | <= 1.14571e-05 |
| Refine 3 | 0.003125 | 19.360 | 66134 | 0 | 17312 | 0 | 39816 | 9006 | <= 5.24218e-07 |
| Refine 4 | 0.0015625 | 14.399 | 49864 | 0 | 10580 | 0 | 31046 | 8238 | <= 2.99697e-08 |

All certified-box self-audits reported `f_hi <= theta`.

## Final Survivor Region

The last refinement file is:

```text
flat_pair_refine_w00015625.jsonl
```

Analyzer command:

```sh
python3 analyze_cert.py flat_pair_refine_w00015625.jsonl --theta 1.0496
```

Final survivor product rectangle:

```text
c1 in [0.626953125, 0.669921875]
c2 in [0.677734375, 0.7353515625]
d1 in [0.330078125, 0.373046875]
d2 in [0.677734375, 0.7353515625]
```

Survivor volume:

```text
2.99696694128e-08
```

All 8238 survivors are certainly admissible.  Boundary-straddling survivor
volume is zero.

Sound reading: outside the survivor union, every admissible normalized
quadrilateral is either certified below `theta` or certified nonoptimal by the
opposite-pair equality condition.

## Distance To T*

The additional distance analyzer computes diagonal-style Euclidean upper bounds
from each refinement's survivor rectangles to the reference trapezoid:

```sh
python3 analyze_refinement_distances.py
```

The `4D product bound` column is the diagonal bound for the displayed product
rectangle `R_C x R_D`.  The `max leaf 4D bound` column is the tighter maximum
over the individual survivor leaves.

| file | survivors | C rectangle bound | D rectangle bound | 4D product bound | max leaf 4D bound |
| --- | ---: | ---: | ---: | ---: | ---: |
| flat_pair_cert_w0025.jsonl | 69787 | 0.925578382 | 0.898853283 | 1.2902064 | 0.832882346 |
| flat_pair_refine_w00125.jsonl | 25031 | 0.647089329 | 0.61938547 | 0.895747152 | 0.640574016 |
| flat_pair_refine_w000625.jsonl | 12302 | 0.172000006 | 0.172000006 | 0.243244742 | 0.159888122 |
| flat_pair_refine_w0003125.jsonl | 9006 | 0.0761032378 | 0.0761032378 | 0.107626231 | 0.0706509998 |
| flat_pair_refine_w00015625.jsonl | 8238 | 0.0406977567 | 0.0406977567 | 0.0575553195 | 0.0457130935 |

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
  --out flat_pair_full_w00015625.jsonl
```

Assembly output:

```text
assembled flat_pair_refine_w00125.jsonl -> 506769 leaves
assembled flat_pair_refine_w000625.jsonl -> 601332 leaves
assembled flat_pair_refine_w0003125.jsonl -> 655164 leaves
assembled flat_pair_refine_w00015625.jsonl -> 696022 leaves
assemble: wrote 696022 leaves to flat_pair_full_w00015625.jsonl
```

Whole-domain verification:

```sh
./fence_validate --verify flat_pair_full_w00015625.jsonl \
  --prec 70 --serial
```

Verified output:

```text
verify: 696022 leaves  (discard 14550, certify 478332, flat_area 213, nonoptimal 194689, survivor 8238)  prec=70
verify: dyadic partition coverage of the full root box: PASSED
verify: worst re-checked certified f_hi = 1.04959999626394  (theta = 1.0496)
verify: 0 failures -> AUDIT PASSED
```
