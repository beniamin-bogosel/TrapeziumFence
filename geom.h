/* geom.h -- arithmetic primitives for the fence validator.
 *
 * Provides:
 *   - the FLINT/arb includes (layout-independent);
 *   - a forward-mode automatic-differentiation "jet" over arb balls, carrying a
 *     value and its 4 partial derivatives wrt the moduli coordinates
 *         index 0 = c1, 1 = c2, 2 = d1, 3 = d2;
 *   - jet arithmetic (all rigorous arb enclosures) used to build the six
 *     functionals once and obtain both their values and Jacobian rows;
 *   - scalar arb geometry helpers (cross, dot, shoelace area, squared distance)
 *     used by the admissibility predicates.
 *
 * SOUNDNESS: every operation is an outward-rounded arb enclosure.  No ball's
 * radius is ever widened away or discarded.
 */
#ifndef TRAP_GEOM_H
#define TRAP_GEOM_H

#ifdef ARB_IN_FLINT
#include <flint/arb.h>
#include <flint/arf.h>
#include <flint/fmpq.h>
#include <flint/fmpz.h>
#else
#include <arb.h>
#include <arf.h>
#include <fmpq.h>
#include <fmpz.h>
#endif

#define TRAP_NV 4   /* number of moduli coordinates: c1,c2,d1,d2 */

/* A jet: value v and partials d[0..3] = d/dc1, d/dc2, d/dd1, d/dd2. */
typedef struct {
    arb_t v;
    arb_t d[TRAP_NV];
} jet;

/* A 2D point built from two jets. */
typedef struct {
    jet x;
    jet y;
} jpt;

/* ---- jet lifecycle ---- */
void jet_init(jet *a);
void jet_clear(jet *a);
void jet_set(jet *r, const jet *a);             /* r = a */

/* Seed a variable: value = val, d[k] = 1, other partials 0. */
void jet_set_var(jet *r, const arb_t val, int k);
/* Constant: value = c, all partials 0. */
void jet_set_const(jet *r, const arb_t c);
void jet_set_si(jet *r, slong c);

/* ---- jet arithmetic (prec = working precision) ---- */
void jet_add(jet *r, const jet *a, const jet *b, slong prec);
void jet_sub(jet *r, const jet *a, const jet *b, slong prec);
void jet_neg(jet *r, const jet *a);
void jet_mul(jet *r, const jet *a, const jet *b, slong prec);
void jet_div(jet *r, const jet *a, const jet *b, slong prec);
void jet_sqr(jet *r, const jet *a, slong prec);
void jet_sqrt(jet *r, const jet *a, slong prec);
void jet_abs(jet *r, const jet *a, slong prec);          /* |a|, straddle-safe */
void jet_atan2(jet *r, const jet *y, const jet *x, slong prec);
void jet_mul_si(jet *r, const jet *a, slong c, slong prec);
/* r = a scaled by a scalar arb s (s treated as a constant, no partials). */
void jet_mul_arb(jet *r, const jet *a, const arb_t s, slong prec);

/* ---- point/vector helpers on jets ---- */
void jpt_init(jpt *p);
void jpt_clear(jpt *p);
void jpt_sub(jpt *r, const jpt *a, const jpt *b, slong prec);   /* r = a - b */
void jet_cross(jet *r, const jpt *a, const jpt *b, slong prec); /* a.x*b.y - a.y*b.x */
void jet_dot(jet *r, const jpt *a, const jpt *b, slong prec);   /* a.x*b.x + a.y*b.y */

/* ---- scalar arb geometry (for admissibility) ---- */
/* cross/dot of two vectors given as raw arb components. */
void trap_cross(arb_t r, const arb_t ax, const arb_t ay,
                const arb_t bx, const arb_t by, slong prec);
void trap_dot(arb_t r, const arb_t ax, const arb_t ay,
              const arb_t bx, const arb_t by, slong prec);

#endif /* TRAP_GEOM_H */
