/* search.c -- dyadic subdivision work queue (serial + optional OpenMP). */
#include "search.h"
#include "admissible.h"
#include "series.h"
#include <stdlib.h>
#include <string.h>

#if defined(_OPENMP)
#include <omp.h>
#endif

static const double SPAN[4] = {2.0, 1.0, 2.0, 1.0};

void search_root_box(double lo[4], double hi[4]) {
    lo[0] = 0.0; hi[0] = 2.0;   /* c1 */
    lo[1] = 0.0; hi[1] = 1.0;   /* c2 */
    lo[2] = -1.0; hi[2] = 1.0;  /* d1 */
    lo[3] = 0.0; hi[3] = 1.0;   /* d2 */
}

/* ---- box stack ---- */
typedef struct { double lo[4], hi[4]; } box_t;
typedef struct { box_t *a; size_t n, cap; } stack_t;

static void st_init(stack_t *s) { s->a = NULL; s->n = 0; s->cap = 0; }
static void st_free(stack_t *s) { free(s->a); }
static void st_push(stack_t *s, const box_t *b) {
    if (s->n == s->cap) {
        s->cap = s->cap ? s->cap * 2 : 1024;
        s->a = realloc(s->a, s->cap * sizeof(box_t));
    }
    s->a[s->n++] = *b;
}
static int st_pop(stack_t *s, box_t *b) {
    if (s->n == 0) return 0;
    *b = s->a[--s->n];
    return 1;
}

static int widest_coord(const box_t *b, double *scaled_width) {
    int best = 0;
    double bw = -1.0;
    for (int k = 0; k < 4; k++) {
        double w = (b->hi[k] - b->lo[k]) / SPAN[k];
        if (w > bw) { bw = w; best = k; }
    }
    *scaled_width = bw;
    return best;
}

/* Classify one box.  Returns LEAF_* if it is a leaf, or -1 if it must be split
 * (children written to c0,c1).  Fills *fenc (min-enclosure) and *item unless
 * discarded.  Uses/raises prec adaptively; *used_prec reports what was used. */
static int classify(const box_t *b, const search_params *p,
                    arb_t fenc, int *item, slong *used_prec,
                    box_t *c0, box_t *c1) {
    if (box_certainly_inadmissible(b->lo, b->hi, p->half, p->prec)) {
        *used_prec = p->prec;
        *item = -1;
        return LEAF_DISCARD;
    }
    slong prec = p->prec;
    if (p->flat_area_cert &&
        box_small_area_certifies_low(b->lo, b->hi, p->theta, prec)) {
        *used_prec = prec;
        *item = -2;
        return LEAF_FLAT_AREA;
    }

    if (p->pair_eq_cert &&
        functionals_opposite_pairs_disjoint(b->lo, b->hi, p->form, prec)) {
        *used_prec = prec;
        *item = -1;
        return LEAF_NONOPTIMAL;
    }

    int act = functionals_min(fenc, b->lo, b->hi, p->form, prec);
    *item = act;
    *used_prec = prec;

    arf_t fhi;
    arf_init(fhi);
    arb_get_ubound_arf(fhi, fenc, prec);
    /* Certify only on a finite enclosure with a finite upper bound <= theta.
     * A non-finite enclosure (functionals failed to bound this coarse box) can
     * never certify -- the box must be split (or retained at the floor). */
    int certified = arb_is_finite(fenc) && arf_is_finite(fhi)
                    && (arf_cmp_d(fhi, p->theta) <= 0);

    /* adaptive precision: if marginally undecided, retry once at higher prec */
    if (!certified && p->adaptive_prec && prec < p->maxprec) {
        double fhid = arf_get_d(fhi, ARF_RND_UP);
        if (fhid <= p->theta * 1.05) {   /* only near the threshold */
            slong hp = prec * 2;
            if (hp > p->maxprec) hp = p->maxprec;
            int act2 = functionals_min(fenc, b->lo, b->hi, p->form, hp);
            *item = act2;
            *used_prec = hp;
            arb_get_ubound_arf(fhi, fenc, hp);
            certified = (arf_cmp_d(fhi, p->theta) <= 0);
            prec = hp;
        }
    }
    arf_clear(fhi);

    if (certified) return LEAF_CERTIFIED;

    double sw;
    int wc = widest_coord(b, &sw);
    if (sw <= p->wfloor) return LEAF_SURVIVOR;

    /* bisect widest scaled coordinate */
    double mid = 0.5 * (b->lo[wc] + b->hi[wc]);
    *c0 = *b; *c1 = *b;
    c0->hi[wc] = mid;
    c1->lo[wc] = mid;
    return -1;
}

