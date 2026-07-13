/* cert.c -- JSONL certificate writer and stateless verifier. */
#define _POSIX_C_SOURCE 200809L
#include "cert.h"
#include "admissible.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static const char *status_name(int s) {
    switch (s) {
        case LEAF_DISCARD:   return "discarded";
        case LEAF_CERTIFIED: return "certified";
        case LEAF_NONOPTIMAL:return "nonoptimal";
        case LEAF_FLAT_AREA: return "flat_area";
        default:             return "survivor";
    }
}

void cert_write_leaf(void *ctx, const double lo[4], const double hi[4],
                     int status, int item, const arb_t encl, slong prec) {
    cert_writer *w = (cert_writer *)ctx;
    if (!w || !w->fp) return;
    double flo = 0.0, fhi = 0.0;
    if (encl) {
        arf_t a; arf_init(a);
        arb_get_lbound_arf(a, encl, 53); flo = arf_get_d(a, ARF_RND_DOWN);
        arb_get_ubound_arf(a, encl, 53); fhi = arf_get_d(a, ARF_RND_UP);
        arf_clear(a);
    }
    /* JSON has no inf/nan literals: emit them as null. */
    char eb[64];
    if (isfinite(flo) && isfinite(fhi))
        snprintf(eb, sizeof eb, "[%.17g,%.17g]", flo, fhi);
    else
        snprintf(eb, sizeof eb, "[null,null]");
    fprintf(w->fp,
        "{\"box\":[[%.17g,%.17g],[%.17g,%.17g],[%.17g,%.17g],[%.17g,%.17g]],"
        "\"status\":\"%s\",\"item\":%d,\"encl\":%s,\"prec\":%ld}\n",
        lo[0], hi[0], lo[1], hi[1], lo[2], hi[2], lo[3], hi[3],
        status_name(status), item, eb, (long)prec);
}

/* ---- verifier ---- */
static int parse_line(const char *line, double lo[4], double hi[4],
                      char status[16], int *item) {
    const char *b = strstr(line, "\"box\":[[");
    if (!b) return 0;
    b += 6; /* advance to the first '[' of the box array */
    if (sscanf(b, "[[%lg,%lg],[%lg,%lg],[%lg,%lg],[%lg,%lg]]",
               &lo[0], &hi[0], &lo[1], &hi[1],
               &lo[2], &hi[2], &lo[3], &hi[3]) != 8) return 0;
    const char *s = strstr(line, "\"status\":\"");
    if (!s) return 0;
    s += strlen("\"status\":\"");
    int i = 0;
    while (*s && *s != '"' && i < 15) status[i++] = *s++;
    status[i] = '\0';
    const char *it = strstr(line, "\"item\":");
    *item = it ? atoi(it + strlen("\"item\":")) : -1;
    return 1;
}

typedef struct {
    double lo[4], hi[4];
    int status;
    int item;
    int used;
    long line_no;
} cert_leaf;

static int parse_status(const char *status) {
    if (strcmp(status, "discarded") == 0) return LEAF_DISCARD;
    if (strcmp(status, "certified") == 0) return LEAF_CERTIFIED;
    if (strcmp(status, "low") == 0) return LEAF_CERTIFIED;
    if (strcmp(status, "nonoptimal") == 0) return LEAF_NONOPTIMAL;
    if (strcmp(status, "flat_area") == 0) return LEAF_FLAT_AREA;
    if (strcmp(status, "survivor") == 0) return LEAF_SURVIVOR;
    return -1;
}

static int cmp_double(double a, double b) {
    return (a > b) - (a < b);
}

static int cmp_box_arrays(const double alo[4], const double ahi[4],
                          const double blo[4], const double bhi[4]) {
    for (int k = 0; k < 4; k++) {
        int c = cmp_double(alo[k], blo[k]);
        if (c) return c;
        c = cmp_double(ahi[k], bhi[k]);
        if (c) return c;
    }
    return 0;
}

static int leaf_cmp(const void *ap, const void *bp) {
    const cert_leaf *a = (const cert_leaf *)ap;
    const cert_leaf *b = (const cert_leaf *)bp;
    return cmp_box_arrays(a->lo, a->hi, b->lo, b->hi);
}

static long find_leaf(cert_leaf *leaf, long n,
                      const double lo[4], const double hi[4]) {
    long l = 0, r = n;
    while (l < r) {
        long m = l + (r - l) / 2;
        int c = cmp_box_arrays(leaf[m].lo, leaf[m].hi, lo, hi);
        if (c < 0) l = m + 1;
        else r = m;
    }
    if (l < n && cmp_box_arrays(leaf[l].lo, leaf[l].hi, lo, hi) == 0) return l;
    return -1;
}

static int widest_coord_cert(const double lo[4], const double hi[4]) {
    static const double span[4] = {2.0, 1.0, 2.0, 1.0};
    int best = 0;
    double bw = -1.0;
    for (int k = 0; k < 4; k++) {
        double w = (hi[k] - lo[k]) / span[k];
        if (w > bw) { bw = w; best = k; }
    }
    return best;
}

