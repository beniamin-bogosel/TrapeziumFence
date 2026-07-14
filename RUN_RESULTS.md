# Regenerated Results

Date: 2026-07-13

Note: current certificates are self-describing.  A JSONL certificate begins
with a metadata line recording `theta`, `form`, `half`, auxiliary flags,
precision provenance, and the root box.  `--verify` reads those values from the
file; passing `--theta`, `--form`, or `--half` during verification is now only
a consistency check.

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
```

## Summary

| Run | wfloor | Time (s) | Leaves | Discarded | Certified | Flat area | Nonoptimal | Survivors | Survivor volume |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Base | 0.025 | 79.139 | 215758 | 11555 | 116319 | 213 | 17884 | 69787 | <= 0.0166385 |
| Refine 1 | 0.0125 | 117.585 | 360798 | 2967 | 282735 | 0 | 50065 | 25031 | <= 0.000372991 |
| Refine 2 | 0.00625 | 35.091 | 119594 | 28 | 51386 | 0 | 55878 | 12302 | <= 1.14571e-05 |
| Refine 3 | 0.003125 | 19.360 | 66134 | 0 | 17312 | 0 | 39816 | 9006 | <= 5.24218e-07 |

All certified-box self-audits reported `f_hi <= theta`.

## Final Survivor Region

The last refinement file is:

```text
flat_pair_refine_w0003125.jsonl
```

Analyzer command:

```sh
python3 analyze_cert.py flat_pair_refine_w0003125.jsonl --theta 1.0496
```

Final survivor product rectangle:

```text
c1 in [0.6171875, 0.703125]
c2 in [0.662109375, 0.748046875]
d1 in [0.296875, 0.3828125]
d2 in [0.662109375, 0.748046875]
```

Survivor volume:

```text
5.24218194187e-07
```

All 9006 survivors are certainly admissible.  Boundary-straddling survivor
volume is zero.

Sound reading: outside the survivor union, every admissible normalized
quadrilateral is either certified below `theta` or certified nonoptimal by the
opposite-pair equality condition.

## Full Certificate Assembly

The refinement files are partial.  Assemble them before whole-domain
verification:

```sh
./fence_validate --assemble \
  flat_pair_cert_w0025.jsonl \
  flat_pair_refine_w00125.jsonl \
  flat_pair_refine_w000625.jsonl \
  flat_pair_refine_w0003125.jsonl \
  --out flat_pair_full_w0003125.jsonl
```

Assembly output:

```text
assembled flat_pair_refine_w00125.jsonl -> 506769 leaves
assembled flat_pair_refine_w000625.jsonl -> 601332 leaves
assembled flat_pair_refine_w0003125.jsonl -> 655164 leaves
assemble: wrote 655164 leaves to flat_pair_full_w0003125.jsonl
```

Whole-domain verification:

```sh
./fence_validate --verify flat_pair_full_w0003125.jsonl \
  --prec 70 --serial
```

Verified output:

```text
verify: 655164 leaves  (discard 14550, certify 467752, flat_area 213, nonoptimal 163643, survivor 9006)  prec=70
verify: dyadic partition coverage of the full root box: PASSED
verify: worst re-checked certified f_hi = 1.04959998999232  (theta = 1.0496)
verify: 0 failures -> AUDIT PASSED
```
