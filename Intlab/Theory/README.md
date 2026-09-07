# Proof notes for the production INTLAB classifier

The mathematical proofs and their manuscript dependencies are in
[survivor_elimination.tex](survivor_elimination.tex). The active production
driver is [../certification_process.m](../certification_process.m); its
[README](../README.md) contains the final stage results, hulls and timings.

## Geometric rule identifiers

These IDs belong to `geometric_low` and `rational_pair_incompatible`, not
the six-fence numbering used by `certified`.

| Item | Rule |
|---|---|
| 1 | Vertical bisecting chord, sharp 16-corner residual |
| 2–4 | Chords normal to CB, BA, AD |
| 5–7 | Area-dependent short CB, BA, AD |
| 8–9 | Polynomial vertex D, C |
| 10–11 | Vertex B, A with constrained quadratic minimization |
| 12–13 | Pair-5, pair-6 angle restriction |
| 14–15 | Rational pair-5, pair-6 threshold bound |
| 16 | Rational pair incompatibility after cancelling common area (conditional) |

Items 1–15 require a finite residual upper bound at most zero. Item 16 requires
a strictly negative upper endpoint and the pair-equality option. A
corner/minimization witness need not enclose the full residual range. Every
claim is recomputed by the verifier, not accepted from its witness. Invalid
denominators and nonfinite evaluations are inconclusive. Bounds apply to the
admissible subset; no stored box is contracted.

The separate `short_edge` rule uses the safe decimal epsilon `0.212` at
`theta='1.0496'`. Startup validates its sufficient inequality rigorously.

## Independent active-set reductions

The inexpensive `p2v0_nonoptimal` prefilter proves all four vertex fences
strictly longer than an upper bound for one opposite-pair fence. Lemma 12,
the independent P2V0 exclusions in Sections 4–5 and Remark 19 exclude a
maximizer in such a cell. It does not need pair equality and does not claim
a value below the numerical threshold. Its item is the bounding pair, 5 or 6.

The stronger `active_pair_vertex_incompatible` rule uses normalized fence
**length** intervals, with the two pairs indexed by 5 and 6. After the
independent P0/P1/P2V0 reductions, a maximizer must have both pairs and at
least one vertex active. Therefore `J=I5 intersect I6` must be nonempty and
meet at least one of `I1..I4`. Every vertex being disjoint, whether above or
below J, excludes maximality. Endpoint contact is inconclusive. Unknown
bounds fail closed; an independently empty J needs no vertex bounds.

This rule is enabled separately from `pairEqualityCertificate`. Schema 8
records its flag and `P0_P1_P2V0_sections4_5_remark19_v1` reduction identifier.
The comparison adds no derivatives itself, but benefits from the
production evaluator's full centered bounds for all six candidates.

Do **not** require two active vertex fences: the supplied manuscript's
Section 4 P2V1 / V={C} argument invokes Proposition 17 in Step 6(ii).
Using that conclusion inside the numerical proof of Proposition 17 would
be circular. The implemented rule retains the single-active-vertex branch.

## Tests and archived research

From `Intlab`, add `Tests` to the path and run `run_tests`. Tests cover
nonzero-width witnesses, invalid metadata, forged claims, touching
intervals, the reference trapezium, child coverage and full certificate
replay.

Historical grid/quadrant experiments and saved-survivor benchmarks are
preserved locally under `../Archive/`, which is ignored by git. They are
not dependencies of the production computation. Their old measurements and
usage examples are retained in the archived READMEs, not mixed with the
current reference results.

The later contractor/active-incidence proposals in the LaTeX note remain
research ideas. The numerical predicates and their analytic premises were
not changed by the cleanup or the new process driver.
