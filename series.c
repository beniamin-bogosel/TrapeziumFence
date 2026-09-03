/* series.c -- enclosures of g(gamma)=gamma/sin(gamma) and g'(gamma). */
#include "series.h"

#define NC 18            /* number of cached Taylor coefficients a_0..a_{NC} */
#define COEFF_PREC 512   /* precision at which coefficients are stored */

static arb_t coeff[NC + 1];  /* a_0 .. a_NC as arb balls (exact-ish) */
static int coeff_ready = 0;

/* Compute a_n exactly as rationals via  a_n = -sum_{k=1}^n (-1)^k a_{n-k}/(2k+1)!
 * with a_0 = 1, then store as arb balls. */
static void series_build(void) {
    fmpq_t *a = flint_malloc((NC + 1) * sizeof(fmpq_t));
    fmpz_t fact, tmpz;
    fmpq_t term, acc;
    fmpz_init(fact); fmpz_init(tmpz);
    fmpq_init(term); fmpq_init(acc);
    for (int n = 0; n <= NC; n++) fmpq_init(a[n]);

    fmpq_one(a[0]);
    for (int n = 1; n <= NC; n++) {
        fmpq_zero(acc);
        for (int k = 1; k <= n; k++) {
            /* (2k+1)! */
            fmpz_fac_ui(fact, (ulong)(2 * k + 1));
            fmpq_set(term, a[n - k]);                 /* a_{n-k} */
            fmpq_div_fmpz(term, term, fact);          /* a_{n-k}/(2k+1)! */
            if (k % 2 == 1)                            /* (-1)^k, subtract handled below */
                fmpq_add(acc, acc, term);             /* +(-1)^k term with k odd => -; */
            else
                fmpq_sub(acc, acc, term);
        }
        /* a_n = -sum_{k}(-1)^k a_{n-k}/(2k+1)!.  We accumulated
         *   acc = sum_k [k odd ? +term : -term] = sum_k -(-1)^k term
         * so acc = -sum_k (-1)^k term = a_n directly. */
        fmpq_set(a[n], acc);
    }
    for (int n = 0; n <= NC; n++) {
        arb_init(coeff[n]);
        arb_set_fmpq(coeff[n], a[n], COEFF_PREC);
    }
    for (int n = 0; n <= NC; n++) fmpq_clear(a[n]);
    flint_free(a);
    fmpz_clear(fact); fmpz_clear(tmpz);
    fmpq_clear(term); fmpq_clear(acc);
    coeff_ready = 1;
}

static void ensure_coeffs(void) {
#if defined(_OPENMP)
    #pragma omp critical (trap_series_init)
#endif
    {
        if (!coeff_ready) series_build();
    }
}

void series_cleanup(void) {
    if (coeff_ready) {
        for (int n = 0; n <= NC; n++) arb_clear(coeff[n]);
        coeff_ready = 0;
    }
}

/* upper bound of gamma as an arb ball (thin, >= true sup). */
static void ghi_ball(arb_t gh, const arb_t gamma) {
    arf_t u;
    arf_init(u);
    arb_get_ubound_arf(u, gamma, COEFF_PREC);
    arb_set_arf(gh, u);
    arf_clear(u);
}

/* Extract the nonnegative part of gamma and prove that its upper endpoint is
 * strictly below pi.  The caller supplies the geometric invariant gamma >= 0,
 * so replacing a negative enclosure tail by zero is a sound intersection.
 * We deliberately do not clamp the upper endpoint to pi: if the input is not
 * proved to stay below a rigorous lower bound for pi, evaluation must fail
 * closed because sin(gamma) can vanish or change sign. */
static int nonnegative_below_pi(arf_t L, arf_t U, const arb_t gamma, slong prec) {
    arf_t PL;
    arf_init(PL);
    arb_get_interval_arf(L, U, gamma, prec);
    if (!arf_is_finite(L) || !arf_is_finite(U) || arf_sgn(U) < 0) {
        arf_clear(PL);
        return 0;
    }
    if (arf_sgn(L) < 0) arf_zero(L);
    {
        arb_t pit; arb_init(pit);
        arb_const_pi(pit, prec + 32);
        arb_get_lbound_arf(PL, pit, prec + 32);
        arb_clear(pit);
    }
    int ok = arf_cmp(L, U) <= 0 && arf_cmp(U, PL) < 0;
    arf_clear(PL);
    return ok;
}

/* Intersect with gamma >= 0 and require the Taylor domain gamma <= pi/2.
 * The comparison uses a rigorous lower bound for pi/2, never an inward clamp. */
static int nonnegative_half_pi(arb_t gn, const arb_t gamma, slong prec) {
    arf_t L, U, PH;
    arf_init(L); arf_init(U); arf_init(PH);
    int ok = nonnegative_below_pi(L, U, gamma, prec);
    if (ok) {
        arb_t pit; arb_init(pit);
        arb_const_pi(pit, prec + 32);
        arb_mul_2exp_si(pit, pit, -1);
        arb_get_lbound_arf(PH, pit, prec + 32);
        arb_clear(pit);
        ok = arf_cmp(U, PH) <= 0;
    }
    if (ok)
        arb_set_interval_arf(gn, L, U, prec);
    else
        arb_indeterminate(gn);
    arf_clear(L); arf_clear(U); arf_clear(PH);
    return ok;
}

