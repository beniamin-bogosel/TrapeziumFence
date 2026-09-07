# Serial MATLAB/INTLAB certification

This is the readable INTLAB implementation used for the final, fully verified
centered certification chain. The FLINT/Arb implementation is unchanged.
There are no parallel workers and no Parallel Computing Toolbox requirement.

The quadrilateral is normalized as
`D=(0,0), C=(1,0), B=(b1,b2), A=(a1,a2)`. Boxes use coordinate order
`(b1,b2,a1,a2)`, with root `[0,2] × [0,1] × [-1,1] × [0,1]`.
The C implementation's historical coordinate names differ, not their order.

## One command for the whole process

From MATLAB:

```matlab
cd('/home/beni/python/TrapeziumFence/Intlab')
summary = certification_process;
```

This runs the base search and all six refinements, then independently verifies
the complete chain. **Existing completed stage files are reused, not overwritten.**
If the seven reference files already exist, no search is repeated, but full
verification is still performed. Explicit verification never trusts a cached
success flag.

For a genuinely fresh computation, use a new output directory:

```matlab
summary = certification_process('Runs/centered');
```

To defer verification, or perform it later:

```matlab
summary = certification_process('Runs/centered', 'compute');
summary = certification_process('Runs/centered', 'verify');
```

Use `[]` instead of the directory name to operate on the files in `Intlab`.
No manual `sourceVerified=true` acknowledgment is needed in this driver.
`compute` checks coverage but does **not** claim independent mathematical
verification; only successful `all` or `verify` sets `summary.verified=true`.

The configured INTLAB installation is `/home/beni/INTLAB/Intlab_V13`. To use
another installation without editing code:

```matlab
summary = certification_process('my_run', 'all', [], '/path/to/Intlab_V13');
```

The driver prints the current machine and an approximate local completion
time based on the reference timings below. It restores MATLAB's original
working directory on completion or error.

## Reference machine and expected time

Reference machine: **Intel Core i7-9750H @ 2.60 GHz, 6 physical cores /
12 logical threads, 32 GB installed RAM (31.1 GiB usable), Linux Mint 22.1,
Linux 6.11.0-29-generic, MATLAB R2024a (24.1.0.2537033), INTLAB 13**.
MATLAB version and the detected CPU,
usable memory and operating system are saved by the new driver for each run.
All searches and checks are serial.

Allow approximately **658 minutes (10 h 58 min) for computation**, followed by
**164 minutes (2 h 44 min) for full verification**: approximately **822 minutes
(13 h 42 min) end-to-end** on the reference machine. These are planning
estimates, not deadlines or predictions scaled automatically to other CPUs.
Thermal throttling, other work, software versions and interval behavior matter.

The old JSONL files do not contain search runtimes. The stage estimates below
are the elapsed windows between each file's filesystem creation timestamp and
its final write, rounded to the nearest minute. They are **not measured MATLAB
search timers** and could include pauses. Unlike differences between successive
files' completion times, they exclude the gaps before the next file was
created. New runs save actual `run_search` elapsed seconds in timing sidecars.

The verification time **9851.8 seconds**, rounded to 164 minutes, is the
user-reported measured total for the complete seven-file replay on
7 September 2026. The successful output was:

```text
Verified 54597 leaves and the complete cover of 8347 seed box(es).
Verified the complete 7-file chain; final survivors: 13488.
Total verification-chain elapsed: 9851.8s.
```

## Recorded centered stages

All stages use `theta='1.0496'`, `form='centered'`, the full root domain,
and the current short-edge, flat-area, geometric, P2V0, pair-equality and
active-pair/vertex rules. The filename prefix is
`intlab_centered_pair_vertex_w`; append the suffix below and `.jsonl`.

| Stage | Width floor | Filename suffix | Input seeds | Terminal leaves | Survivors | Estimated computation (min) |
|---|---:|---|---:|---:|---:|---:|
| Base | 0.025 | `0025` | 1 | 109,200 | 21,488 | 187 |
| Refinement 1 | 0.0125 | `00125` | 21,488 | 89,646 | 6,973 | 130 |
| Refinement 2 | 0.00625 | `000625` | 6,973 | 34,322 | 4,857 | 56 |
| Refinement 3 | 0.003125 | `0003125` | 4,857 | 28,170 | 5,073 | 49 |
| Refinement 4 | 0.0015625 | `00015625` | 5,073 | 30,830 | 6,085 | 56 |
| Refinement 5 | 0.00078125 | `000078125` | 6,085 | 38,288 | 8,347 | 76 |
| Refinement 6 | 0.000390625 | `0000390625` | 8,347 | 54,597 | 13,488 | 104 |

There are **385,053 leaf records across the seven files**. This includes
intermediate survivors subsequently refined; it is not the leaf count of an
assembled final partition. More survivors at a finer stage need not mean a
larger unresolved region: each cell is smaller.

