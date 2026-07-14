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
- `functionals.h`, `functionals.c`: the six upper-bound functionals.  In
  centered mode, natural and mean-value enclosures are intersected when they
  overlap; their hull is used if finite enclosures ever disagree.  The module
  also provides `functionals_opposite_pairs_disjoint`, the optional
  nonoptimality test based on the two opposite-pair fences.
- `admissible.h`, `admissible.c`: conservative discard tests for boxes that
  certainly contain no normalized admissible quadrilateral, plus
  `box_small_area_certifies_low`, the flat/small-area low-fence certificate.

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
- `tests/test_trap.c`: smoke tests for the reference trapezoid, pair-bound
  formula agreement, series evaluation, admissibility, one sub-threshold
  certification, the pair-equality nonoptimality certificate, and the
  flat-area certificate.
- `tests/test_cert_cli.sh`: CLI regression test for certificate verification,
  including fast failure on a missing certificate leaf.

## Certificate Reading

Every certificate begins with a metadata JSON line recording the schema,
threshold `theta`, enclosure `form`, `half` flag, auxiliary-certificate flags,
run precision, and root box.  The verifier reads `theta`, `form`, and `half`
from this line.  Supplying those options to `--verify` is now only a
cross-check; a mismatch is reported as `FAIL[meta]`.  Old JSONL files without
metadata must be regenerated or assembled with the current program before they
can be audited.

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

The search space is four-dimensional, with coordinates

```text
(c1, c2, d1, d2)
```

where `C = (c1,c2)` and `D = (d1,d2)`.  A cell is always a rectangular product

```text
[c1_lo,c1_hi] x [c2_lo,c2_hi] x [d1_lo,d1_hi] x [d2_lo,d2_hi].
```

Equivalently, a cell is a pair of planar rectangles:

```text
C-cell = [c1_lo,c1_hi] x [c2_lo,c2_hi],
D-cell = [d1_lo,d1_hi] x [d2_lo,d2_hi].
```

The initial rectangles are

```text
C in [0,2] x [0,1],
D in [-1,1] x [0,1].
```

The splitter chooses the coordinate with largest scaled width, using the root
spans `(2,1,2,1)`:

```text
scaled_width(c1) = width(c1)/2
scaled_width(c2) = width(c2)/1
scaled_width(d1) = width(d1)/2
scaled_width(d2) = width(d2)/1.
```

It then halves only that coordinate interval.  If the chosen coordinate is
`c1` or `c2`, the C-cell is split and the D-cell is unchanged.  If the chosen
coordinate is `d1` or `d2`, the D-cell is split and the C-cell is unchanged.

The first coordinate round gives equal-axis planar grids: C is split into four
rectangles and D is split into four rectangles.  Combining one C rectangle with
one D rectangle gives `4 * 4 = 16` four-dimensional cells.

## Auxiliary Certificates

### Pair-Equality Nonoptimality

At an actual optimizer, the two fence constructions associated to the two pairs
of opposite sides must have equal value.  The code encloses the two
opposite-pair items.  If both enclosures are finite and disjoint, the box is
written as `nonoptimal`.

This certificate is useful because it removes boxes where the current
upper-bound envelope is too wide to certify low, but where the necessary
optimality condition is already impossible.

### Flat/Small-Area Low Certificate

Let

```text
A = Area(ABCD),    h = max(c2,d2).
```

For the fence associated to the pair `AB,CD`, a direct geometric construction
gives

```text
L_AB,CD <= h.
```

For admissible convex quadrilaterals in the normalized domain,

```text
A = area(ABC) + area(ACD) > c2/2,
A = area(ABD) + area(BCD) > d2/2.
```

Hence `h <= 2A`, and therefore

```text
L_AB,CD / sqrt(A) <= 2 sqrt(A).
```

If interval arithmetic proves `A_ub <= theta^2/4`, then

```text
L_AB,CD / sqrt(A) <= theta,
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
```

Observed data:

| Run | wfloor | Time (s) | Leaves | Discarded | Certified | Flat-area | Nonoptimal | Survivors | Survivor volume |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Base | 0.025 | 79.139 | 215758 | 11555 | 116319 | 213 | 17884 | 69787 | <= 1.66385e-2 |
| Refine 1 | 0.0125 | 117.585 | 360798 | 2967 | 282735 | 0 | 50065 | 25031 | <= 3.72991e-4 |
| Refine 2 | 0.00625 | 35.091 | 119594 | 28 | 51386 | 0 | 55878 | 12302 | <= 1.14571e-5 |
| Refine 3 | 0.003125 | 19.360 | 66134 | 0 | 17312 | 0 | 39816 | 9006 | <= 5.24218e-7 |

After the last recorded refinement, all remaining survivors were certainly
admissible and the boundary-straddling survivor volume was zero.  The final
survivor union is contained in

```text
R_C = [0.6171875, 0.703125] x [0.662109375, 0.748046875],
R_D = [0.296875, 0.3828125] x [0.662109375, 0.748046875].
```

The reference trapezoid is

```text
C* = (0.6417451566, 0.7071006812),
D* = (0.3582548434, 0.7071006812),
f(T*) ~= 1.04968581549.
```

The equal-axis PNG figure is
`Documentation/figures/current_survivor_rectangles.png`.

Assemble the full-domain certificate:

```sh
./fence_validate --assemble \
  flat_pair_cert_w0025.jsonl \
  flat_pair_refine_w00125.jsonl \
  flat_pair_refine_w000625.jsonl \
  flat_pair_refine_w0003125.jsonl \
  --out flat_pair_full_w0003125.jsonl
```

Then audit the assembled certificate:

```sh
./fence_validate --verify flat_pair_full_w0003125.jsonl \
  --prec 70 --serial
```

Recorded assembled audit:

```text
verify: 655164 leaves  (discard 14550, certify 467752, flat_area 213,
        nonoptimal 163643, survivor 9006)  prec=70
verify: dyadic partition coverage of the full root box: PASSED
verify: worst re-checked certified f_hi = 1.04959998999232
verify: 0 failures -> AUDIT PASSED
```

## Current Conclusion

For `theta = 1.0496`, with the current code and recorded refinement chain, the
remaining unresolved region is contained in `R_C x R_D` above.  Consequently,
outside `R_C x R_D`, every admissible normalized quadrilateral is either
certified to have shortest fence value at most `1.0496` or is certified
nonoptimal by the pair-equality necessary condition.

This means there is no possible above-threshold optimizer outside
`R_C x R_D`.  The statement is not that every point outside the product
rectangle is low by a fence-length bound alone; some boxes are eliminated by
the independent nonoptimality certificate.

The refinement files listed above are incremental.  Use `--assemble` to obtain
a single auditable certificate for the whole root domain, then run
`./fence_validate --verify` on the assembled JSONL file with sufficient
precision.  The assembled file carries the certified `theta`, `form`, and
`half` values in its metadata; passing them again is optional and acts only as
a consistency check.  Verifying a refinement-only file as a full certificate
now fails immediately with a missing-child coverage error.
