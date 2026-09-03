/* test_trap.c -- unit tests for the fence validator.
 *
 * (a) T* enclosure reproduction (six items + f);
 * (b) both pair forms agree at gamma ~ 0.1;
 * (c) both pair forms use the obtuse sector containing an admissible quad;
 * (d) gamma/sin gamma series vs direct at gamma = 0.05, 0.1, 0.3;
 * (e) a known-inadmissible box is discarded;
 * (f) a deep-interior sub-threshold box certifies with a sensible active item;
 * (g) disjoint opposite-pair intervals certify pair-equality incompatibility;
 * (h) the flat-area estimate certifies small-area boxes.
 */
#include "functionals.h"
#include "series.h"
#include "admissible.h"
#include "threshold.h"
#include <stdio.h>
#include <math.h>

static int failures = 0;
static void ok(int cond, const char *msg) {
    printf("  [%s] %s\n", cond ? "PASS" : "FAIL", msg);
    if (!cond) failures++;
}

/* upper/lower bound of ball as double */
static double ub(const arb_t x) {
    arf_t u; arf_init(u); arb_get_ubound_arf(u, x, 64);
    double d = arf_get_d(u, ARF_RND_UP); arf_clear(u); return d;
}
static double lb(const arb_t x) {
    arf_t u; arf_init(u); arb_get_lbound_arf(u, x, 64);
    double d = arf_get_d(u, ARF_RND_DOWN); arf_clear(u); return d;
}
/* is the whole enclosure within tol of v?  (The reference C*,D* are only 10-digit,
 * so the evaluated items sit ~1e-9 from the exact L*; tol absorbs that.) */
static int approx(const arb_t x, double v, double tol) {
    return fabs(ub(x) - v) <= tol && fabs(lb(x) - v) <= tol;
}

/* Prove all strict convexity and longest-side conditions at a point using
 * outward-rounded Arb arithmetic. */
static int point_provably_admissible(const double coord[4], slong prec) {
    arb_t c1, c2, d1, d2, bcx, bcy, cdx, cdy, dax, day, cr, len2, one;
    arb_init(c1); arb_init(c2); arb_init(d1); arb_init(d2);
    arb_init(bcx); arb_init(bcy); arb_init(cdx); arb_init(cdy);
    arb_init(dax); arb_init(day); arb_init(cr); arb_init(len2); arb_init(one);
    arb_set_d(c1, coord[0]); arb_set_d(c2, coord[1]);
    arb_set_d(d1, coord[2]); arb_set_d(d2, coord[3]);
    arb_sub_si(bcx, c1, 1, prec); arb_set(bcy, c2);
    arb_sub(cdx, d1, c1, prec); arb_sub(cdy, d2, c2, prec);
    arb_neg(dax, d1); arb_neg(day, d2); arb_one(one);

    int good = arb_is_positive(c2) && arb_is_positive(d2);
    trap_cross(cr, bcx, bcy, cdx, cdy, prec);
    good = good && arb_is_positive(cr);
    trap_cross(cr, cdx, cdy, dax, day, prec);
    good = good && arb_is_positive(cr);
    trap_dot(len2, bcx, bcy, bcx, bcy, prec);
    good = good && arb_lt(len2, one);
    trap_dot(len2, cdx, cdy, cdx, cdy, prec);
    good = good && arb_lt(len2, one);
    trap_dot(len2, dax, day, dax, day, prec);
    good = good && arb_lt(len2, one);

    arb_clear(c1); arb_clear(c2); arb_clear(d1); arb_clear(d2);
    arb_clear(bcx); arb_clear(bcy); arb_clear(cdx); arb_clear(cdy);
    arb_clear(dax); arb_clear(day); arb_clear(cr); arb_clear(len2); arb_clear(one);
    return good;
}

/* Compare an outward-rounded lower endpoint to an exact decimal rational. */
static int lower_bound_gt_decimal(const arb_t x, const char *decimal, slong prec) {
    arf_t lower;
    fmpq_t lhs, rhs;
    arf_init(lower); fmpq_init(lhs); fmpq_init(rhs);
    arb_get_lbound_arf(lower, x, prec);
    arf_get_fmpq(lhs, lower);
    int proved = !threshold_parse_fmpq(rhs, decimal) && fmpq_cmp(lhs, rhs) > 0;
    arf_clear(lower); fmpq_clear(lhs); fmpq_clear(rhs);
    return proved;
}