/* Pure Taylor-series enclosure of g(gamma) with rigorous tail bound.
 * Valid for a nonnegative gamma enclosure proved to lie in [0,pi/2]; used for
 * the value near 0 and for the series-vs-direct cross-check test. */
void gamma_over_sin_series(arb_t res, const arb_t gamma, slong prec) {
    ensure_coeffs();
    arb_t gn, g2, pw, sum, gh, gh2, den, t;
    arb_init(gn); arb_init(g2); arb_init(pw); arb_init(sum);
    arb_init(gh); arb_init(gh2); arb_init(den); arb_init(t);

    if (!nonnegative_half_pi(gn, gamma, prec)) {
        arb_indeterminate(res);
        arb_clear(gn); arb_clear(g2); arb_clear(pw); arb_clear(sum);
        arb_clear(gh); arb_clear(gh2); arb_clear(den); arb_clear(t);
        return;
    }
    /* tight nonnegative square: [lo^2, hi^2] */
    {
        arf_t L, U;
        arf_init(L); arf_init(U);
        arb_get_interval_arf(L, U, gn, prec);
        arb_t al, au;
        arb_init(al); arb_init(au);
        arb_set_arf(al, L); arb_set_arf(au, U);
        arb_mul(al, al, al, prec);
        arb_mul(au, au, au, prec);
        arb_union(g2, al, au, prec);        /* [L^2, U^2] */
        arb_clear(al); arb_clear(au);
        arf_clear(L); arf_clear(U);
    }
    arb_one(sum);                            /* a_0 */
    arb_one(pw);
    for (int n = 1; n <= NC; n++) {
        arb_mul(pw, pw, g2, prec);           /* gamma^{2n} in [0, ghi^{2n}] */
        arb_addmul(sum, coeff[n], pw, prec);
    }
    /* tail sum_{n>NC} a_n g^{2n} <= a_{NC} ghi^{2(NC+1)} / (1 - ghi^2/6) */
    ghi_ball(gh, gn);
    arb_mul(gh2, gh, gh, prec);
    arb_one(t);
    for (int n = 0; n <= NC; n++) arb_mul(t, t, gh2, prec);  /* ghi^{2(NC+1)} */
    arb_mul(t, coeff[NC], t, prec);
    arb_div_si(den, gh2, 6, prec);
    arb_sub_si(den, den, 1, prec);
    arb_neg(den, den);                       /* 1 - ghi^2/6 */
    arb_div(t, t, den, prec);
    arb_mul_2exp_si(t, t, -1);
    arb_add_error(t, t);                     /* [0, tailbound] */
    arb_add(res, sum, t, prec);

    arb_clear(gn); arb_clear(g2); arb_clear(pw); arb_clear(sum);
    arb_clear(gh); arb_clear(gh2); arb_clear(den); arb_clear(t);
}

void gamma_over_sin_value(arb_t res, const arb_t gamma, slong prec) {
    /* g is strictly increasing on [0,pi).  Over a ball [lo,hi] known to
     * contain a nonnegative geometric angle, its range is [g(lo),g(hi)].
     * Endpoint evaluation avoids 0/0 at the origin. */
    arf_t lo, hi, Lout, Uout;
    arf_init(lo); arf_init(hi); arf_init(Lout); arf_init(Uout);
    if (!nonnegative_below_pi(lo, hi, gamma, prec)) {
        arb_indeterminate(res);
        arf_clear(lo); arf_clear(hi); arf_clear(Lout); arf_clear(Uout);
        return;
    }

    /* lower endpoint g(lo) */
    if (arf_is_zero(lo)) {
        arf_one(Lout);
    } else {
        arb_t a, s;
        arb_init(a); arb_init(s);
        arb_set_arf(a, lo);
        arb_sin(s, a, prec);
        if (arb_is_positive(s)) {
            arb_div(a, a, s, prec);
            arb_get_lbound_arf(Lout, a, prec);
        } else {
            arb_indeterminate(res);
            arb_clear(a); arb_clear(s);
            arf_clear(lo); arf_clear(hi); arf_clear(Lout); arf_clear(Uout);
            return;
        }
        arb_clear(a); arb_clear(s);
    }
    /* upper endpoint g(hi) */
    if (arf_is_zero(hi)) {
        arf_one(Uout);
    } else {
        arb_t a, s;
        arb_init(a); arb_init(s);
        arb_set_arf(a, hi);
        arb_sin(s, a, prec);
        if (arb_is_positive(s)) {
            arb_div(a, a, s, prec);
            arb_get_ubound_arf(Uout, a, prec);
        } else {
            arb_indeterminate(res);
            arb_clear(a); arb_clear(s);
            arf_clear(lo); arf_clear(hi); arf_clear(Lout); arf_clear(Uout);
            return;
        }
        arb_clear(a); arb_clear(s);
    }
    arb_set_interval_arf(res, Lout, Uout, prec);
    arf_clear(lo); arf_clear(hi); arf_clear(Lout); arf_clear(Uout);
}

