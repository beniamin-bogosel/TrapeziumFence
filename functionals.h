/* functionals.h -- six scale-invariant fence candidates and their enclosures.
 *
 * Quadrilateral ABCD, CCW, with A=(0,0), B=(1,0), C=(c1,c2), D=(d1,d2).
 * Moduli coordinates (box axes): 0=c1, 1=c2, 2=d1, 3=d2.
 *
 * Six items are explicit fence-construction values.  Each candidate value is
 * an upper bound for the true shortest fence length / sqrt(area), so their
 * minimum certifies low boxes.  Items 4 and 5 have a stronger contract: they
 * enclose the exact closed-form values of the two opposite-pair constructions,
 * not arbitrary majorants.  The pair-equality incompatibility certificate
 * relies on this.
 *
 *   items 0..3 : vertex fence candidates sqrt(angle) at A, B, C, D;
 *   items 4,5  : exact opposite-pair fence candidates for (AB,CD), (BC,DA).
 * f = min of the six candidate values.
 *
 * Two enclosure modes over a box:
 *   FORM_NATURAL  : direct arb extension (overestimation O(w));
 *   FORM_CENTERED : mean-value form  item(mid) + sum_k J_k(B) (B_k - mid_k),
 *                   with the Jacobian rows J_k obtained by forward-mode AD
 *                   (the jet type in geom.h).  On smooth boxes away from
 *                   singularities its overestimation is O(w^2).  No quadratic
 *                   rate is claimed otherwise; a nonfinite centered form falls
 *                   back to the natural enclosure.
 *   The centered and natural results are combined conservatively: intersection
 *   when they overlap, the finite one if only one is finite, and otherwise
 *   their hull.
 *
 * For CCW-oriented opposite sides d1,d2, gamma is the aperture of the sector
 * containing the quadrilateral:
 *   gamma = atan2(|cross(d1,d2)|, -dot(d1,d2)) in [0,pi].
 * It can be obtuse.  The pair candidates switch between two mathematically
 * identical formulas according to the acute separation of the supporting lines:
 *   Form1 (apex O intersection)      away from parallel;
 *   Form2 (parallel-safe, symmetric) near parallel.
 * See functionals.c for the derivation; the choice affects only numerical
 * conditioning, never soundness.
 */
#ifndef TRAP_FUNCTIONALS_H
#define TRAP_FUNCTIONALS_H

#include "geom.h"

typedef enum { FORM_NATURAL = 0, FORM_CENTERED = 1 } enclosure_form;

#define GAMMA_SPLIT 0.08   /* acute line-separation threshold selecting Form1/Form2 */

/* Enclose all six candidate values over the box [lo,hi]^4 into encl[0..5]. */
void functionals_eval(arb_t encl[6], const double lo[4], const double hi[4],
                      enclosure_form form, slong prec);

/* Pair-equality incompatibility certificate.  Returns 1 iff the interval
 * enclosures for items 4=(AB,CD) and 5=(BC,DA) are finite and disjoint over
 * the whole box, proving that it contains no quadrilateral satisfying the
 * imposed equality of these two exact pair-construction values.
 *
 * Soundness depends on items 4 and 5 enclosing the exact pair-construction
 * values.  Disjointness of two unrelated upper majorants would not certify
 * incompatibility.  The equality is not a universal optimizer condition: in
 * the proof using this code it is supplied analytically in the active
 * P={P1,P2} cases. */
int functionals_opposite_pairs_disjoint(const double lo[4], const double hi[4],
                                        enclosure_form form, slong prec);

/* Compute the minimum of the finite candidate enclosures and return the index
 * (0..5) delivering its smallest upper endpoint.  That endpoint is always a
 * rigorous one-sided upper bound for the true six-item minimum.  The whole
 * fenc interval encloses the minimum only when all six items are finite; its
 * lower endpoint is otherwise diagnostic.  If no item is finite, fenc is +inf. */
int functionals_min(arb_t fenc, const double lo[4], const double hi[4],
                    enclosure_form form, slong prec);

/* Test hook: one pair candidate at a point, forcing the formula.
 * pair: 0=(AB,CD), 1=(BC,DA).  force: 0=Form1(apex), 1=Form2(parallel-safe). */
void functionals_pair_forced(arb_t out, const double coord[4], int pair,
                             int force, slong prec);

#endif /* TRAP_FUNCTIONALS_H */