static void test_Tstar(slong prec) {
    printf("(a) T* reproduction\n");
    double c1=0.6417451566, c2=0.7071006812, d1=0.3582548434, d2=0.7071006812;
    double coord[4]={c1,c2,d1,d2};
    double lo[4]={c1,c2,d1,d2}, hi[4]={c1,c2,d1,d2};
    ok(point_provably_admissible(coord, prec),
       "the explicit reference quadrilateral is rigorously admissible");
    arb_t e[6]; for (int i=0;i<6;i++) arb_init(e[i]);
    functionals_eval(e, lo, hi, FORM_NATURAL, prec);
    ok(approx(e[0], 1.0496858154873, 1e-7) && approx(e[1], 1.0496858154873, 1e-7),
       "base-corner vertex bounds ~ L* = 1.0496858154873");
    ok(approx(e[2], 1.428198985, 1e-7) && approx(e[3], 1.428198985, 1e-7),
       "top-corner vertex bounds ~ 1.428198985");
    ok(approx(e[4], 1.0496858154873, 1e-7) && approx(e[5], 1.0496858154873, 1e-7),
       "both pair bounds ~ L*");
    arb_t f; arb_init(f);
    int act = functionals_min(f, lo, hi, FORM_NATURAL, prec);
    ok(approx(f, 1.0496858154873, 1e-7), "f = min ~ L*");
    ok(lower_bound_gt_decimal(f, "1.0496", prec),
       "the reference value is rigorously greater than exact theta=1.0496");
    ok(act == 4 || act == 5, "active item is a pair bound at T*");
    for (int i=0;i<6;i++) arb_clear(e[i]);
    arb_clear(f);
}

static void test_pair_forms(slong prec) {
    printf("(b) pair forms agree at gamma ~ 0.1\n");
    double t = tan(0.1);
    double coord[4] = {0.75, 0.7, 0.25, 0.7 + 0.5*t};   /* CD ~ 0.1 rad off AB */
    arb_t f1, f2, d; arb_init(f1); arb_init(f2); arb_init(d);
    functionals_pair_forced(f1, coord, 0, 0, prec);   /* Form1 apex */
    functionals_pair_forced(f2, coord, 0, 1, prec);   /* Form2 parallel-safe */
    arb_sub(d, f1, f2, prec);
    arb_abs(d, d);
    ok(ub(d) < 1e-10, "Form1 vs Form2 agree to < 1e-10");
    ok(arb_overlaps(f1, f2), "Form1 and Form2 enclosures overlap");
    arb_clear(f1); arb_clear(f2); arb_clear(d);
}

static void test_obtuse_pair_forms(slong prec) {
    printf("(c) pair forms use the obtuse containing sector\n");
    /* ABCD is strictly convex, CCW, and has AB as a longest side.  For the
     * pair (BC,DA), the containing sector has aperture
     * atan2(0.36,-0.04) = 1.68145... > pi/2.  Using the acute line angle would
     * incorrectly give the much smaller value 1.22050... . */
    double coord[4] = {0.5, 0.4, 0.4, 0.4};
    ok(point_provably_admissible(coord, prec),
       "obtuse-sector test quadrilateral is rigorously admissible");

    arb_t f1, f2, d; arb_init(f1); arb_init(f2); arb_init(d);
    functionals_pair_forced(f1, coord, 1, 0, prec);   /* Form1 apex */
    functionals_pair_forced(f2, coord, 1, 1, prec);   /* Form2 parallel-safe */
    ok(approx(f1, 1.3097413128223512, 1e-10),
       "Form1 uses the obtuse containing-sector aperture");
    ok(approx(f2, 1.3097413128223512, 1e-10),
       "Form2 supports gamma/sin(gamma) for an obtuse aperture");
    arb_sub(d, f1, f2, prec); arb_abs(d, d);
    ok(ub(d) < 1e-10 && arb_overlaps(f1, f2),
       "forced apex and parallel-safe forms agree in the obtuse case");

    arb_t e[6]; for (int i=0;i<6;i++) arb_init(e[i]);
    functionals_eval(e, coord, coord, FORM_NATURAL, prec);
    ok(approx(e[5], 1.3097413128223512, 1e-10),
       "automatic formula selection preserves the obtuse pair value");
    for (int i=0;i<6;i++) arb_clear(e[i]);

    double lo[4] = {0.4999, 0.3999, 0.3999, 0.3999};
    double hi[4] = {0.5001, 0.4001, 0.4001, 0.4001};
    arb_t natural[6], centered[6];
    for (int i=0;i<6;i++) { arb_init(natural[i]); arb_init(centered[i]); }
    functionals_eval(natural, lo, hi, FORM_NATURAL, prec);
    functionals_eval(centered, lo, hi, FORM_CENTERED, prec);
    ok(arb_contains(natural[5], f1),
       "natural obtuse-sector box enclosure contains its midpoint value");
    ok(arb_contains(centered[5], f1),
       "centered obtuse-sector box enclosure contains its midpoint value");
    for (int i=0;i<6;i++) { arb_clear(natural[i]); arb_clear(centered[i]); }

    /* A thin admissible quadrilateral for which (BC,DA) has nearly parallel
     * supporting lines but an aperture close to pi.  This forces the automatic
     * path through the parallel-safe form at the other end of its domain. */
    double near_pi[4] = {0.5, 0.01, 0.4, 0.01};
    ok(point_provably_admissible(near_pi, prec),
       "near-pi-sector test quadrilateral is rigorously admissible");
    functionals_pair_forced(f1, near_pi, 1, 0, prec);
    functionals_pair_forced(f2, near_pi, 1, 1, prec);
    ok(approx(f1, 1.7774020682956107, 1e-10) &&
       approx(f2, 1.7774020682956107, 1e-10),
       "forced pair forms agree for a containing sector near pi");
    arb_sub(d, f1, f2, prec); arb_abs(d, d);
    ok(ub(d) < 1e-10 && arb_overlaps(f1, f2),
       "near-pi forced pair enclosures overlap");
    arb_t near_items[6];
    for (int i=0;i<6;i++) arb_init(near_items[i]);
    functionals_eval(near_items, near_pi, near_pi, FORM_NATURAL, prec);
    ok(approx(near_items[5], 1.7774020682956107, 1e-10),
       "automatic evaluation preserves the near-pi pair value");
    for (int i=0;i<6;i++) arb_clear(near_items[i]);

    arb_clear(f1); arb_clear(f2); arb_clear(d);
}

