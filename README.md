# TrapeziumFence

Companion code for the paper

**The quadrilateral with the longest shortest fence**

by **Beniamin Bogosel**, **Dorin Bucur** and **Ilaria Fragala**.

Interval-arithmetic checker for the normalized longest-shortest-fence problem.

We normalize a convex quadrilateral by choosing a longest edge, naming it
`DC`, scaling it to length `1`, and placing
`D=(0,0)`, `C=(1,0)`.  Then
`B=(b1,b2)`, `A=(a1,a2)` and the normalized domain is

```
b1 in [0,2],  b2 in [0,1],  a1 in [-1,1],  a2 in [0,1],
convex/CCW in the order D,C,B,A, and |CB|, |BA|, |AD| <= 1.
```

The objective is `shortest_fence_length / sqrt(area(DCBA))`, so this
longest-edge normalization does not lose generality.

Internally, the C source and JSONL boxes keep the original coordinate slots
`(c1,c2,d1,d2)`.  In the public notation used here,
`(c1,c2)` means `B=(b1,b2)` and `(d1,d2)` means `A=(a1,a2)`.

## What Is Certified

For each box in `(b1,b2,a1,a2)`, the program evaluates six rigorous
enclosures of explicit fence-construction values:

- four vertex-angle candidates;
- two exact opposite-side pair-construction candidates.  The geometric sector
  containing the quadrilateral is selected explicitly, with a parallel-safe
  formula used near parallel side pairs.

Proposition 6 identifies the normalized shortest fence with the minimum of
these six values (the code takes square roots of the squared formulas in the
paper).  For box certification only the one-sided consequence is needed: each
item is an upper bound, so an interval proof that `f_hi <= theta` proves that
every admissible quadrilateral in the box has normalized shortest fence value
at most `theta`.  Such a box is written as `"certified"`.

Notation misalignment: The paper uses the notation `sigma` instead of `theta` for the threshold.

Two optional certificates are also available:

- `--flat-area-cert`: certifies low boxes using
  `L_DC,BA / sqrt(area) <= 2 sqrt(area)`;
- `--pair-eq-cert`: marks a box `pair_incompatible` when the two exact
  opposite-pair candidate intervals are disjoint.  This proves that the box
  cannot satisfy the equality hypothesis in Proposition 17.  That equality is
  available in the paper only after reducing to the analytic cases in which
  both opposite-pair fences are active; it is not asserted for every optimizer.

The output is therefore an exclusion certificate:

- `discarded`: the box contains no normalized admissible quadrilateral (or,
  with `--half`, lies wholly outside the selected symmetry half-domain);
- `certified`: the shortest fence is proved `<= theta` throughout the box;
- `flat_area`: the shortest fence is proved `<= theta` by the small-area
  estimate;
- `pair_incompatible`: the box cannot meet the opposite-pair equality
  hypothesis of Proposition 17;
- `survivor`: unresolved at the requested width floor.

Legacy schema-3 JSONL files use the status string `"nonoptimal"` for
`pair_incompatible`; the verifier and analysis scripts accept both spellings.
The old spelling must not be read as an unconditional global claim.

For a full-domain certificate (`half=false`), within the equality class of
Proposition 17 (in particular, after the paper's active-pair analytic
reduction), the survivor union is a rigorous outer neighborhood of possible
above-threshold candidates.  With `half=true`, this statement applies first to
the selected representative half-domain; reflect its survivor union to obtain
the full-domain neighborhood.  When `--pair-eq-cert` is enabled, not every
eliminated box is certified low; some instead fail the conditional equality
hypothesis.

## Build

Requires FLINT/arb, MPFR, and GMP.  The recorded runs used FLINT `3.0.1`
with Arb through the merged FLINT 3 layout (`<flint/arb.h>`), MPFR `4.2.1`,
GMP `6.3.0`, GCC `13.3.0`, Python `3.12.9`, and pdfTeX from TeX Live
2023/Debian.

```
make OMP=0      # deterministic single-threaded build
make            # OMP=auto: use OpenMP when the compiler supports it
make OMP=1      # require OpenMP support
make test
```

## Run

Example:

```
./fence_validate --theta 1.04 --wfloor 0.05 --prec 53 \
  --form centered --serial --out cert_104.jsonl --stats

./fence_validate --verify cert_104.jsonl --prec 80 --serial

python3 analyze_cert.py cert_104.jsonl --theta 1.04

python3 analyze_refinement_distances.py
```

For refinement chains, assemble the base file and all refinement files before
whole-domain verification:

```
./fence_validate --assemble base.jsonl refine1.jsonl refine2.jsonl \
  --out assembled_full.jsonl

./fence_validate --verify assembled_full.jsonl --prec 80 --serial
```

Important options:

- `--theta T`: exact decimal threshold to certify below.  The literal is
  parsed as a rational for proof comparisons; the printed double is diagnostic.
