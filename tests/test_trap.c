/* test_trap.c -- unit tests for the fence validator.
 *
 * (a) T* enclosure reproduction (six items + f);
 * (b) both pair forms agree at gamma ~ 0.1;
 * (c) gamma/sin gamma series vs direct at gamma = 0.05, 0.1, 0.3;
 * (d) a known-inadmissible box is discarded;
 * (e) a deep-interior sub-threshold box certifies with a sensible active item;
 * (f) disjoint opposite-pair intervals certify nonoptimality;
 * (g) the flat-area estimate certifies small-area boxes.
 */
#include "functionals.h"
#include "series.h"
#include "admissible.h"
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

static void test_Tstar(slong prec) {
    printf("(a) T* reproduction\n");
    double c1=0.6417451566, c2=0.7071006812, d1=0.3582548434, d2=0.7071006812;
    double lo[4]={c1,c2,d1,d2}, hi[4]={c1,c2,d1,d2};
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

static void test_series(slong prec) {
    printf("(c) gamma/sin gamma series vs direct\n");
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
}

static void test_inadmissible(slong prec) {
    printf("(d) known-inadmissible box is discarded\n");
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
    printf("(e) deep-interior sub-threshold box certifies\n");
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
    printf("(f) opposite-pair equality certificate\n");
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
    printf("(g) flat-area certificate\n");
    double theta = 1.0496;
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
