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
 * Since gamma in [0, pi/2] always, g^2/6 <= (pi/2)^2/6 < 0.42, so the bound is
 * valid throughout.  The value enclosure is additionally intersected with the
 * unconditional monotonicity enclosure [1, ghi/sin(ghi)] (g is increasing on
 * (0,pi)); that intersection is sound regardless of the series tail argument.
 */
#ifndef TRAP_SERIES_H
#define TRAP_SERIES_H

#include "geom.h"

/* Enclose g(gamma) = gamma/sin(gamma) over the ball `gamma`. */
void gamma_over_sin_value(arb_t res, const arb_t gamma, slong prec);
/* Pure Taylor-series enclosure (with rigorous tail); used by the cross-check
 * test against the direct form.  Valid for |gamma| <= pi/2. */
void gamma_over_sin_series(arb_t res, const arb_t gamma, slong prec);
/* Enclose g'(gamma) = (sin g - g cos g)/sin^2 g over the ball `gamma`. */
void gamma_over_sin_deriv(arb_t res, const arb_t gamma, slong prec);
/* Jet version: res = g(gamma) with res.d[k] = g'(gamma) * gamma.d[k]. */
void jet_gamma_over_sin(jet *res, const jet *gamma, slong prec);

/* Free the cached Taylor coefficients (optional; call at shutdown). */
void series_cleanup(void);

#endif /* TRAP_SERIES_H */