static void test_series(slong prec) {
    printf("(d) gamma/sin gamma series vs direct\n");
    double gs[3] = {0.05, 0.1, 0.3};
    for (int i=0;i<3;i++) {
        arb_t g, s, dir, sn; arb_init(g); arb_init(s); arb_init(dir); arb_init(sn);
        arb_set_d(g, gs[i]);
        gamma_over_sin_series(s, g, prec);
        arb_sin(sn, g, prec);
        arb_div(dir, g, sn, prec);                 /* direct gamma/sin gamma */
        char msg[64]; snprintf(msg, sizeof msg, "series overlaps direct at gamma=%.2f", gs[i]);
        ok(arb_overlaps(s, dir), msg);
        arb_t diff; arb_init(diff); arb_sub(diff, s, dir, prec); arb_abs(diff, diff);
        snprintf(msg, sizeof msg, "series matches direct to <1e-12 at gamma=%.2f", gs[i]);
        ok(ub(diff) < 1e-12, msg);
        arb_clear(diff);
        arb_clear(g); arb_clear(s); arb_clear(dir); arb_clear(sn);
    }

    arb_t g, value, deriv;
    arb_init(g); arb_init(value); arb_init(deriv);
    arb_set_d(g, 0.3);
    gamma_over_sin_deriv(deriv, g, prec);
    double expected_deriv = (sin(0.3) - 0.3*cos(0.3)) /
                            (sin(0.3)*sin(0.3));
    ok(approx(deriv, expected_deriv, 1e-12),
       "gamma/sin(gamma) derivative matches its analytic value");

    arf_t lo, hi;
    arf_init(lo); arf_init(hi);
    arf_zero(lo); arf_set_d(hi, 0.1);
    arb_set_interval_arf(g, lo, hi, prec);
    gamma_over_sin_deriv(deriv, g, prec);
    double deriv_at_point_one = (sin(0.1) - 0.1*cos(0.1)) /
                                (sin(0.1)*sin(0.1));
    ok(arb_is_finite(deriv) && lb(deriv) <= 0.0 &&
       ub(deriv) >= deriv_at_point_one,
       "derivative series encloses the full interval touching zero");

    arb_set_d(g, 2.0);
    gamma_over_sin_value(value, g, prec);
    gamma_over_sin_deriv(deriv, g, prec);
    ok(arb_is_finite(value) && arb_is_finite(deriv),
       "direct value and derivative support an obtuse gamma");
    gamma_over_sin_series(value, g, prec);
    ok(!arb_is_finite(value), "Taylor evaluator rejects gamma > pi/2");

    arf_set_d(lo, 3.1); arf_set_d(hi, 3.2);
    arb_set_interval_arf(g, lo, hi, prec);
    gamma_over_sin_value(value, g, prec);
    gamma_over_sin_deriv(deriv, g, prec);
    ok(!arb_is_finite(value) && !arb_is_finite(deriv),
       "gamma/sin(gamma) value and derivative fail closed across pi");
    arf_clear(lo); arf_clear(hi);
    arb_clear(g); arb_clear(value); arb_clear(deriv);
}

