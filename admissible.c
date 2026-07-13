/* admissible.c -- certainly-inadmissible test over a box. */
#include "admissible.h"

static void set_box(arb_t x, double lo, double hi, slong prec) {
    arf_t L, U;
    arf_init(L); arf_init(U);
    arf_set_d(L, lo); arf_set_d(U, hi);
    arb_set_interval_arf(x, L, U, prec);
    arf_clear(L); arf_clear(U);
}

/* squared length of vector (ux,uy). */
static void sqlen(arb_t r, const arb_t ux, const arb_t uy, slong prec) {
    arb_t t;
    arb_init(t);
    arb_mul(r, ux, ux, prec);
    arb_mul(t, uy, uy, prec);
    arb_add(r, r, t, prec);
    arb_clear(t);
}

int box_small_area_certifies_low(const double lo[4], const double hi[4],
                                 double theta, slong prec) {
    arb_t c1, c2, d1, d2, area, t1, t2;
    arb_init(c1); arb_init(c2); arb_init(d1); arb_init(d2);
    arb_init(area); arb_init(t1); arb_init(t2);
    set_box(c1, lo[0], hi[0], prec);
    set_box(c2, lo[1], hi[1], prec);
    set_box(d1, lo[2], hi[2], prec);
    set_box(d2, lo[3], hi[3], prec);

    /* A = (c2 + c1*d2 - d1*c2)/2.  If ub(A) <= theta^2/4, then for every
     * admissible quadrilateral in the box the flat-area certificate gives
     * L/sqrt(A) <= 2 sqrt(A) <= theta. */
    arb_mul(t1, c1, d2, prec);
    arb_mul(t2, d1, c2, prec);
    arb_add(area, c2, t1, prec);
    arb_sub(area, area, t2, prec);
    arb_mul_2exp_si(area, area, -1);

    arf_t u;
    arf_init(u);
    arb_get_ubound_arf(u, area, prec);
    int ok = arb_is_finite(area) && arf_is_finite(u)
             && arf_cmp_d(u, 0.25 * theta * theta) <= 0;

    arf_clear(u);
    arb_clear(c1); arb_clear(c2); arb_clear(d1); arb_clear(d2);
    arb_clear(area); arb_clear(t1); arb_clear(t2);
    return ok;
}

int box_certainly_inadmissible(const double lo[4], const double hi[4],
                               int half, slong prec) {
    arb_t c1, c2, d1, d2, one;
    arb_init(c1); arb_init(c2); arb_init(d1); arb_init(d2); arb_init(one);
    set_box(c1, lo[0], hi[0], prec);
    set_box(c2, lo[1], hi[1], prec);
    set_box(d1, lo[2], hi[2], prec);
    set_box(d2, lo[3], hi[3], prec);
    arb_one(one);

    int bad = 0;

    /* edge vectors: AB=(1,0), BC=(c1-1,c2), CD=(d1-c1,d2-c2), DA=(-d1,-d2). */
    arb_t bcx, bcy, cdx, cdy, dax, day, cr, t;
    arb_init(bcx); arb_init(bcy); arb_init(cdx); arb_init(cdy);
    arb_init(dax); arb_init(day); arb_init(cr); arb_init(t);
    arb_sub_si(bcx, c1, 1, prec);           /* c1-1 */
    arb_set(bcy, c2);
    arb_sub(cdx, d1, c1, prec);
    arb_sub(cdy, d2, c2, prec);
    arb_neg(dax, d1);
    arb_neg(day, d2);

    /* convex/CCW cross products (must all be > 0):
     *   cross(AB,BC) = c2
     *   cross(BC,CD) = bcx*cdy - bcy*cdx
     *   cross(CD,DA) = cdx*day - cdy*dax   ( = c1 d2 - d1 c2 )
     *   cross(DA,AB) = d2
     * discard if any is certainly <= 0 (entirely negative over the box). */
    if (!bad && arb_is_negative(c2)) bad = 1;
    if (!bad && arb_is_negative(d2)) bad = 1;
    if (!bad) {
        arb_mul(cr, bcx, cdy, prec);
        arb_mul(t, bcy, cdx, prec);
        arb_sub(cr, cr, t, prec);
        if (arb_is_negative(cr)) bad = 1;
    }
    if (!bad) {
        arb_mul(cr, cdx, day, prec);
        arb_mul(t, cdy, dax, prec);
        arb_sub(cr, cr, t, prec);
        if (arb_is_negative(cr)) bad = 1;
    }

    /* distance / longest-side constraints (all squared, must be <= 1):
     *   |BC|^2 = bcx^2 + bcy^2   (= |C-B|^2, and AB>=BC)
     *   |CD|^2 = cdx^2 + cdy^2   (= |C-D|^2, and AB>=CD)
     *   |DA|^2 = d1^2 + d2^2     (= |D-A|^2, and AB>=DA)
     * discard if any is certainly > 1. */
    if (!bad) {
        sqlen(cr, bcx, bcy, prec);
        if (arb_gt(cr, one)) bad = 1;
    }
    if (!bad) {
        sqlen(cr, cdx, cdy, prec);
        if (arb_gt(cr, one)) bad = 1;
    }
    if (!bad) {
        sqlen(cr, d1, d2, prec);
        if (arb_gt(cr, one)) bad = 1;
    }

    /* optional half-symmetry: require c1 + d1 >= 1; discard if certainly < 1. */
    if (!bad && half) {
        arb_add(t, c1, d1, prec);
        if (arb_lt(t, one)) bad = 1;
    }

    arb_clear(bcx); arb_clear(bcy); arb_clear(cdx); arb_clear(cdy);
    arb_clear(dax); arb_clear(day); arb_clear(cr); arb_clear(t);
    arb_clear(c1); arb_clear(c2); arb_clear(d1); arb_clear(d2); arb_clear(one);
    return bad;
}
