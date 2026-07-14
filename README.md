# TrapeziumFence

Companion code for the paper

**The quadrilateral with the longest shortest fence**

by **Beniamin Bogosel**, **Dorin Bucur** and **Ilaria Fragala**.

Interval-arithmetic checker for the normalized longest-shortest-fence problem.

We normalize a convex quadrilateral by choosing a longest edge, naming it
`AB`, scaling it to length `1`, and placing
`A=(0,0)`, `B=(1,0)`.  Then
`C=(c1,c2)`, `D=(d1,d2)` and the normalized domain is

```
c1 in [0,2],  c2 in [0,1],  d1 in [-1,1],  d2 in [0,1],
convex/CCW, and |BC|, |CD|, |DA| <= 1.
```

The objective is `shortest_fence_length / sqrt(area(ABCD))`, so this
longest-edge normalization does not lose generality.

## What Is Certified

For each box in `(c1,c2,d1,d2)`, the program evaluates six rigorous upper
bounds for the normalized shortest fence:

- four vertex-angle bounds;
- two opposite-side pair bounds, using a parallel-safe formula near parallel
  side pairs.

Let `f = min(six upper bounds)`.  If interval arithmetic proves
`f_hi <= theta` on a box, then every admissible quadrilateral in that box has
normalized shortest fence value at most `theta`.  Such a box is written as
`"certified"`.

Two optional certificates are also available:

- `--flat-area-cert`: certifies low boxes using
  `L_AB,CD / sqrt(area) <= 2 sqrt(area)`;
- `--pair-eq-cert`: certifies `nonoptimal` boxes when the two opposite-pair
  fence intervals are disjoint, violating a necessary equality condition for
  an optimizer.

The output is therefore an exclusion certificate:

- `discarded`: the box contains no normalized admissible quadrilateral;
- `certified`: the shortest fence is proved `<= theta` throughout the box;
- `flat_area`: the shortest fence is proved `<= theta` by the small-area
  estimate;
- `nonoptimal`: no optimizer lies in the box by the opposite-pair equality
  certificate;
- `survivor`: unresolved at the requested width floor.

Thus the survivor union is a rigorous outer neighborhood of the possible
above-threshold optimizers.  When `--pair-eq-cert` is enabled, not every
eliminated box is necessarily certified low; some are eliminated as
nonoptimal.

## Build

Requires FLINT/arb, MPFR, and GMP.

```
make OMP=0      # deterministic single-threaded build
make            # OpenMP build if available
make test
```

## Run

Example:

```
./fence_validate --theta 1.04 --wfloor 0.05 --prec 53 \
  --form centered --serial --out cert_104.jsonl --stats

./fence_validate --verify cert_104.jsonl --prec 80 --serial

python3 analyze_cert.py cert_104.jsonl --theta 1.04
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
- `--pair-eq-cert`: apply the opposite-pair equality nonoptimality certificate.
- `--verify FILE`: independently re-checks metadata, local claims, and proves
  the JSONL leaves tile the full root box.  The certificate stores `theta`,
  `form`, `half`, and root-domain provenance; command-line values for those
  fields are optional cross-checks, not the source of the claim.  Incomplete
  coverage now fails immediately with the first missing dyadic child.
- `--assemble FILE... --out OUT`: replace survivor leaves through a refinement
  chain and write one full-domain certificate.
- `--max-leaves N`: debugging limit only; a truncated run will not pass full
  certificate coverage verification.

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
- Post-processing reports only certified-low regions and unresolved survivors.
  It does not treat lower bounds on `min(upper bounds)` as lower bounds for the
  true shortest fence.

## Reference Point

The conjectured isosceles trapezoid used for sanity checks is

```
C* = (0.6417451566, 0.7071006812)
D* = (0.3582548434, 0.7071006812)
f(T*) ~= 1.049685815
```

Unit tests reproduce the six bound values at this point and check the
parallel-safe pair formula against the apex formula away from exact
parallelism.