### Outer rectangles at each stage

The hulls are coordinatewise extrema of all stored survivor boxes. The
following decimal endpoints are exact dyadic values, not inward-rounded
display approximations. The two hulls forget correlations between A and B;
not every point in their product belongs to a survivor.

| Width floor | A x-range | A y-range | B x-range | B y-range |
|---:|---|---|---|---|
| 0.025 | [-0.28125, 0.46875] | [0.25, 1] | [0.53125, 1.28125] | [0.234375, 1] |
| 0.0125 | [0.09375, 0.4375] | [0.5625, 0.84375] | [0.5625, 0.90625] | [0.5546875, 0.8359375] |
| 0.00625 | [0.2421875, 0.3984375] | [0.6328125, 0.77734375] | [0.6015625, 0.7578125] | [0.62890625, 0.7734375] |
| 0.003125 | [0.296875, 0.3828125] | [0.662109375, 0.748046875] | [0.6171875, 0.703125] | [0.662109375, 0.748046875] |
| 0.0015625 | [0.326171875, 0.373046875] | [0.677734375, 0.7353515625] | [0.626953125, 0.673828125] | [0.677734375, 0.7353515625] |
| 0.00078125 | [0.3408203125, 0.369140625] | [0.6865234375, 0.7275390625] | [0.630859375, 0.6591796875] | [0.68603515625, 0.72705078125] |
| 0.000390625 | [0.3486328125, 0.3662109375] | [0.692138671875, 0.721923828125] | [0.6337890625, 0.6513671875] | [0.692138671875, 0.721923828125] |

### Exclusion counts

These categories are disjoint within a stage. `Other` collects inadmissibility,
short-edge and flat-area exclusions.

| Stage | Explicit fence below threshold | Geometric below threshold | P2V0 | Active pair/vertex incompatible | Other |
|---|---:|---:|---:|---:|---:|
| Base | 24,360 | 16,503 | 12,100 | 30,950 | 3,799 |
| Refinement 1 | 30,532 | 7,855 | 7,344 | 36,428 | 514 |
| Refinement 2 | 6,174 | 5,425 | 0 | 17,866 | 0 |
| Refinement 3 | 3,141 | 3,979 | 0 | 15,977 | 0 |
| Refinement 4 | 3,656 | 3,661 | 0 | 17,428 | 0 |
| Refinement 5 | 5,055 | 4,222 | 0 | 20,664 | 0 |
| Refinement 6 | 7,399 | 5,338 | 0 | 28,372 | 0 |

### Comparison with the recorded FLINT/Arb chain

| Width floor | INTLAB survivors | FLINT/Arb survivors |
|---:|---:|---:|
| 0.025 | 21,488 | 69,787 |
| 0.0125 | 6,973 | 25,031 |
| 0.00625 | 4,857 | 12,302 |
| 0.003125 | 5,073 | 9,006 |
| 0.0015625 | 6,085 | 8,238 |
| 0.00078125 | 8,347 | 11,018 |
| 0.000390625 | 13,488 | 18,525 |

The final FLINT x-ranges are A = [0.349609375, 0.3662109375] and
B = [0.6337890625, 0.64990234375], with the same y-ranges as INTLAB.
INTLAB has 27.2% fewer final cells but slightly wider horizontal hulls.
**It has not reproduced the paper's exact FLINT horizontal rectangles.**
The rule sets differ: INTLAB includes additional analytic exclusions.
FLINT's recorded computation is much faster; see [../RUN_RESULTS.md](../RUN_RESULTS.md).
Neither survivor count alone nor agreement of hulls proves a certificate.

## Progress, files and restart behavior

Search progress is printed every **100 leaves**. `classified` includes
internal boxes as well as terminal leaves. `pending` is a depth-first stack,
not a percentage of the remaining work. Verification also prints every 100
records, with an ETA for the **current file only**. The driver's separate
reference-based finish estimate concerns the remaining process.

New output uses `filename.jsonl.partial` until the search and its structural
coverage check both finish. Only then is it renamed to `filename.jsonl`.
No existing completed certificate is overwritten. On restarting, a leftover
partial stage is moved to `Archive/Interrupted/` in the output directory,
and that stage is recomputed from its input seeds. Completed stages are reused
after exact path/endpoint coverage checks. This is a **stage checkpoint**, not
resumption from a saved internal search stack. An incomplete file already
named `.jsonl` is rejected, not automatically discarded.

Only run one process per output directory. Use `Ctrl+C` to stop; rerun the
same command to continue at the first incomplete stage. Keep all seven final
JSONL files together. Each refinement covers only its predecessor's survivors.

Generated files:

- `*_stats.mat`: actual search seconds, counts, file size and machine for each
  newly computed stage. Legacy files without timings retain `NaN`, not
  fabricated measurements.