static int verify_leaf_claim(const cert_leaf *leaf, double theta,
                             enclosure_form form, int half, slong prec,
                             arb_t fenc, double *worst_cert_fhi,
                             long *n_disc, long *n_cert, long *n_flat,
                             long *n_nonopt, long *n_surv) {
    if (leaf->status == LEAF_DISCARD) {
        (*n_disc)++;
        if (!box_certainly_inadmissible(leaf->lo, leaf->hi, half, prec)) {
            fprintf(stderr, "FAIL[discard]: box not certainly inadmissible at line %ld\n",
                    leaf->line_no);
            return 1;
        }
        return 0;
    }

    if (leaf->status == LEAF_CERTIFIED) {
        (*n_cert)++;
        functionals_min(fenc, leaf->lo, leaf->hi, form, prec);
        arf_t u;
        arf_init(u);
        arb_get_ubound_arf(u, fenc, prec);
        double fhi = arf_get_d(u, ARF_RND_UP);
        int finite = arb_is_finite(fenc) && arf_is_finite(u);
        arf_clear(u);
        if (finite && fhi > *worst_cert_fhi) *worst_cert_fhi = fhi;
        if (!finite || fhi > theta) {
            fprintf(stderr,
                    "FAIL[certify]: f_hi=%g > theta=%.15g or non-finite at line %ld\n",
                    fhi, theta, leaf->line_no);
            return 1;
        }
        return 0;
    }

    if (leaf->status == LEAF_FLAT_AREA) {
        (*n_flat)++;
        if (!box_small_area_certifies_low(leaf->lo, leaf->hi, theta, prec)) {
            fprintf(stderr,
                    "FAIL[flat_area]: area upper bound too large at line %ld\n",
                    leaf->line_no);
            return 1;
        }
        return 0;
    }

    if (leaf->status == LEAF_NONOPTIMAL) {
        (*n_nonopt)++;
        if (!functionals_opposite_pairs_disjoint(leaf->lo, leaf->hi, form, prec)) {
            fprintf(stderr,
                    "FAIL[nonoptimal]: opposite-pair intervals overlap at line %ld\n",
                    leaf->line_no);
            return 1;
        }
        return 0;
    }

    if (leaf->status == LEAF_SURVIVOR) {
        (*n_surv)++;
        return 0;
    }

    fprintf(stderr, "FAIL[status]: unknown status at line %ld\n", leaf->line_no);
    return 1;
}

static long verify_cover_node(cert_leaf *leaf, long n,
                              const double lo[4], const double hi[4],
                              double theta, enclosure_form form, int half,
                              slong prec, arb_t fenc,
                              double *worst_cert_fhi,
                              long *n_disc, long *n_cert, long *n_flat,
                              long *n_nonopt, long *n_surv,
                              int depth) {
    if (depth > 128) {
        fprintf(stderr, "FAIL[cover]: exceeded recursion depth near "
                        "[%.17g,%.17g]x...\n", lo[0], hi[0]);
        return 1;
    }

    long idx = find_leaf(leaf, n, lo, hi);
    if (idx >= 0) {
        if (leaf[idx].used) {
            fprintf(stderr, "FAIL[cover]: duplicate box at line %ld\n",
                    leaf[idx].line_no);
            return 1;
        }
        leaf[idx].used = 1;
        return verify_leaf_claim(&leaf[idx], theta, form, half, prec, fenc,
                                 worst_cert_fhi, n_disc, n_cert, n_flat,
                                 n_nonopt, n_surv);
    }

    int wc = widest_coord_cert(lo, hi);
    double mid = 0.5 * (lo[wc] + hi[wc]);
    if (!(lo[wc] < mid && mid < hi[wc])) {
        fprintf(stderr, "FAIL[cover]: missing unsplittable box "
                        "[%.17g,%.17g]x[%.17g,%.17g]x"
                        "[%.17g,%.17g]x[%.17g,%.17g]\n",
                lo[0], hi[0], lo[1], hi[1], lo[2], hi[2], lo[3], hi[3]);
        return 1;
    }

    double l0[4], h0[4], l1[4], h1[4];
    for (int k = 0; k < 4; k++) {
        l0[k] = lo[k]; h0[k] = hi[k];
        l1[k] = lo[k]; h1[k] = hi[k];
    }
    h0[wc] = mid;
    l1[wc] = mid;
    long fails = 0;
    fails += verify_cover_node(leaf, n, l0, h0, theta, form, half, prec, fenc,
                               worst_cert_fhi, n_disc, n_cert, n_flat,
                               n_nonopt, n_surv,
                               depth + 1);
    fails += verify_cover_node(leaf, n, l1, h1, theta, form, half, prec, fenc,
                               worst_cert_fhi, n_disc, n_cert, n_flat,
                               n_nonopt, n_surv,
                               depth + 1);
    return fails;
}

