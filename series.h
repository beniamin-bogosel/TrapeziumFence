/* series.h -- rigorous enclosures of g(gamma) = gamma / sin(gamma) and its
 * derivative near gamma = 0, plus the jet wrapper used by the parallel-safe
 * pair bound.
 *
 * g is real-analytic on (-pi, pi) with the even Taylor series
 *     gamma/sin(gamma) = 1 + gamma^2/6 + 7 gamma^4/360 + 31 gamma^6/15120 + ...
 *     = sum_{n>=0} a_n gamma^{2n},   a_0 = 1,  a_n > 0.
 * The coefficient ratios a_{n+1}/a_n decrease monotonically from a_1/a_0 = 1/6
 * to the limit 1/pi^2, so a_{n+1}/a_n <= 1/6 for all n, giving the rigorous tail
 *     sum_{n>=N} a_n g^{2n} <= a_N g^{2N} / (1 - g^2/6)   for g^2 < 6.
 * The Taylor implementation is restricted to gamma in [0,pi/2], where
 * gamma^2/6 <= (pi/2)^2/6 < 0.42.  Values and derivatives away from zero are
 * evaluated directly on [0,pi); derivatives on intervals touching zero use
 * the Taylor form.  This supports obtuse containing sectors.  Callers supply
 * the geometric invariant that the true angle is nonnegative, so a small
 * negative enclosure tail may be intersected with zero.  An input not proved
 * to stay below pi fails closed instead of being clamped across the pole of
 * 1/sin(gamma).
 */
#ifndef TRAP_SERIES_H
#define TRAP_SERIES_H

#include "geom.h"

/* Enclose g(gamma) = gamma/sin(gamma) for a quantity independently known to be
 * nonnegative whose ball has upper endpoint below pi.  A negative enclosure
 * tail caused by dependency/rounding is intersected with zero. */
void gamma_over_sin_value(arb_t res, const arb_t gamma, slong prec);
/* Pure Taylor-series enclosure (with rigorous tail); used by the cross-check
 * test against the direct form.  The represented quantity must be known
 * nonnegative and the ball's upper endpoint must not exceed pi/2. */
void gamma_over_sin_series(arb_t res, const arb_t gamma, slong prec);
/* Enclose g'(gamma) under the same nonnegativity invariant, below pi. */
void gamma_over_sin_deriv(arb_t res, const arb_t gamma, slong prec);
/* Jet version: res = g(gamma) with res.d[k] = g'(gamma) * gamma.d[k]. */
void jet_gamma_over_sin(jet *res, const jet *gamma, slong prec);

/* Free the cached Taylor coefficients (optional; call at shutdown). */
void series_cleanup(void);

#endif /* TRAP_SERIES_H */