- `certification_summary.mat`: stage counts, exact endpoint hulls, available
  timings, machine information and whether this invocation fully verified.
- `verification_reports.mat`: reports from a successfully completed full
  verification. Reports and timing sidecars are diagnostics, not substitutes
  for replay; explicit verification always rechecks the chain.

The driver refreshes its summary and reports. Successful verification is
indicated by the final success message and `summary.verified=true`, not merely
by a file existing. After `compute`, run the same directory with `'verify'`.

For a quick, complete but deliberately coarse test of the driver:

```matlab
smoke = certification_process('Runs/smoke', 'all', [1,0.5]);
```

Custom schedules have separate filenames and `paperSchedule=false`. They
do not certify the paper's final neighborhood.

## Code map

| Task | Files |
|---|---|
| Single process entry point | `certification_process.m` |
| Search / refinement from a saved file | `run_search.m`, `refine_survivors.m` |
| Child coverage / scheduling | `root_box.m`, `split_box.m`, `scaled_box_width.m` |
| Classification | `categorize_box.m`, `certainly_inadmissible.m` |
| Six-fence enclosures | `evaluate_fences.m`, `quadrilateral_area.m`, `interval_atan2_nonnegative.m`, `gamma_over_sin_interval.m` |
| Cheap analytic exclusions | `short_edge_parameters.m`, `short_edge_certifies_nonoptimal.m`, `geometric_exclusion.m`, `p2v0_exclusion.m`, `active_pair_vertex_exclusion.m` |
| Certificate output | `open_certificate.m`, `write_certificate_leaf.m`, `close_certificate.m` |
| Independent verification | `verify_certificate.m`, `verify_seeded_certificate.m`, `verify_refinement_chain.m`, `canonical_certificate_status.m`, `certificate_record_count.m` |
| Analysis / reference competitor | `analyze_certificate.m`, `demo_reference_trapezium.m` |
| Setup / defaults | `setup_intlab.m`, `default_options.m` |
| Proof explanations | [Theory/](Theory/README.md), [LaTeX note](Theory/survivor_elimination.tex) |
| Regression tests | `Tests/` |

The mathematical routines are unchanged by this cleanup. Their natural/hybrid
compatibility paths and `pair5_interval_derivative.m` remain because the
shared evaluator, historical certificate replay and regression tests use
them. The production driver fixes the full-centered mode; it does not select
a hybrid or grid experiment.

Old grid/quadrant variants, historical benchmarks, previous orchestration
wrappers, superseded output files and previous documentation are preserved
under **`Archive/`**, ignored by git. Do not use `addpath(genpath(...))` on
this tree: archived code is not part of the active implementation.

Run the bounded regression suite from `Intlab`:

```matlab
addpath('Tests')
run_tests
```

## Certification meaning and safeguards

- Every box stores double-precision lower and upper endpoints. Both children
  share exactly the same split point; no inward-rounded child reconstruction
  can leave a coverage gap. This matches the stored-box strategy in FLINT.
- Mathematical predicates use outward-rounded INTLAB arithmetic. Threshold
  comparisons use the conservative endpoint of `intval('1.0496')`.
- Natural evaluations precede full centered bounds. Centering intersects the
  natural enclosure with `f(midpoint) + sum D_k f(box)*(box_k-midpoint_k)`.
  Displacement is formed by interval subtraction; nonfinite derivatives fall
  back conservatively. A consistency failure is not silently accepted.
- `short_edge`, `p2v0_nonoptimal`, and `active_pair_vertex_incompatible`
  invoke documented analytic reductions. They need not prove a fence below
  `theta`. In particular, with normalized fence-length intervals `I1..I6`,
  the pair intersection `J=I5 intersect I6` must meet at least one vertex
  interval at a maximizer after the independent P0/P1/P2V0 reductions.
  The test never assumes two active vertices or the P2V1 conclusion that uses
  Proposition 17. See the [proof note](Theory/survivor_elimination.tex).
- `certified`, `geometric_low`, and `flat_area` prove the appropriate
  threshold bound. `pair_incompatible` and `rational_pair_incompatible`
  require the recorded opposite-pair equality premise.
- `survivor` means unresolved, not a confirmed maximizer or a lower bound.
  Verification checks survivor width and coverage; it need not reevaluate
  every fence on a box that makes no exclusion claim.
- Schema 8 records the enabled analytic rules. The verifier reconstructs
  claims from endpoints, never trusts stored witnesses, and checks source
  links and complete coverage. The general verifier still supports schemas
  4–7; the single production driver deliberately rejects different settings
  in its reference chain.

The verified conclusion localizes possible maximizers to the final survivor
union, subject to the stated analytic reductions. It does not prove uniqueness
inside that union, nor silently replace INTLAB's hulls with FLINT's tighter ones.