static void test_inadmissible(slong prec) {
    printf("(e) known-inadmissible box is discarded\n");
    /* C far outside disk(B,1): c1 in [3,3.1] => |C-B| >> 1 */
    double lo[4] = {3.0, 0.4, 0.3, 0.4};
    double hi[4] = {3.1, 0.5, 0.4, 0.5};
    ok(box_certainly_inadmissible(lo, hi, 0, prec), "far-away C box is certainly inadmissible");
    /* c2 strictly negative => not CCW/convex */
    double lo2[4] = {0.6, -0.5, 0.3, 0.4};
    double hi2[4] = {0.7, -0.4, 0.4, 0.5};
    ok(box_certainly_inadmissible(lo2, hi2, 0, prec), "negative-c2 box is certainly inadmissible");
    /* a genuinely admissible small box near the optimum is NOT discarded */
    double lo3[4] = {0.64, 0.70, 0.35, 0.70};
    double hi3[4] = {0.65, 0.71, 0.36, 0.71};
    ok(!box_certainly_inadmissible(lo3, hi3, 0, prec), "near-optimum box is not discarded");
}

static void test_interior_certify(slong prec) {
    printf("(f) deep-interior sub-threshold box certifies\n");
    /* A flatter symmetric trapezoid well below the optimum: f should be < 1.04. */
    double c1=0.65, c2=0.45, d1=0.35, d2=0.45, h=0.01;
    double lo[4]={c1-h,c2-h,d1-h,d2-h}, hi[4]={c1+h,c2+h,d1+h,d2+h};
    ok(!box_certainly_inadmissible(lo, hi, 0, prec), "test box is admissible");
    arb_t f; arb_init(f);
    int act = functionals_min(f, lo, hi, FORM_CENTERED, prec);
    printf("      f enclosure = "); arb_printn(f, 12, 0); printf("  active item=%d\n", act);
    ok(ub(f) <= 1.04, "f_hi <= theta=1.04 (box certifies)");
    ok(act >= 0 && act <= 5, "a well-defined active item delivered the bound");
    arb_clear(f);
}

static void test_pair_equality_certificate(slong prec) {
    printf("(g) opposite-pair equality certificate\n");
    double tlo[4] = {0.6417450566, 0.7071005812, 0.3582547434, 0.7071005812};
    double thi[4] = {0.6417452566, 0.7071007812, 0.3582549434, 0.7071007812};
    ok(!functionals_opposite_pairs_disjoint(tlo, thi, FORM_NATURAL, prec),
       "small T* box is not rejected by pair-equality certificate");

    double lo[4] = {0.8, 0.6, 0.2, 0.7};
    double hi[4] = {0.8, 0.6, 0.2, 0.7};
    ok(!box_certainly_inadmissible(lo, hi, 0, prec), "asymmetric test point is admissible");
    ok(functionals_opposite_pairs_disjoint(lo, hi, FORM_NATURAL, prec),
       "asymmetric point has disjoint opposite-pair intervals");
}

static void test_flat_area_certificate(slong prec) {
    printf("(h) flat-area certificate\n");
    const char *theta = "1.0496";
    double lo[4] = {0.79, 0.09, 0.19, 0.09};
    double hi[4] = {0.81, 0.11, 0.21, 0.11};
    ok(!box_certainly_inadmissible(lo, hi, 0, prec), "flat test box is admissible");
    ok(box_small_area_certifies_low(lo, hi, theta, prec),
       "flat box has area upper bound <= theta^2/4");

    double tlo[4] = {0.6417450566, 0.7071005812, 0.3582547434, 0.7071005812};
    double thi[4] = {0.6417452566, 0.7071007812, 0.3582549434, 0.7071007812};
    ok(!box_small_area_certifies_low(tlo, thi, theta, prec),
       "small T* box is not eliminated by flat-area certificate");
}

int main(void) {
    slong prec = 90;
    printf("=== fence validator unit tests ===\n");
    test_Tstar(prec);
    test_pair_forms(prec);
    test_obtuse_pair_forms(prec);
    test_series(prec);
    test_inadmissible(prec);
    test_interior_certify(prec);
    test_pair_equality_certificate(prec);
    test_flat_area_certificate(prec);
    series_cleanup();
    printf("=== %s (%d failure%s) ===\n",
           failures ? "TESTS FAILED" : "ALL TESTS PASSED",
           failures, failures==1?"":"s");
    return failures ? 1 : 0;
}
