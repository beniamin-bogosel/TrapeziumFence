# File Guide

See `README.md` for the mathematical normalization.  This file describes the
implementation layers, certificate semantics, and the current recorded
threshold experiment.

## Core

- `geom.h`, `geom.c`: FLINT/arb wrappers and a four-variable forward-mode
  automatic-differentiation type (`jet`) used for mean-value interval
  enclosures.
- `series.h`, `series.c`: rigorous enclosures for `gamma/sin(gamma)` and its
  derivative in the parallel-safe opposite-pair bound.
- `functionals.h`, `functionals.c`: the six explicit fence-construction
  candidates.  Each candidate gives an upper bound for the true shortest fence;
  items 4 and 5 are exact opposite-pair construction values, which is essential
  for the pair-equality certificate.  In centered mode, natural and mean-value
  enclosures are intersected when they overlap; their hull is used if finite
  enclosures ever disagree.  The module also provides
  `functionals_opposite_pairs_disjoint`, the optional nonoptimality test based
  on the two exact opposite-pair fences.
- `admissible.h`, `admissible.c`: conservative discard tests for boxes that
  certainly contain no normalized admissible quadrilateral, plus
  `box_small_area_certifies_low`, the flat/small-area low-fence certificate.
- `threshold.h`, `threshold.c`: exact decimal threshold parsing and rational
  comparisons used by the low-fence and flat-area certificates.

## Search And Certificates

- `search.h`, `search.c`: dyadic subdivision.  A leaf is discarded, certified
  low by the six-function minimum, certified low by the flat-area estimate,
  certified nonoptimal by pair-fence disagreement, or retained as an unresolved
  survivor once it reaches `wfloor`.
- `cert.h`, `cert.c`: JSONL writer, verifier, and assembler.  Verification
  first checks the self-describing metadata line, then recomputes all local
  claims and checks that the certificate leaves tile the full root box exactly
  according to the same dyadic splitting rule.  Missing dyadic children fail
  immediately instead of triggering deep subdivision.  Assembly replaces
  survivor leaves through a refinement chain and writes one full-domain
  certificate, after checking that all inputs have compatible metadata.
- `fence_validate.c`: CLI entry point.

## Tools

- `analyze_cert.py`: safe summary of certified-low volume, nonoptimal volume,
  and unresolved survivor neighborhood around the reference trapezoid.
- `analyze_refinement_distances.py`: compares each recorded refinement's
  survivor rectangles to `T*`, reporting diagonal-style Euclidean upper bounds
  for `A`, `B`, and the full four-coordinate product rectangle.
- `tests/test_trap.c`: smoke tests for the reference trapezoid, pair-bound
  formula agreement, series evaluation, admissibility, one sub-threshold
  certification, the pair-equality nonoptimality certificate, and the
  flat-area certificate.
- `tests/test_cert_cli.sh`: CLI regression test for certificate verification,
  including fast failure on a missing certificate leaf.

## Certificate Reading

Every certificate begins with a schema-3 metadata JSON line recording the
threshold `theta` as an exact decimal string, enclosure `form`, `half` flag,
auxiliary-certificate flags, run precision, and root box.  The verifier reads
`theta`, `form`, and `half` from this line.  Supplying those options to
`--verify` is now only a cross-check; a mismatch is reported as `FAIL[meta]`.
Old JSONL files without schema-3 metadata must be regenerated with the current
program before they can be audited.

The exact `theta` literal is parsed as a rational.  Certified leaves compare
the Arb upper endpoint to this rational, and the flat-area certificate compares
the area upper endpoint to the exact rational `theta^2/4`.  Numeric
`theta_approx` fields and printed doubles are diagnostic only.

For a threshold `theta`, a verified certificate proves:

- `discarded` boxes contain no normalized admissible quadrilateral;
- `certified` boxes have true normalized shortest fence value `<= theta`;
- `flat_area` boxes have true normalized shortest fence value `<= theta`, by
  the small-area estimate;
- `nonoptimal` boxes contain no optimizer, because the two opposite-pair fence
  intervals are disjoint;
- `survivor` boxes are unresolved by the current certificates.

The distinction between `flat_area` and `nonoptimal` matters.  A `flat_area`
box is a low-fence certificate.  A `nonoptimal` box is a necessary-condition
certificate: it excludes optimality, but it is not itself a proof that every
point in the box has shortest fence value below `theta`.

Thus, outside the survivor union, every admissible quadrilateral is either
certified below `theta` or certified nonoptimal.  Equivalently, outside the
survivor union there is no possible above-threshold optimizer.

## Cells And Splitting

The public notation uses the normalized quadrilateral
`D=(0,0)`, `C=(1,0)`, `B=(b1,b2)`, `A=(a1,a2)`, in the cyclic order
`D,C,B,A`.  The search space is four-dimensional, with public coordinates

```text
(b1, b2, a1, a2)
```