static void stats_add_leaf(search_stats *st, const box_t *b, int status,
                           const arb_t fenc, slong used_prec) {
    st->n_leaves++;
    if (used_prec > st->max_prec_used) st->max_prec_used = used_prec;
    if (status == LEAF_DISCARD) {
        st->n_discard++;
    } else if (status == LEAF_FLAT_AREA) {
        st->n_flat_area++;
    } else if (status == LEAF_NONOPTIMAL) {
        st->n_nonoptimal++;
    } else if (status == LEAF_CERTIFIED) {
        st->n_certified++;
        arf_t u; arf_init(u);
        arb_get_ubound_arf(u, fenc, 53);
        double fhi = arf_get_d(u, ARF_RND_UP);
        arf_clear(u);
        if (fhi > st->max_cert_fhi) st->max_cert_fhi = fhi;
    } else { /* survivor */
        st->n_survivor++;
        double vol = 1.0;
        for (int k = 0; k < 4; k++) vol *= (b->hi[k] - b->lo[k]);
        for (int k = 0; k < 4; k++) {
            double c = 0.5 * (b->lo[k] + b->hi[k]);
            if (!st->have_survivor) {
                st->surv_lo[k] = b->lo[k];
                st->surv_hi[k] = b->hi[k];
                st->surv_centroid[k] = 0.0;
            } else {
                if (b->lo[k] < st->surv_lo[k]) st->surv_lo[k] = b->lo[k];
                if (b->hi[k] > st->surv_hi[k]) st->surv_hi[k] = b->hi[k];
            }
            st->surv_centroid[k] += c * vol;   /* accumulate; normalised by caller */
        }
        st->surv_vol += vol;
        st->have_survivor = 1;
    }
}

/* ---- serial driver (consumes an already-seeded stack) ---- */
static void run_serial(const search_params *p, search_stats *st,
                       leaf_fn cb, void *ctx, stack_t *sp) {
    stack_t s = *sp;   /* take ownership */
    box_t b, c0, c1;
    arb_t fenc;
    arb_init(fenc);
    while (st_pop(&s, &b)) {
        int item;
        slong up;
        int status = classify(&b, p, fenc, &item, &up, &c0, &c1);
        if (status < 0) {
            st->n_internal++;
            st_push(&s, &c0);
            st_push(&s, &c1);
        } else {
            stats_add_leaf(st, &b, status, fenc, up);
            if (cb) cb(ctx, b.lo, b.hi, status,
                       item,
                       (status == LEAF_DISCARD || status == LEAF_NONOPTIMAL ||
                        status == LEAF_FLAT_AREA) ? NULL : fenc, up);
            if (p->max_leaves && st->n_leaves >= p->max_leaves) break;
        }
    }
    arb_clear(fenc);
    st_free(&s);
}

#if defined(_OPENMP)
/* ---- parallel driver: shared stack + idle counter for termination ---- */
static void run_parallel(const search_params *p, search_stats *st,
                         leaf_fn cb, void *ctx, stack_t *sp) {
    stack_t s = *sp;   /* take ownership */
    int nthreads = omp_get_max_threads();
    volatile int idle = 0;
    volatile int stop = 0;

    #pragma omp parallel
    {
        box_t b, c0, c1;
        arb_t fenc;
        arb_init(fenc);
        int counted_idle = 0;
        for (;;) {
            int got = 0;
            #pragma omp critical (trap_stack)
            {
                if (s.n > 0) { b = s.a[--s.n]; got = 1; }
            }
            if (!got) {
                /* register idleness; terminate when all threads idle */
                if (!counted_idle) {
                    #pragma omp atomic
                    idle++;
                    counted_idle = 1;
                }
                if (idle >= nthreads || stop) break;
                continue;
            }
            if (counted_idle) {
                #pragma omp atomic
                idle--;
                counted_idle = 0;
            }
            int item; slong up;
            int status = classify(&b, p, fenc, &item, &up, &c0, &c1);
            if (status < 0) {
                #pragma omp critical (trap_stack)
                {
                    st->n_internal++;
                    st_push(&s, &c0);
                    st_push(&s, &c1);
                }
            } else {
                #pragma omp critical (trap_stats)
                {
                    stats_add_leaf(st, &b, status, fenc, up);
                    if (cb) cb(ctx, b.lo, b.hi, status,
                               item,
                               (status == LEAF_DISCARD || status == LEAF_NONOPTIMAL ||
                                status == LEAF_FLAT_AREA) ? NULL : fenc, up);
                    if (p->max_leaves && st->n_leaves >= p->max_leaves) stop = 1;
                }
            }
        }
        arb_clear(fenc);
    }
    st_free(&s);
}
#endif

static void run_stack(const search_params *p, search_stats *st, leaf_fn cb,
                      void *ctx, stack_t *s) {
    memset(st, 0, sizeof(*st));
    st->max_cert_fhi = -1.0;
#if defined(_OPENMP)
    if (!p->serial && omp_get_max_threads() > 1) {
        run_parallel(p, st, cb, ctx, s);
        return;
    }
#endif
    run_serial(p, st, cb, ctx, s);
}

void search_run(const search_params *p, search_stats *st, leaf_fn cb, void *ctx) {
    stack_t s;
    st_init(&s);
    box_t root;
    search_root_box(root.lo, root.hi);
    st_push(&s, &root);
    run_stack(p, st, cb, ctx, &s);
}

void search_run_seeded(const search_params *p, search_stats *st, leaf_fn cb,
                       void *ctx, const double (*seed_lo)[4],
                       const double (*seed_hi)[4], size_t nseeds) {
    stack_t s;
    st_init(&s);
    for (size_t i = 0; i < nseeds; i++) {
        box_t b;
        for (int k = 0; k < 4; k++) {
            b.lo[k] = seed_lo[i][k];
            b.hi[k] = seed_hi[i][k];
        }
        st_push(&s, &b);
    }
    run_stack(p, st, cb, ctx, &s);
}
