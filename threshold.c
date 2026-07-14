/* threshold.c -- exact decimal threshold helpers. */
#define _POSIX_C_SOURCE 200809L
#include "threshold.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#if defined(ARB_IN_FLINT)
#include <flint/fmpz.h>
#else
#include <fmpz.h>
#endif

static int parse_exp10(const char **pp, long *out) {
    const char *p = *pp;
    int sign = 1;
    long v = 0;
    int any = 0;
    if (*p == '+' || *p == '-') {
        if (*p == '-') sign = -1;
        p++;
    }
    while (isdigit((unsigned char)*p)) {
        int d = *p - '0';
        if (v > (LONG_MAX - d) / 10) return 1;
        v = 10 * v + d;
        any = 1;
        p++;
    }
    if (!any) return 1;
    *out = sign * v;
    *pp = p;
    return 0;
}

int threshold_parse_fmpq(fmpq_t q, const char *s) {
    if (!s) return 1;
    const char *p = s;
    while (isspace((unsigned char)*p)) p++;

    int neg = 0;
    if (*p == '+' || *p == '-') {
        neg = (*p == '-');
        p++;
    }

    size_t cap = strlen(p) + 2;
    char *digits = malloc(cap);
    if (!digits) return 1;
    size_t nd = 0;
    long frac = 0;
    int seen_dot = 0;

    while (isdigit((unsigned char)*p) || *p == '.') {
        if (*p == '.') {
            if (seen_dot) { free(digits); return 1; }
            seen_dot = 1;
            p++;
            continue;
        }
        if (nd + 1 >= cap) { free(digits); return 1; }
        digits[nd++] = *p++;
        if (seen_dot) {
            if (frac == LONG_MAX) { free(digits); return 1; }
            frac++;
        }
    }
    if (nd == 0) { free(digits); return 1; }
    digits[nd] = '\0';

    long exp10 = 0;
    if (*p == 'e' || *p == 'E') {
        p++;
        if (parse_exp10(&p, &exp10)) { free(digits); return 1; }
    }
    while (isspace((unsigned char)*p)) p++;
    if (*p != '\0') { free(digits); return 1; }

    fmpz_t pow10;
    fmpz_init(pow10);
    int fail = 0;
    if (fmpz_set_str(fmpq_numref(q), digits, 10) != 0) {
        fail = 1;
    } else {
        if (neg) fmpz_neg(fmpq_numref(q), fmpq_numref(q));
        long shift = exp10 - frac;
        if (shift >= 0) {
            fmpz_ui_pow_ui(pow10, 10, (ulong)shift);
            fmpz_mul(fmpq_numref(q), fmpq_numref(q), pow10);
            fmpz_one(fmpq_denref(q));
        } else {
            unsigned long den_exp = (unsigned long)(-shift);
            fmpz_ui_pow_ui(fmpq_denref(q), 10, (ulong)den_exp);
        }
        if (!fail) fmpq_canonicalise(q);
    }
    fmpz_clear(pow10);
    free(digits);
    return fail;
}

int threshold_set_arb(arb_t out, const char *s, slong prec) {
    fmpq_t q;
    fmpq_init(q);
    int fail = threshold_parse_fmpq(q, s);
    if (!fail) arb_set_fmpq(out, q, prec);
    fmpq_clear(q);
    return fail;
}

int threshold_arf_leq(const arf_t x, const char *theta_str) {
    if (!arf_is_finite(x)) return 0;
    fmpq_t lhs, rhs;
    fmpq_init(lhs);
    fmpq_init(rhs);
    arf_get_fmpq(lhs, x);
    int ok = !threshold_parse_fmpq(rhs, theta_str) &&
             fmpq_cmp(lhs, rhs) <= 0;
    fmpq_clear(lhs);
    fmpq_clear(rhs);
    return ok;
}

int threshold_area_ub_leq(const arf_t area_ub, const char *theta_str) {
    if (!arf_is_finite(area_ub)) return 0;
    fmpq_t lhs, theta, limit;
    fmpq_init(lhs);
    fmpq_init(theta);
    fmpq_init(limit);
    arf_get_fmpq(lhs, area_ub);
    int ok = 0;
    if (!threshold_parse_fmpq(theta, theta_str)) {
        fmpq_mul(limit, theta, theta);
        fmpq_div_2exp(limit, limit, 2);
        ok = fmpq_cmp(lhs, limit) <= 0;
    }
    fmpq_clear(lhs);
    fmpq_clear(theta);
    fmpq_clear(limit);
    return ok;
}

double threshold_to_double(const char *s) {
    if (!s) return 0.0;
    errno = 0;
    return strtod(s, NULL);
}
