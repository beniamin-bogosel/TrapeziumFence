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
  candidates.  Proposition 6 identifies their minimum with the normalized
  shortest fence; in particular, each candidate gives an upper bound.
  Items 4 and 5 are exact opposite-pair construction values.  Their angles are
  the geometric sectors containing the quadrilateral, and a parallel-safe
  expression is used near parallelism.  Exactness is essential for the
  conditional pair-equality test.  In centered mode, natural and mean-value
  enclosures are intersected when they overlap; their hull is used if finite
  enclosures ever disagree.  The module also provides
  `functionals_opposite_pairs_disjoint`, the optional pair-compatibility test
  based on the two exact opposite-pair fences.
- `admissible.h`, `admissible.c`: conservative discard tests for boxes that
  certainly contain no normalized admissible quadrilateral, plus
  `box_small_area_certifies_low`, the flat/small-area low-fence certificate.
- `threshold.h`, `threshold.c`: exact decimal threshold parsing and rational
  comparisons used by the low-fence and flat-area certificates.

## Search And Certificates

- `search.h`, `search.c`: dyadic subdivision.  A leaf is discarded, certified
  low by the six-function minimum, certified low by the flat-area estimate,
  marked pair-incompatible by pair-fence disagreement, or retained as an
  unresolved survivor once it reaches `wfloor`.  The optional OpenMP driver
  uses atomic termination state and the actual runtime team size, including
  when the runtime dynamically reduces the requested number of threads.
- `cert.h`, `cert.c`: JSONL writer, verifier, and assembler.  Verification
  first checks the self-describing metadata line, then recomputes all local
  claims and checks that the certificate leaves tile the full root box exactly
  according to the same dyadic splitting rule.  Missing dyadic children fail
  immediately instead of triggering deep subdivision.  Assembly replaces
  survivor leaves through a refinement chain and writes one full-domain
  certificate.  It requires matching schema, exact threshold, enclosure form,
  half-domain choice, and root; it ORs the two auxiliary-certificate flags and
  records the minimum component run precision in the assembled metadata.
- `fence_validate.c`: CLI entry point.

## Tools

- `analyze_cert.py`: safe summary of certified-low volume, pair-incompatible
  volume, and the unresolved survivor neighborhood around the reference
  trapezoid.  It sums the dyadic box volumes as exact rationals and uses exact
  rational endpoint tests for the certainly-admissible survivor core.
- `analyze_refinement_distances.py`: compares each recorded refinement's
  survivor rectangles to `T*`, reporting floating-point distance diagnostics
  for `A`, `B`, and the full four-coordinate product rectangle.  These distance
  figures use ordinary floating point and are explicitly labelled diagnostics;
  they are not validated certificate bounds.
- `tests/test_trap.c`: smoke tests for the reference trapezoid and its strict
  threshold comparison, ordinary obtuse and near-`pi` pair-bound formula
  agreement, value/derivative series evaluation, admissibility, one
  sub-threshold certification, the conditional pair-equality compatibility
  test, and the flat-area certificate.
- `tests/test_cert_cli.sh`: CLI regression test for certificate verification,
  including strict numeric-input checks, legacy status compatibility, null
  diagnostic fields, dynamic-team OpenMP termination, an exact leaf cap, and
  fast failure on a missing certificate leaf.

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

- `discarded` boxes contain no normalized admissible quadrilateral, or (when
  `half=true`) lie wholly outside the selected symmetry half-domain;
- `certified` boxes have true normalized shortest fence value `<= theta`;
- `flat_area` boxes have true normalized shortest fence value `<= theta`, by
  the small-area estimate;
- `pair_incompatible` boxes cannot satisfy the opposite-pair equality
  hypothesis of Proposition 17, because the two exact pair-value intervals are
  disjoint;
- `survivor` boxes are unresolved by the current certificates.

Legacy schema-3 certificates spell `pair_incompatible` as `nonoptimal`.  The
current verifier and analysis tools accept both wire-format strings.  The
legacy name must not be interpreted as an unconditional assertion.

The distinction between `flat_area` and `pair_incompatible` matters.  A
`flat_area` box is a low-fence certificate.  A `pair_incompatible` box proves
only failure of the equality assumed in Proposition 17.  In the manuscript
that equality applies after the analytic reduction to configurations in which
both opposite-pair fences are active; it is not a necessary condition stated
for every possible optimizer.

For `half=false`, outside the survivor union every admissible quadrilateral is
either certified below `theta` or fails the opposite-pair equality hypothesis.
Equivalently, within the Proposition 17 equality class (including the relevant
active-pair cases), there is no possible above-threshold candidate outside the
survivor union.  For `half=true`, the statement is initially restricted to the
selected representative half-domain; the full-domain outer neighborhood is
the union of the survivors and their reflected copies.

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

### Conditional Pair-Equality Compatibility

