/* admissible.h -- interval admissibility test for a box of quadrilaterals.
 *
 * A box (c1,c2,d1,d2 ranges) is DISCARDED only if some admissibility predicate
 * is *certainly* violated over the whole box (the interval proves it false).
 * If a predicate merely straddles its constraint the box is KEPT: the six
 * functionals remain valid upper bounds on the admissible sub-part, so a
 * straddling box that still certifies below theta is sound to reject.
 *
 * Admissible (A=(0,0), B=(1,0), C=(c1,c2), D=(d1,d2)) iff all hold:
 *   - convex & CCW: all four consecutive-edge cross products > 0;
 *   - c2 > 0, d2 > 0;
 *   - |C-B| <= 1 and |D-A| <= 1 and |C-D| <= 1   (squared);
 *   - AB longest: 1 >= |BC|, |CD|, |DA|          (squared);
 *   - (optional half-symmetry) c1 + d1 >= 1.
 */
#ifndef TRAP_ADMISSIBLE_H
#define TRAP_ADMISSIBLE_H

#include "geom.h"

/* Returns 1 if the box is certainly inadmissible (safe to DISCARD), else 0. */
int box_certainly_inadmissible(const double lo[4], const double hi[4],
                               int half, slong prec);

/* Low-fence certificate for flat boxes.  For admissible quadrilaterals,
 * the explicit AB-to-CD fence satisfies L/sqrt(A) <= 2 sqrt(A).  Therefore a
 * box is certified below theta if its interval upper bound for area A is at
 * most the exact rational theta^2/4, where theta is parsed from theta_str. */
int box_small_area_certifies_low(const double lo[4], const double hi[4],
                                 const char *theta_str, slong prec);

#endif /* TRAP_ADMISSIBLE_H */