- `--wfloor W`: stop splitting unresolved boxes once their largest scaled width
  is at most `W`.
- `--form natural|centered`: direct interval extension or mean-value form.
- `--flat-area-cert`: apply the small-area low-fence certificate.
- `--pair-eq-cert`: apply the conditional opposite-pair equality compatibility
  test used in Proposition 17.
- `--verify FILE`: independently re-checks metadata, local claims, and proves
  the JSONL leaves tile the full root box.  The certificate stores `theta`,
  `form`, `half`, and root-domain provenance; command-line values for those
  fields are optional cross-checks, not the source of the claim.  Incomplete
  coverage now fails immediately with the first missing dyadic child.
- `--assemble FILE... --out OUT`: replace survivor leaves through a refinement
  chain and write one full-domain certificate.
- `--max-leaves N`: debugging limit only; a truncated run will not pass full
  certificate coverage verification.

`analyze_refinement_distances.py` is a convenience analyzer for the recorded
refinement chain.  It computes diagonal-style distances from each survivor
rectangle to the displayed decimal reference trapezoid `T*`.  These are
ordinary floating-point diagnostics, not validated certificate bounds.

## Soundness Fixes Relative To The Starting Snapshot

- Certificate verification now checks the complete dyadic partition of the root
  box, not just each JSON line in isolation, and fails fast on missing regions.
- Certificate files are self-describing: the verifier rejects missing metadata
  and rejects command-line `theta`, `form`, or `half` values that disagree with
  the file.
- Threshold comparisons are exact in the supplied decimal `theta`: certified
  leaves compare Arb upper endpoints to the corresponding rational, and
  `--flat-area-cert` compares to the exact rational `theta^2/4`.
- Refinement files can be assembled with `--assemble`; each refinement is
  structurally checked against the survivor boxes it replaces.
- Centered and natural enclosures are intersected when possible; if finite
  enclosures ever fail to overlap, their interval hull is used conservatively.
- Post-processing does not infer lower bounds for unresolved boxes.  At the
  explicit reference point, however, Proposition 6 makes the finite six-item
  minimum equal to the normalized shortest fence, so its rigorous lower
  endpoint supplies the strict comparison with `1.0496`.
- Opposite-side candidates use the actual sector containing the quadrilateral,
  rather than always replacing it by the acute angle between support lines.
- The exact verifier keeps Arb/ARF objects alive until every comparison using
  them has completed.
- Proof parameters are parsed strictly: thresholds and width floors must be
  positive and finite, precisions must be positive, and malformed numeric
  suffixes are rejected.  Certificate metadata also requires a positive exact
  threshold.
- OpenMP termination flags use atomic accesses, and idle-worker termination is
  based on the actual team size so dynamically reduced teams cannot hang.  The
  Makefile probes optional OpenMP support, records header dependencies, and
  keeps serial and OpenMP objects in separate build directories so changing
  `OMP` cannot reuse objects compiled for the other mode.

## Reference Point

The explicit admissible isosceles trapezoid used for sanity checks is

```
A* = (0.3582548434, 0.7071006812)
B* = (0.6417451566, 0.7071006812)
Phi(T_ref) >= 1.04968581409050329 > 1.0496
```

The coordinate literals are parsed as binary64 values.  Their exact C99
hexadecimal forms are `A*.x=0x1.6eda5b90257aep-2`,
`B*.x=0x1.4892d237ed429p-1`, and
`A*.y=B*.y=0x1.6a0919b977760p-1`.  Thus the enclosure below applies to a
fully specified dyadic quadrilateral, not to unstated exact decimal
coordinates.

At 160-bit precision the evaluator encloses its normalized shortest-fence
value by

```text
[1.0496858140905033070 +/- 1.24e-20].
```

This strict lower bound supplies the competitor needed to turn a
`Phi <= 1.0496` certificate into an exclusion from global maximality.  Unit
tests check admissibility and this strict inequality, reproduce the six values,
and compare the parallel-safe pair formula with the apex formula away from
exact parallelism.

## Recorded Artifact Provenance

The historical assembled certificate
`flat_pair_full_w0000390625.jsonl` has SHA-256

```text
58e3a555c83b54e90676062092eaa988da5bf247435949fa663a229921ad8000
```

Its recorded C source is the source unchanged since commit
`5bcd9694c66cbeab03515c624225f7398c74bc82`; the final-refinement run was
documented in commit `c6dcd2dcebffad89d8143352f719bd6bfa83877b`.
The JSONL files are generated artifacts and are not stored in Git.  The
historical artifact has also been fully rechecked by the repaired verifier: all
799,546 leaves and the full dyadic cover pass unchanged.  A release intended to
support independent proof checking should archive the compressed certificate
beside a tagged repaired source revision and publish both hashes.  Regenerating
it is optional but would replace the legacy `nonoptimal` spelling with the
canonical `pair_incompatible` spelling; either way, record the exact verifier
revision used for the audit.