size_t cert_load_survivors(const char *path, double (**lo)[4], double (**hi)[4]) {
    FILE *fp = fopen(path, "r");
    if (!fp) { fprintf(stderr, "cannot open %s\n", path); return 0; }
    char *line = NULL;
    size_t cap = 0, n = 0, alloc = 1024;
    *lo = malloc(alloc * sizeof(**lo));
    *hi = malloc(alloc * sizeof(**hi));
    while (getline(&line, &cap, fp) != -1) {
        double blo[4], bhi[4];
        char status[16];
        int item;
        if (!parse_line(line, blo, bhi, status, &item)) continue;
        if (strcmp(status, "survivor") != 0) continue;
        if (n == alloc) {
            alloc *= 2;
            *lo = realloc(*lo, alloc * sizeof(**lo));
            *hi = realloc(*hi, alloc * sizeof(**hi));
        }
        for (int k = 0; k < 4; k++) { (*lo)[n][k] = blo[k]; (*hi)[n][k] = bhi[k]; }
        n++;
    }
    free(line);
    fclose(fp);
    return n;
}

int cert_verify(const char *path, double theta, enclosure_form form,
                int half, slong prec) {
    FILE *fp = fopen(path, "r");
    if (!fp) { fprintf(stderr, "cert_verify: cannot open %s\n", path); return -1; }

    char *line = NULL;
    size_t cap = 0;
    long n = 0, fails = 0, n_disc = 0, n_cert = 0, n_flat = 0;
    long n_nonopt = 0, n_surv = 0;
    long alloc = 1024;
    cert_leaf *leaf = malloc((size_t)alloc * sizeof(*leaf));
    if (!leaf) {
        fclose(fp);
        fprintf(stderr, "cert_verify: out of memory\n");
        return -1;
    }
    double worst_cert_fhi = -1.0;
    arb_t fenc;
    arb_init(fenc);

    long physical_line = 0;
    while (getline(&line, &cap, fp) != -1) {
        physical_line++;
        double lo[4], hi[4];
        char status[16];
        int item;
        long line_no = physical_line;
        if (!parse_line(line, lo, hi, status, &item)) {
            fails++;
            fprintf(stderr, "FAIL[parse]: could not parse line %ld\n", line_no);
            continue;
        }
        int st = parse_status(status);
        if (st < 0) {
            fails++;
            fprintf(stderr, "FAIL[status]: unknown status '%s' at line %ld\n",
                    status, line_no);
            continue;
        }
        if (n == alloc) {
            alloc *= 2;
            cert_leaf *tmp = realloc(leaf, (size_t)alloc * sizeof(*leaf));
            if (!tmp) {
                free(line);
                free(leaf);
                arb_clear(fenc);
                fclose(fp);
                fprintf(stderr, "cert_verify: out of memory\n");
                return -1;
            }
            leaf = tmp;
        }
        for (int k = 0; k < 4; k++) {
            leaf[n].lo[k] = lo[k];
            leaf[n].hi[k] = hi[k];
            if (!(lo[k] <= hi[k])) {
                fails++;
                fprintf(stderr, "FAIL[box]: inverted interval at line %ld\n", line_no);
            }
        }
        leaf[n].status = st;
        leaf[n].item = item;
        leaf[n].used = 0;
        leaf[n].line_no = line_no;
        n++;
    }
    free(line);
    fclose(fp);

    if (n == 0) {
        fails++;
        fprintf(stderr, "FAIL[cover]: certificate is empty\n");
    } else if (!fails) {
        qsort(leaf, (size_t)n, sizeof(*leaf), leaf_cmp);
        for (long i = 1; i < n; i++) {
            if (cmp_box_arrays(leaf[i - 1].lo, leaf[i - 1].hi,
                               leaf[i].lo, leaf[i].hi) == 0) {
                fails++;
                fprintf(stderr, "FAIL[cover]: duplicate boxes at lines %ld and %ld\n",
                        leaf[i - 1].line_no, leaf[i].line_no);
            }
        }
    }

    if (!fails) {
        double root_lo[4], root_hi[4];
        search_root_box(root_lo, root_hi);
        fails += verify_cover_node(leaf, n, root_lo, root_hi, theta, form, half,
                                   prec, fenc, &worst_cert_fhi,
                                   &n_disc, &n_cert, &n_flat, &n_nonopt, &n_surv, 0);
        for (long i = 0; i < n; i++) {
            if (!leaf[i].used) {
                fails++;
                fprintf(stderr, "FAIL[cover]: unused or overlapping leaf at line %ld\n",
                        leaf[i].line_no);
            }
        }
    }

    arb_clear(fenc);
    free(leaf);

    printf("verify: %ld leaves  (discard %ld, certify %ld, flat_area %ld, nonoptimal %ld, survivor %ld)  prec=%ld\n",
           n, n_disc, n_cert, n_flat, n_nonopt, n_surv, (long)prec);
    printf("verify: dyadic partition coverage of the full root box: %s\n",
           fails ? "FAILED" : "PASSED");
    printf("verify: worst re-checked certified f_hi = %.15g  (theta = %.15g)\n",
           worst_cert_fhi, theta);
    printf("verify: %ld failures -> %s\n", fails, fails ? "AUDIT FAILED" : "AUDIT PASSED");
    return (int)fails;
}