Proposition 17 assumes that the two fence constructions associated with the two
pairs of opposite sides have equal value.  The paper establishes this equality
in the analytic cases where both pair fences are active.  The code encloses the
two exact opposite-pair candidate values.  If both enclosures are finite and
disjoint, the box is written as `pair_incompatible` because it contains no
quadrilateral satisfying that hypothesis.

This test is useful because it removes boxes where the candidate-value envelope
is too wide to certify low, but the conditional equality is impossible.  Its
soundness relies on items 4 and 5 being exact pair-construction values, with the
correct sector containing the quadrilateral, rather than arbitrary upper
majorants.  It does not by itself exclude optimizers in analytic configurations
where pair equality has not been proved.

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

For `theta = 1.0496`, ordinary high-precision evaluation gives the diagnostic
approximations

```text
2*pi - 3*theta^2 ~= 2.9782048271795865 radians
                   ~= 170.63856712287902 degrees.
```

If `alpha_max` is the largest angle, the guaranteed gap from a straight angle
satisfies

```text
pi - alpha_max >= 3*theta^2 - pi
               ~= 0.16338782641020676 radians.
```

The exact symbolic expressions, not the rounded diagnostics, carry the
inequality directions above.

This explains why very flat candidates should be removable.  The implemented
flat-area certificate is the direct quantitative version used in the recorded
runs.

## Reproducing The Recorded Theta = 1.0496 Run

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

./fence_validate --theta 1.0496 --wfloor 0.000390625 --prec 70 \
  --form centered --serial --flat-area-cert --pair-eq-cert \
  --refine flat_pair_refine_w000078125.jsonl \
  --out flat_pair_refine_w0000390625.jsonl --stats
```

Observed data:

| Run | wfloor | Time (s) | Leaves | Discarded | Certified | Flat-area | Pair-incompatible | Survivors | Exact survivor volume |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Base | 0.025 | 79.139 | 215758 | 11555 | 116319 | 213 | 17884 | 69787 | 69787/4194304 |
| Refine 1 | 0.0125 | 117.585 | 360798 | 2967 | 282735 | 0 | 50065 | 25031 | 25031/67108864 |
| Refine 2 | 0.00625 | 35.091 | 119594 | 28 | 51386 | 0 | 55878 | 12302 | 6151/536870912 |
| Refine 3 | 0.003125 | 19.360 | 66134 | 0 | 17312 | 0 | 39816 | 9006 | 4503/8589934592 |
| Refine 4 | 0.0015625 | 14.399 | 49864 | 0 | 10580 | 0 | 31046 | 8238 | 4119/137438953472 |
| Refine 5 | 0.00078125 | 14.503 | 50504 | 0 | 9696 | 0 | 29790 | 11018 | 5509/2199023255552 |
| Refine 6 | 0.000390625 | 22.381 | 72276 | 0 | 12739 | 0 | 41012 | 18525 | 18525/70368744177664 |

After the last recorded refinement, all remaining survivors were certainly
admissible and the boundary-straddling survivor volume was zero.  The final
survivor union is contained in

```text
R_A = [1432/4096, 1500/4096] x [2835/4096, 2957/4096]
    = [0.349609375, 0.3662109375]
      x [0.692138671875, 0.721923828125],
R_B = [2596/4096, 2662/4096] x [2835/4096, 2957/4096]
    = [0.6337890625, 0.64990234375]
      x [0.692138671875, 0.721923828125].
