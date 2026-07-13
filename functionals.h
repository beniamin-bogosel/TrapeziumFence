/* functionals.h -- the six scale-invariant fence bounds and their enclosures.
 *
 * Quadrilateral ABCD, CCW, with A=(0,0), B=(1,0), C=(c1,c2), D=(d1,d2).
 * Moduli coordinates (box axes): 0=c1, 1=c2, 2=d1, 3=d2.
 *
 * Six items (each an upper bound for fence_length / sqrt(area)):
 *   items 0..3 : vertex bounds  sqrt(angle) at A, B, C, D;
 *   items 4,5  : opposite-pair bounds for (AB,CD) and (BC,DA).
 * f = min of the six.
 *
 * Two enclosure modes over a box:
 *   FORM_NATURAL  : direct arb extension (overestimation O(w));
 *   FORM_CENTERED : mean-value form  item(mid) + sum_k J_k(B) (B_k - mid_k),
 *                   with the Jacobian rows J_k obtained by forward-mode AD
 *                   (the jet type in geom.h).  Overestimation O(w^2).
 *   The centered result is intersected with the natural one (both sound).
 *
 * The pair bounds switch between two mathematically identical formulas:
 *   Form1 (apex O intersection)     when inf(gamma) >  GAMMA_SPLIT;
 *   Form2 (parallel-safe, symmetric) when inf(gamma) <= GAMMA_SPLIT.
 * See functionals.c for the derivation; the choice affects only numerical
 * conditioning, never soundness.
 */
#ifndef TRAP_FUNCTIONALS_H
#define TRAP_FUNCTIONALS_H

#include "geom.h"

typedef enum { FORM_NATURAL = 0, FORM_CENTERED = 1 } enclosure_form;

#define GAMMA_SPLIT 0.08   /* inf(gamma) threshold selecting Form1 vs Form2 */

/* Enclose all six item bounds over the box [lo,hi]^4 into encl[0..5]. */
void functionals_eval(arb_t encl[6], const double lo[4], const double hi[4],
                      enclosure_form form, slong prec);

/* Necessary optimality certificate: at a true optimum the two opposite-pair
 * fence candidates must be equal.  Returns 1 iff the interval enclosures for
 * items 4=(AB,CD) and 5=(BC,DA) are finite and disjoint over the whole box.
 * Such a box contains no optimal quadrilateral, though it need not be
 * certified below a numerical threshold theta. */
int functionals_opposite_pairs_disjoint(const double lo[4], const double hi[4],
                                        enclosure_form form, slong prec);

/* Enclose f = min of the six items over the box.  Writes the min-enclosure to
 * fenc and returns the index (0..5) of the item delivering the smallest upper
 * bound (the "active" item that certifies the box when fenc's ubound <= theta).
 */
int functionals_min(arb_t fenc, const double lo[4], const double hi[4],
                    enclosure_form form, slong prec);

/* Test hook: one pair bound at a point, forcing the formula.
 * pair: 0=(AB,CD), 1=(BC,DA).  force: 0=Form1(apex), 1=Form2(parallel-safe). */
void functionals_pair_forced(arb_t out, const double coord[4], int pair,
                             int force, slong prec);

#endif /* TRAP_FUNCTIONALS_H */