void gamma_over_sin_deriv(arb_t res, const arb_t gamma, slong prec) {
    ensure_coeffs();
    arf_t glo, ghi;
    arf_init(glo); arf_init(ghi);
    if (!nonnegative_below_pi(glo, ghi, gamma, prec)) {
        arb_indeterminate(res);
        arf_clear(glo); arf_clear(ghi);
        return;
    }
    if (arf_sgn(glo) > 0) {
        /* g'(g) = (sin g - g cos g)/sin^2 g */
        arb_t s, c, num, den;
        arb_init(s); arb_init(c); arb_init(num); arb_init(den);
        arb_sin_cos(s, c, gamma, prec);
        if (arb_is_positive(s)) {
            arb_mul(num, gamma, c, prec);
            arb_sub(num, s, num, prec);     /* sin g - g cos g */
            arb_mul(den, s, s, prec);       /* sin^2 g */
            arb_div(res, num, den, prec);
        } else {
            arb_indeterminate(res);
        }
        arb_clear(s); arb_clear(c); arb_clear(num); arb_clear(den);
        arf_clear(glo); arf_clear(ghi);
        return;
    }
    /* If the interval includes zero, use the stable positive-term series only
     * in its proved domain [0,pi/2].  A wider interval fails closed; the
     * centered enclosure can then fall back to its natural enclosure. */
    arb_t gn, g2, pw, sum, gh;
    arb_init(gn); arb_init(g2); arb_init(pw); arb_init(sum); arb_init(gh);
    if (!nonnegative_half_pi(gn, gamma, prec)) {
        arb_indeterminate(res);
        arb_clear(gn); arb_clear(g2); arb_clear(pw); arb_clear(sum); arb_clear(gh);
        arf_clear(glo); arf_clear(ghi);
        return;
    }
    arf_clear(glo); arf_clear(ghi);
    /* series: g'(g) = sum_{n>=1} a_n (2n) g^{2n-1}, all terms >= 0 on [0,ghi]. */
    /* tight nonnegative square [lo^2, hi^2] */
    {
        arf_t L, U;
        arf_init(L); arf_init(U);
        arb_get_interval_arf(L, U, gn, prec);
        arb_t al, au;
        arb_init(al); arb_init(au);
        arb_set_arf(al, L); arb_set_arf(au, U);
        arb_mul(al, al, al, prec); arb_mul(au, au, au, prec);
        arb_union(g2, al, au, prec);
        arb_clear(al); arb_clear(au);
        arf_clear(L); arf_clear(U);
    }
    arb_zero(sum);
    arb_set(pw, gn);                        /* gamma^1 (>=0) */
    for (int n = 1; n <= NC; n++) {
        /* term_n = a_n * 2n * gamma^{2n-1}; pw currently gamma^{2n-1} */
        arb_t t;
        arb_init(t);
        arb_mul_si(t, coeff[n], 2 * n, prec);
        arb_mul(t, t, pw, prec);
        arb_add(sum, sum, t, prec);
        arb_mul(pw, pw, g2, prec);          /* -> gamma^{2n+1} */
        arb_clear(t);
    }
    /* The ratio of successive derivative terms is at most ghi^2/3.
     * Bounding the omitted tail by the last included term D_NC divided by
     * 1-ghi^2/3 is deliberately loose (it drops one ratio factor) but sound. */
    ghi_ball(gh, gn);
    {
        arb_t gh2, den, t, tail;
        arb_init(gh2); arb_init(den); arb_init(t); arb_init(tail);
        arb_mul(gh2, gh, gh, prec);
        /* D_NC = a_NC * 2*NC * ghi^{2NC-1}. */
        arb_one(t);
        for (int n = 1; n <= 2 * NC - 1; n++) arb_mul(t, t, gh, prec);  /* ghi^{2NC-1} */
        arb_mul_si(t, t, 2 * NC, prec);
        arb_mul(t, t, coeff[NC], prec);
        arb_div_si(den, gh2, 3, prec);
        arb_sub_si(den, den, 1, prec);
        arb_neg(den, den);                  /* 1 - ghi^2/3 */
        arb_div(t, t, den, prec);
        arb_set(tail, t);
        arb_mul_2exp_si(tail, tail, -1);
        arb_add_error(tail, tail);          /* [0, t] */
        arb_add(sum, sum, tail, prec);
        arb_clear(gh2); arb_clear(den); arb_clear(t); arb_clear(tail);
    }
    arb_set(res, sum);
    arb_clear(gn); arb_clear(g2); arb_clear(pw); arb_clear(sum); arb_clear(gh);
}

void jet_gamma_over_sin(jet *res, const jet *gamma, slong prec) {
    arb_t gval, gder;
    arb_init(gval); arb_init(gder);
    gamma_over_sin_value(gval, gamma->v, prec);
    gamma_over_sin_deriv(gder, gamma->v, prec);
    arb_set(res->v, gval);
    for (int k = 0; k < TRAP_NV; k++)
        arb_mul(res->d[k], gder, gamma->d[k], prec);
    arb_clear(gval); arb_clear(gder);
}