where `B = (b1,b2)` and `A = (a1,a2)`.  The C implementation and JSONL files
retain the legacy slot order `(c1,c2,d1,d2)`; these slots correspond to
`(b1,b2,a1,a2)` in the notation used in this document.  A cell is always a
rectangular product

```text
[b1_lo,b1_hi] x [b2_lo,b2_hi] x [a1_lo,a1_hi] x [a2_lo,a2_hi].
```

Equivalently, a cell is a pair of planar rectangles:

```text
A-cell = [a1_lo,a1_hi] x [a2_lo,a2_hi],
B-cell = [b1_lo,b1_hi] x [b2_lo,b2_hi].
```

The initial rectangles are

```text
A in [-1,1] x [0,1],
B in [0,2] x [0,1].
```

The splitter chooses the coordinate with largest scaled width, using the root
spans `(2,1,2,1)`:

```text
scaled_width(b1) = width(b1)/2
scaled_width(b2) = width(b2)/1
scaled_width(a1) = width(a1)/2
scaled_width(a2) = width(a2)/1.
```

It then halves only that coordinate interval.  If the chosen coordinate is
`b1` or `b2`, the B-cell is split and the A-cell is unchanged.  If the chosen
coordinate is `a1` or `a2`, the A-cell is split and the B-cell is unchanged.

The first coordinate round gives equal-axis planar grids: A is split into four
rectangles and B is split into four rectangles.  Combining one A rectangle with
one B rectangle gives `4 * 4 = 16` four-dimensional cells.

## Auxiliary Certificates

### Pair-Equality Nonoptimality

At an actual optimizer, the two fence constructions associated to the two pairs
of opposite sides must have equal value.  The code encloses the two exact
opposite-pair candidate values.  If both enclosures are finite and disjoint,
the box is written as `nonoptimal`.

This certificate is useful because it removes boxes where the current
candidate-value envelope is too wide to certify low, but where the necessary
optimality condition is already impossible.  Its soundness relies on items 4
and 5 being exact pair-construction values, not arbitrary upper majorants.

### Flat/Small-Area Low Certificate

Let

```text
A_Q = Area(DCBA),    h = max(a2,b2).
```

For the fence associated to the pair `DC,BA`, a direct geometric construction
gives

```text
L_DC,BA <= h.
```

For admissible convex quadrilaterals in the normalized domain,

```text
A_Q = area(DCB) + area(DBA) > b2/2,
A_Q = area(DCA) + area(CBA) > a2/2.
```

Hence `h <= 2A`, and therefore

```text
L_DC,BA / sqrt(A_Q) <= 2 sqrt(A_Q).
```

If interval arithmetic proves `A_ub <= theta^2/4`, then

```text
L_DC,BA / sqrt(A_Q) <= theta,
```

so the box is certified low.  For `theta = 1.0496`,

```text
theta^2     = 1.10166016,
theta^2 / 4 = 0.27541504.
```

### Angle Consequence

The vertex fence at an angle `alpha` gives normalized value `sqrt(alpha)`.
Therefore, if the smallest angle is below `theta^2`, the quadrilateral is
already certified below `theta`.  Any possible above-threshold optimizer must
have every angle at least `theta^2`, so its largest angle is at most

```text
2*pi - 3*theta^2.
```

For `theta = 1.0496`, this upper bound is

```text
2.978204827 radians = 170.638567 degrees.
```

Equivalently, the excess over a straight angle is at least

```text
3*theta^2 - pi = 0.163387826 radians.
```

This explains why very flat candidates should be removable.  The implemented
flat-area certificate is the direct quantitative version used in the current
runs.

## Reproducing The Current Theta = 1.0496 Run

These runs were made in serial mode (`--serial`, `OMP=0`) with
`--form centered`, precision `70`, and both auxiliary certificates enabled:
`--flat-area-cert --pair-eq-cert`.  Each refinement step uses only the survivor
boxes from the previous step.

Machine used for the timings:

- Host: `beni-ThinkPad-P53`
- OS/kernel: Ubuntu 24.04, Linux `6.11.0-29-generic`
- CPU: Intel Core i7-9750H @ 2.60 GHz, 6 cores / 12 threads
- Memory: 31 GiB RAM, 29 GiB swap
- Compiler: Ubuntu GCC `13.3.0`
- FLINT: `3.0.1` through the merged FLINT 3 Arb layout
  (`<flint/arb.h>`, linked with `-lflint`)
- MPFR: `4.2.1`
- GMP: `6.3.0`
- Python: `3.12.9`
- pdfTeX: `3.141592653-2.6-1.40.25`, TeX Live 2023/Debian
- Build/run mode: single-threaded (`make OMP=0`, `--serial`)

Build:

```sh
make OMP=0
```

