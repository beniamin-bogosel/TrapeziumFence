/* search.h -- dyadic subdivision search over the moduli box.
 *
 * Refine-until-below-threshold:
 *   DISCARD   box certainly inadmissible;
 *   CERTIFY   f_hi <= theta  (shortest fence is proved <= theta on the box);
 *   FLAT      area upper bound <= theta^2/4, so the flat-area fence estimate
 *             L/sqrt(A) <= 2 sqrt(A) certifies the box low;
 *   NONOPT    opposite-pair fence intervals are disjoint, so the box contains
 *             no optimizer by the pair-equality necessary condition;
 *   SURVIVOR  f_hi > theta and scaled width <= wfloor (retained, not split further);
 *   else      bisect the widest (span-scaled) coordinate and recurse.
 *
 * Coordinates are scaled by their initial spans (2,1,2,1) so "widest" and the
 * wfloor stop are measured on comparable [0,1] axes; wfloor is a fraction of the
 * domain span.  Reported survivor volumes are physical (unscaled).
 */
#ifndef TRAP_SEARCH_H
#define TRAP_SEARCH_H

#include "geom.h"
#include "functionals.h"

enum { LEAF_DISCARD = 0, LEAF_CERTIFIED = 1, LEAF_SURVIVOR = 2,
       LEAF_NONOPTIMAL = 3, LEAF_FLAT_AREA = 4 };

typedef struct {
    double theta;
    double wfloor;
    slong  prec;
    slong  maxprec;       /* cap for adaptive precision */
    int    half;          /* quotient the reflection symmetry */
    int    adaptive_prec; /* retry marginal undecided boxes at higher prec */
    int    pair_eq_cert;  /* certify nonoptimal if opposite-pair intervals disagree */
    int    flat_area_cert;/* certify low if area upper bound <= theta^2/4 */
    enclosure_form form;
    long   max_leaves;    /* debugging cap; truncated runs fail full verification */
    int    serial;        /* force single-threaded even if OpenMP is available */
} search_params;

typedef struct {
    long n_discard, n_certified, n_flat_area, n_nonoptimal, n_survivor;
    long n_leaves, n_internal;
    double surv_lo[4], surv_hi[4];
    double surv_centroid[4];  /* volume-weighted mean of survivor box centres */
    double surv_vol;          /* sum of physical survivor box volumes (upper bnd) */
    double max_cert_fhi;      /* largest certified f_hi (self-audit: must be <= theta) */
    slong  max_prec_used;
    int    have_survivor;
} search_stats;

/* Leaf callback: invoked once per leaf (discard/certify/survivor).
 * encl is the min-enclosure [flo,fhi] of f over the box (undefined for discard,
 * where it is passed as NULL). item is the active item index (or -1). */
typedef void (*leaf_fn)(void *ctx, const double lo[4], const double hi[4],
                        int status, int item, const arb_t encl, slong prec);

/* Run the search.  cb may be NULL. */
void search_run(const search_params *p, search_stats *st, leaf_fn cb, void *ctx);

/* Run the search seeded from explicit boxes instead of the root box (used by
 * --refine to re-subdivide a previous run's survivors at a finer wfloor).
 * seed_lo/seed_hi are nseeds boxes.  Together with the certified/discarded
 * leaves of the run that produced the seeds, the output forms a complete
 * certificate of the original domain. */
void search_run_seeded(const search_params *p, search_stats *st, leaf_fn cb,
                       void *ctx, const double (*seed_lo)[4],
                       const double (*seed_hi)[4], size_t nseeds);

/* Initialise the raw search box into lo/hi (respects half if you wish to seed). */
void search_root_box(double lo[4], double hi[4]);

#endif /* TRAP_SEARCH_H */
