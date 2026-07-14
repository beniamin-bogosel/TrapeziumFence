/* threshold.h -- exact decimal threshold helpers. */
#ifndef TRAP_THRESHOLD_H
#define TRAP_THRESHOLD_H

#include "geom.h"

#if defined(ARB_IN_FLINT)
#include <flint/fmpq.h>
#else
#include <fmpq.h>
#endif

/* Parse a finite decimal literal, including optional scientific notation, as
 * the exact rational represented by the characters.  Returns 0 on success. */
int threshold_parse_fmpq(fmpq_t q, const char *s);

/* Convert an exact threshold literal to an Arb ball. */
int threshold_set_arb(arb_t out, const char *s, slong prec);

/* Exact comparisons against the decimal literal.  Return nonzero only when
 * the comparison is proved. */
int threshold_arf_leq(const arf_t x, const char *theta_str);
int threshold_area_ub_leq(const arf_t area_ub, const char *theta_str);

/* Display/heuristic conversion only; never use this for certification. */
double threshold_to_double(const char *s);

#endif /* TRAP_THRESHOLD_H */