Commands:

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
```

Observed data:

| Run | wfloor | Time (s) | Leaves | Discarded | Certified | Flat-area | Nonoptimal | Survivors | Survivor volume |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Base | 0.025 | 79.139 | 215758 | 11555 | 116319 | 213 | 17884 | 69787 | <= 1.66385e-2 |
| Refine 1 | 0.0125 | 117.585 | 360798 | 2967 | 282735 | 0 | 50065 | 25031 | <= 3.72991e-4 |
| Refine 2 | 0.00625 | 35.091 | 119594 | 28 | 51386 | 0 | 55878 | 12302 | <= 1.14571e-5 |
| Refine 3 | 0.003125 | 19.360 | 66134 | 0 | 17312 | 0 | 39816 | 9006 | <= 5.24218e-7 |
| Refine 4 | 0.0015625 | 14.399 | 49864 | 0 | 10580 | 0 | 31046 | 8238 | <= 2.99697e-8 |
| Refine 5 | 0.00078125 | 14.503 | 50504 | 0 | 9696 | 0 | 29790 | 11018 | <= 2.5052e-9 |

After the last recorded refinement, all remaining survivors were certainly
admissible and the boundary-straddling survivor volume was zero.  The final
survivor union is contained in

```text
R_A = [0.3427734375, 0.369140625] x [0.6865234375, 0.7270507812],
R_B = [0.630859375, 0.6572265625] x [0.6865234375, 0.7270507812].
```

The reference trapezoid is

```text
A* = (0.3582548434, 0.7071006812),
B* = (0.6417451566, 0.7071006812),
f(T*) ~= 1.04968581549.
```

The equal-axis PNG figure is
`Documentation/figures/current_survivor_rectangles.png`.  The overview PNG
showing these rectangles inside the conjectured trapezium neighborhood is
`Documentation/figures/survivor_rectangles_overview.png`.

Distance-to-`T*` analyzer:

```sh
python3 analyze_refinement_distances.py
```

The analyzer takes the survivor bounding rectangles from each refinement and
computes farthest-corner distances from
`A* = (0.3582548434, 0.7071006812)` and
`B* = (0.6417451566, 0.7071006812)`.  The four-dimensional product bound is
`sqrt(A_bound^2 + B_bound^2)`.  `max leaf 4D bound` is the tighter maximum over
individual survivor leaves rather than over the displayed product rectangle.

| File | Survivors | A rectangle bound | B rectangle bound | 4D product bound | Max leaf 4D bound |
| --- | ---: | ---: | ---: | ---: | ---: |
| `flat_pair_cert_w0025.jsonl` | 69787 | 0.898853283 | 0.925578382 | 1.2902064 | 0.832882346 |
| `flat_pair_refine_w00125.jsonl` | 25031 | 0.61938547 | 0.647089329 | 0.895747152 | 0.640574016 |
| `flat_pair_refine_w000625.jsonl` | 12302 | 0.172000006 | 0.172000006 | 0.243244742 | 0.159888122 |
| `flat_pair_refine_w0003125.jsonl` | 9006 | 0.0761032378 | 0.0761032378 | 0.107626231 | 0.0706509998 |
| `flat_pair_refine_w00015625.jsonl` | 8238 | 0.0406977567 | 0.0406977567 | 0.0575553195 | 0.0457130935 |
| `flat_pair_refine_w000078125.jsonl` | 11018 | 0.0257506677 | 0.0257506677 | 0.0364169435 | 0.0324385554 |

Assemble the full-domain certificate:

```sh
./fence_validate --assemble \
  flat_pair_cert_w0025.jsonl \
  flat_pair_refine_w00125.jsonl \
  flat_pair_refine_w000625.jsonl \
  flat_pair_refine_w0003125.jsonl \
  flat_pair_refine_w00015625.jsonl \
  flat_pair_refine_w000078125.jsonl \
  --out flat_pair_full_w000078125.jsonl
```

Then audit the assembled certificate:

```sh
./fence_validate --verify flat_pair_full_w000078125.jsonl \
  --prec 70 --serial
```

Recorded assembled audit:

```text
verify: 738288 leaves  (discard 14550, certify 488028, flat_area 213,
        nonoptimal 224479, survivor 11018)  prec=70
verify: dyadic partition coverage of the full root box: PASSED
verify: worst re-checked certified f_hi = 1.04959999626394
verify: 0 failures -> AUDIT PASSED
```

## Current Conclusion

For `theta = 1.0496`, with the current code and recorded refinement chain, the
remaining unresolved region is contained in `R_A x R_B` above.  Consequently,
outside `R_A x R_B`, every admissible normalized quadrilateral is either
certified to have shortest fence value at most `1.0496` or is certified
nonoptimal by the pair-equality necessary condition.

This means there is no possible above-threshold optimizer outside
`R_A x R_B`.  The statement is not that every point outside the product
rectangle is low by a fence-length bound alone; some boxes are eliminated by
the independent nonoptimality certificate.

The refinement files listed above are incremental.  Use `--assemble` to obtain
a single auditable certificate for the whole root domain, then run
`./fence_validate --verify` on the assembled JSONL file with sufficient
precision.  The assembled file carries the certified `theta`, `form`, and
`half` values in its metadata; passing them again is optional and acts only as
a consistency check.  Verifying a refinement-only file as a full certificate
now fails immediately with a missing-child coverage error.
