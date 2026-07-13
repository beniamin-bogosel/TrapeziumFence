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

/* gn = gamma clamped to [0, pi/2].  Since gamma = atan2(nonneg, nonneg) the true
 * value always lies in [0, pi/2], so intersecting the enclosure with this range
 * is sound; it also keeps hi < pi (so g(hi)=hi/sin(hi) stays positive and the
 * series tail bound, valid for |gamma| <= pi/2, applies).  A wide box can make
 * arb_atan2 return an enclosure up to ~pi near the origin; this clamp tames it. */
static void clamp_nonneg(arb_t gn, const arb_t gamma, slong prec) {
    arf_t L, U, PH;
    arf_init(L); arf_init(U); arf_init(PH);
    arb_get_interval_arf(L, U, gamma, prec);
    /* upper bound for pi/2 */
    {
        arb_t pit; arb_init(pit);
        arb_const_pi(pit, prec + 8);
        arb_mul_2exp_si(pit, pit, -1);
        arb_get_ubound_arf(PH, pit, prec + 8);
        arb_clear(pit);
    }
    if (arf_sgn(L) < 0) arf_zero(L);
    if (arf_cmp(U, PH) > 0) arf_set(U, PH);
    if (arf_cmp(L, U) > 0) arf_set(L, U);   /* degenerate guard */
    arb_set_interval_arf(gn, L, U, prec);
    arf_clear(L); arf_clear(U); arf_clear(PH);
}

/* Pure Taylor-series enclosure of g(gamma) with rigorous tail bound.
 * Valid for any gamma with |gamma| <= pi/2; used for the value near 0 and, as a
 * standalone routine, for the series-vs-direct cross-check test. */
void gamma_over_sin_series(arb_t res, const arb_t gamma, slong prec) {
    ensure_coeffs();
    arb_t gn, g2, pw, sum, gh, gh2, den, t;
    arb_init(gn); arb_init(g2); arb_init(pw); arb_init(sum);
    arb_init(gh); arb_init(gh2); arb_init(den); arb_init(t);

    clamp_nonneg(gn, gamma, prec);
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
    ensure_coeffs();
    /* g is even and strictly increasing in |gamma| on (-pi,pi).  Over a ball
     * [lo,hi] with true gamma>=0 the exact range is [g(lo), g(hi)].  Evaluate
     * the two endpoints (g(0)=1) and assemble [lbound(g(lo)), ubound(g(hi))].
     * This monotone enclosure is exact and sound for thin and wide balls alike
     * and never forms 0/0. */
    arb_t gn;
    arb_init(gn);
    clamp_nonneg(gn, gamma, prec);
    arf_t lo, hi, Lout, Uout;
    arf_init(lo); arf_init(hi); arf_init(Lout); arf_init(Uout);
    arb_get_interval_arf(lo, hi, gn, prec);

    /* lower endpoint g(lo) */
    if (arf_is_zero(lo)) {
        arf_one(Lout);
    } else {
        arb_t a, s;
        arb_init(a); arb_init(s);
        arb_set_arf(a, lo);
        arb_sin(s, a, prec);
        arb_div(a, a, s, prec);
        arb_get_lbound_arf(Lout, a, prec);
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
        arb_div(a, a, s, prec);
        arb_get_ubound_arf(Uout, a, prec);
        arb_clear(a); arb_clear(s);
    }
    arb_set_interval_arf(res, Lout, Uout, prec);
    arf_clear(lo); arf_clear(hi); arf_clear(Lout); arf_clear(Uout);
    arb_clear(gn);
}

void gamma_over_sin_deriv(arb_t res, const arb_t gamma, slong prec) {
    ensure_coeffs();
    if (arb_is_positive(gamma)) {
        /* g'(g) = (sin g - g cos g)/sin^2 g */
        arb_t s, c, num, den;
        arb_init(s); arb_init(c); arb_init(num); arb_init(den);
        arb_sin_cos(s, c, gamma, prec);
        arb_mul(num, gamma, c, prec);
        arb_sub(num, s, num, prec);         /* sin g - g cos g */
        arb_mul(den, s, s, prec);           /* sin^2 g */
        arb_div(res, num, den, prec);
        arb_clear(s); arb_clear(c); arb_clear(num); arb_clear(den);
        return;
    }
    /* series: g'(g) = sum_{n>=1} a_n (2n) g^{2n-1}, all terms >= 0 on [0,ghi]. */
    arb_t gn, g2, pw, sum, gh;
    arb_init(gn); arb_init(g2); arb_init(pw); arb_init(sum); arb_init(gh);
    clamp_nonneg(gn, gamma, prec);
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
    /* tail: term_{NC+1}/(1 - ghi^2/3), positive. */
    ghi_ball(gh, gn);
    {
        arb_t gh2, den, t, tail;
        arb_init(gh2); arb_init(den); arb_init(t); arb_init(tail);
        arb_mul(gh2, gh, gh, prec);
        /* term_{NC+1} = a_{NC} * 2*NC * ghi^{2NC-1}  (use NC as index bound) */
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