```

These are exact dyadic endpoints from the certificate.  Shorter decimal
rounding must be directed outward; the earlier ten-digit display rounded some
endpoints inward and therefore described a rectangle slightly smaller than the
actual survivor hull.

The exact sum of the final dyadic survivor-box volumes is
`18525/70368744177664`, approximately
`2.6325608359911712e-10`.  This volume is descriptive and is not used in the
exclusion proof.

The reference trapezoid is

```text
A_ref = (0.3582548434, 0.7071006812),
B_ref = (0.6417451566, 0.7071006812),
Phi(T_ref) >= 1.04968581409050329 > 1.0496.
```

These decimal literals are parsed as binary64 values.  The exact coordinates
are `A_ref.x=0x1.6eda5b90257aep-2`,
`B_ref.x=0x1.4892d237ed429p-1`, and
`A_ref.y=B_ref.y=0x1.6a0919b977760p-1` in C99 hexadecimal notation.

At 160-bit precision, evaluating this explicit admissible reference
quadrilateral gives the Arb enclosure
`[1.0496858140905033070 +/- 1.24e-20]`.  The strict lower endpoint, not merely a
rounded conjectural value, is what proves that any quadrilateral certified at
or below `1.0496` cannot be a global maximizer.

The equal-axis PNG figure is
`Documentation/figures/current_survivor_rectangles.png`.  The overview PNG
showing these rectangles inside the conjectured trapezium neighborhood is
`Documentation/figures/survivor_rectangles_overview.png`.

Distance-to-`T*` analyzer:

```sh
python3 analyze_refinement_distances.py
```

The analyzer takes the survivor bounding rectangles from each refinement and
computes ordinary floating-point farthest-corner diagnostics from
`A* = (0.3582548434, 0.7071006812)` and
`B* = (0.6417451566, 0.7071006812)`.  The four-dimensional product bound is
`sqrt(A_distance^2 + B_distance^2)`.  `max leaf 4D distance` is the tighter
diagnostic over individual survivor leaves rather than over the displayed
product rectangle.  These values are not interval-certified and are not used
in the proof.

| File | Survivors | A distance (diagnostic) | B distance (diagnostic) | 4D product distance (diagnostic) | Max leaf 4D distance (diagnostic) |
| --- | ---: | ---: | ---: | ---: | ---: |
| `flat_pair_cert_w0025.jsonl` | 69787 | 0.898853283 | 0.925578382 | 1.2902064 | 0.832882346 |
| `flat_pair_refine_w00125.jsonl` | 25031 | 0.61938547 | 0.647089329 | 0.895747152 | 0.640574016 |
| `flat_pair_refine_w000625.jsonl` | 12302 | 0.172000006 | 0.172000006 | 0.243244742 | 0.159888122 |
| `flat_pair_refine_w0003125.jsonl` | 9006 | 0.0761032378 | 0.0761032378 | 0.107626231 | 0.0706509998 |
| `flat_pair_refine_w00015625.jsonl` | 8238 | 0.0406977567 | 0.0406977567 | 0.0575553195 | 0.0457130935 |
| `flat_pair_refine_w000078125.jsonl` | 11018 | 0.0257506677 | 0.0257506677 | 0.0364169435 | 0.0324385554 |
| `flat_pair_refine_w0000390625.jsonl` | 18525 | 0.0172802155 | 0.0170411685 | 0.024269472 | 0.0237844268 |

Assemble the full-domain certificate:

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

Then audit the assembled certificate:

```sh
./fence_validate --verify flat_pair_full_w0000390625.jsonl \
  --prec 70 --serial
```

Recorded assembled audit:

```text
verify: 799546 leaves  (discard 14550, certify 500767, flat_area 213,
        pair_incompatible 265491, survivor 18525)  prec=70
verify: dyadic partition coverage of the full root box: PASSED
verify: worst re-checked certified f_hi = 1.0495999994684  (theta = 1.0496)
verify: 0 failures -> AUDIT PASSED
```

This is the repaired verifier's output.  The historical JSONL leaf records use
the legacy status string `nonoptimal`; the verifier reports those records as
`pair_incompatible`.  They record failure of the Proposition 17 equality
hypothesis, not unconditional nonoptimality.  The displayed `f_hi` is a
rounded binary64 diagnostic; the audit result comes from direct comparison of
each Arb upper endpoint with the exact rational threshold.

### Artifact provenance

The historical assembled artifact above has SHA-256

```text
58e3a555c83b54e90676062092eaa988da5bf247435949fa663a229921ad8000
```

The C source used for the recorded chain was unchanged since commit
`5bcd9694c66cbeab03515c624225f7398c74bc82`; the final refinement and results
were documented in commit `c6dcd2dcebffad89d8143352f719bd6bfa83877b`.
The repaired verifier has rechecked this same artifact in full: all 799,546
leaves and the dyadic cover pass unchanged.  Because JSONL artifacts are
intentionally ignored by Git, a publishable proof package should archive the
compressed assembled certificate, tag the exact repaired verifier source, and
publish hashes for both.  Regeneration is optional but would emit the canonical
`pair_incompatible` status; any regenerated artifact needs its own source and
checksum record.

## Current Conclusion

For `theta = 1.0496`, with the current code and recorded refinement chain, the
remaining unresolved region is contained in `R_A x R_B` above.  Consequently,
outside `R_A x R_B`, every admissible normalized quadrilateral is either
certified to have shortest fence value at most `1.0496` or is proved
incompatible with the opposite-pair equality hypothesis in Proposition 17.

After the paper's analytic reduction to the cases in which both opposite-pair
fences are active, this means there is no possible above-threshold candidate
outside `R_A x R_B`.  Without that reduction, the numerical statement is only
the disjunction “certified low or pair-incompatible”; it is not a global
nonoptimality theorem.  Nor does it say that every point outside the product
rectangle is low by a fence-length bound alone.

The refinement files listed above are incremental.  Use `--assemble` to obtain
a single auditable certificate for the whole root domain, then run
`./fence_validate --verify` on the assembled JSONL file with sufficient
precision.  The assembled file carries the certified `theta`, `form`, and
`half` values in its metadata; passing them again is optional and acts only as
a consistency check.  Verifying a refinement-only file as a full certificate
now fails immediately with a missing-child coverage error.
