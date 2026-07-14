/* cert.c -- JSONL certificate writer and stateless verifier. */
#define _POSIX_C_SOURCE 200809L
#include "cert.h"
#include "admissible.h"
#include "threshold.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

typedef struct {
    int present;
    int schema;
    char theta_str[128];
    double theta_approx;
    enclosure_form form;
    int half;
    int pair_eq_cert;
    int flat_area_cert;
    slong run_prec;
    int has_root;
    double root_lo[4], root_hi[4];
} cert_meta;

static const char *form_name(enclosure_form form) {
    return form == FORM_CENTERED ? "centered" : "natural";
}

static int parse_form_name(const char *s, enclosure_form *form) {
    if (strcmp(s, "natural") == 0) {
        *form = FORM_NATURAL;
        return 1;
    }
    if (strcmp(s, "centered") == 0) {
        *form = FORM_CENTERED;
        return 1;
    }
    return 0;
}

static void cert_meta_init(cert_meta *m) {
    memset(m, 0, sizeof(*m));
    m->schema = -1;
    m->run_prec = -1;
}

static const char *json_value(const char *line, const char *name) {
    char key[64];
    snprintf(key, sizeof key, "\"%s\"", name);
    const char *p = strstr(line, key);
    if (!p) return NULL;
    p = strchr(p + strlen(key), ':');
    if (!p) return NULL;
    p++;
    while (*p && isspace((unsigned char)*p)) p++;
    return p;
}

static int json_string_field(const char *line, const char *name,
                             char *out, size_t out_sz) {
    const char *p = json_value(line, name);
    if (!p || *p != '"' || out_sz == 0) return 0;
    p++;
    size_t n = 0;
    while (*p && *p != '"') {
        if (n + 1 < out_sz) out[n++] = *p;
        p++;
    }
    if (*p != '"') return 0;
    out[n] = '\0';
    return 1;
}

static int json_long_field(const char *line, const char *name, long *out) {
    const char *p = json_value(line, name);
    char *end = NULL;
    if (!p) return 0;
    *out = strtol(p, &end, 10);
    return end && end != p;
}

static int json_bool_field(const char *line, const char *name, int *out) {
    const char *p = json_value(line, name);
    if (!p) return 0;
    if (strncmp(p, "true", 4) == 0) {
        *out = 1;
        return 1;
    }
    if (strncmp(p, "false", 5) == 0) {
        *out = 0;
        return 1;
    }
    if (*p == '0' || *p == '1') {
        *out = (*p == '1');
        return 1;
    }
    return 0;
}

static int json_root_field(const char *line, cert_meta *m) {
    const char *p = json_value(line, "root");
    if (!p) return 0;
    if (sscanf(p, "[[%lg,%lg],[%lg,%lg],[%lg,%lg],[%lg,%lg]]",
               &m->root_lo[0], &m->root_hi[0], &m->root_lo[1], &m->root_hi[1],
               &m->root_lo[2], &m->root_hi[2], &m->root_lo[3], &m->root_hi[3]) != 8)
        return 0;
    m->has_root = 1;
    return 1;
}

static int parse_meta_line(const char *line, cert_meta *m) {
    char type[32];
    if (!json_string_field(line, "type", type, sizeof type)) return 0;
    if (strcmp(type, "meta") != 0) return 0;

    cert_meta tmp;
    cert_meta_init(&tmp);
    tmp.present = 1;
    char form[32];
    long schema, run_prec;
    if (!json_string_field(line, "theta", tmp.theta_str, sizeof tmp.theta_str) &&
        !json_string_field(line, "theta_str", tmp.theta_str, sizeof tmp.theta_str)) {
        return -1;
    }
    fmpq_t theta_q;
    fmpq_init(theta_q);
    int theta_bad = threshold_parse_fmpq(theta_q, tmp.theta_str);
    fmpq_clear(theta_q);
    if (theta_bad) return -1;
    if (!json_long_field(line, "schema", &schema) ||
        !json_string_field(line, "form", form, sizeof form) ||
        !parse_form_name(form, &tmp.form) ||
        !json_bool_field(line, "half", &tmp.half) ||
        !json_root_field(line, &tmp)) {
        return -1;
    }
    tmp.schema = (int)schema;
    tmp.theta_approx = threshold_to_double(tmp.theta_str);
    if (json_bool_field(line, "pair_eq_cert", &tmp.pair_eq_cert) == 0)
        tmp.pair_eq_cert = 0;
    if (json_bool_field(line, "flat_area_cert", &tmp.flat_area_cert) == 0)
        tmp.flat_area_cert = 0;
    if (json_long_field(line, "run_prec", &run_prec))
        tmp.run_prec = (slong)run_prec;
    *m = tmp;
    return 1;
}

static void cert_write_meta_struct(FILE *fp, const cert_meta *m) {
    fprintf(fp,
        "{\"type\":\"meta\",\"schema\":%d,\"theta\":\"%s\",\"theta_approx\":%.17g,"
        "\"form\":\"%s\",\"half\":%s,"
        "\"pair_eq_cert\":%s,\"flat_area_cert\":%s,\"run_prec\":%ld,"
        "\"root\":[[%.17g,%.17g],[%.17g,%.17g],[%.17g,%.17g],[%.17g,%.17g]],"
        "\"split_spans\":[2,1,2,1]}\n",
        m->schema, m->theta_str, m->theta_approx, form_name(m->form),
        m->half ? "true" : "false",
        m->pair_eq_cert ? "true" : "false",
        m->flat_area_cert ? "true" : "false",
        (long)m->run_prec,
        m->root_lo[0], m->root_hi[0], m->root_lo[1], m->root_hi[1],
        m->root_lo[2], m->root_hi[2], m->root_lo[3], m->root_hi[3]);
}

void cert_write_meta(FILE *fp, const search_params *p) {
    if (!fp || !p) return;
    cert_meta m;
    cert_meta_init(&m);
    m.present = 1;
    m.schema = 3;
    snprintf(m.theta_str, sizeof m.theta_str, "%s", p->theta_str);
    m.theta_approx = p->theta;
    m.form = p->form;
    m.half = p->half;
    m.pair_eq_cert = p->pair_eq_cert;
    m.flat_area_cert = p->flat_area_cert;
    m.run_prec = p->prec;
    m.has_root = 1;
    search_root_box(m.root_lo, m.root_hi);
    cert_write_meta_struct(fp, &m);
}

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

static int box_equal(const double alo[4], const double ahi[4],
                     const double blo[4], const double bhi[4]) {
    return cmp_box_arrays(alo, ahi, blo, bhi) == 0;
}

static int leaf_inside_node(const cert_leaf *leaf,
                            const double lo[4], const double hi[4]) {
    for (int k = 0; k < 4; k++) {
        if (leaf->lo[k] < lo[k] || leaf->hi[k] > hi[k]) return 0;
    }
    return 1;
}

static long *identity_index(long n) {
    if (n <= 0) return NULL;
    long *idx = malloc((size_t)n * sizeof(*idx));
    if (!idx) return NULL;
    for (long i = 0; i < n; i++) idx[i] = i;
    return idx;
}

static void print_box_prefix(const char *tag, const double lo[4], const double hi[4]) {
    fprintf(stderr,
            "%s[%.17g,%.17g]x[%.17g,%.17g]x"
            "[%.17g,%.17g]x[%.17g,%.17g]\n",
            tag, lo[0], hi[0], lo[1], hi[1], lo[2], hi[2], lo[3], hi[3]);
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

static int verify_leaf_claim(const cert_leaf *leaf, const char *theta_str,
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
        if (!finite || !threshold_arf_leq(u, theta_str)) {
            fprintf(stderr,
                    "FAIL[certify]: f_hi=%g > theta=%s or non-finite at line %ld\n",
                    fhi, theta_str, leaf->line_no);
            return 1;
        }
        return 0;
    }

    if (leaf->status == LEAF_FLAT_AREA) {
        (*n_flat)++;
        if (!box_small_area_certifies_low(leaf->lo, leaf->hi, theta_str, prec)) {
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

static int verify_cover_node_slice(cert_leaf *leaf, const long *idx, long nidx,
                                   const double lo[4], const double hi[4],
                                   const char *theta_str, enclosure_form form, int half,
                                   slong prec, arb_t fenc,
                                   double *worst_cert_fhi,
                                   long *n_disc, long *n_cert, long *n_flat,
                                   long *n_nonopt, long *n_surv,
                                   int depth) {
    if (depth > 128) {
        print_box_prefix("FAIL[cover]: exceeded recursion depth near ", lo, hi);
        return 1;
    }

    if (nidx == 0) {
        print_box_prefix("FAIL[cover]: missing certificate leaves for ", lo, hi);
        return 1;
    }

    long exact = -1;
    for (long i = 0; i < nidx; i++) {
        cert_leaf *lf = &leaf[idx[i]];
        if (!leaf_inside_node(lf, lo, hi)) {
            fprintf(stderr, "FAIL[cover]: line %ld is outside expected node\n",
                    lf->line_no);
            print_box_prefix("  node: ", lo, hi);
            print_box_prefix("  leaf: ", lf->lo, lf->hi);
            return 1;
        }
        if (box_equal(lf->lo, lf->hi, lo, hi)) {
            exact = i;
        }
    }

    if (exact >= 0) {
        if (nidx != 1) {
            fprintf(stderr,
                    "FAIL[cover]: line %ld covers a node that also has %ld descendant/overlapping leaves\n",
                    leaf[idx[exact]].line_no, nidx - 1);
            print_box_prefix("  node: ", lo, hi);
            return 1;
        }
        cert_leaf *lf = &leaf[idx[exact]];
        if (lf->used) {
            fprintf(stderr, "FAIL[cover]: duplicate/overused box at line %ld\n",
                    lf->line_no);
            return 1;
        }
        lf->used = 1;
        return verify_leaf_claim(lf, theta_str, form, half, prec, fenc,
                                 worst_cert_fhi, n_disc, n_cert, n_flat,
                                 n_nonopt, n_surv);
    }

    int wc = widest_coord_cert(lo, hi);
    double mid = 0.5 * (lo[wc] + hi[wc]);
    if (!(lo[wc] < mid && mid < hi[wc])) {
        print_box_prefix("FAIL[cover]: missing unsplittable box ", lo, hi);
        return 1;
    }

    double l0[4], h0[4], l1[4], h1[4];
    for (int k = 0; k < 4; k++) {
        l0[k] = lo[k]; h0[k] = hi[k];
        l1[k] = lo[k]; h1[k] = hi[k];
    }
    h0[wc] = mid;
    l1[wc] = mid;
    long *left = malloc((size_t)nidx * sizeof(*left));
    long *right = malloc((size_t)nidx * sizeof(*right));
    if (!left || !right) {
        free(left); free(right);
        fprintf(stderr, "FAIL[cover]: out of memory while partitioning node\n");
        return 1;
    }
    long nl = 0, nr = 0;
    for (long i = 0; i < nidx; i++) {
        cert_leaf *lf = &leaf[idx[i]];
        if (lf->hi[wc] <= mid) {
            left[nl++] = idx[i];
        } else if (lf->lo[wc] >= mid) {
            right[nr++] = idx[i];
        } else {
            fprintf(stderr,
                    "FAIL[cover]: line %ld crosses deterministic split coordinate %d at %.17g\n",
                    lf->line_no, wc, mid);
            print_box_prefix("  node: ", lo, hi);
            print_box_prefix("  leaf: ", lf->lo, lf->hi);
            free(left); free(right);
            return 1;
        }
    }
    if (nl == 0) {
        print_box_prefix("FAIL[cover]: missing left child ", l0, h0);
        free(left); free(right);
        return 1;
    }
    if (nr == 0) {
        print_box_prefix("FAIL[cover]: missing right child ", l1, h1);
        free(left); free(right);
        return 1;
    }

    int fail = verify_cover_node_slice(leaf, left, nl, l0, h0, theta_str, form, half,
                                       prec, fenc, worst_cert_fhi,
                                       n_disc, n_cert, n_flat, n_nonopt, n_surv,
                                       depth + 1);
    if (!fail) {
        fail = verify_cover_node_slice(leaf, right, nr, l1, h1, theta_str, form, half,
                                       prec, fenc, worst_cert_fhi,
                                       n_disc, n_cert, n_flat, n_nonopt, n_surv,
                                       depth + 1);
    }
    free(left); free(right);
    return fail;
}

static int cover_node_slice_structure(cert_leaf *leaf, const long *idx, long nidx,
                                      const double lo[4], const double hi[4],
                                      int depth) {
    if (depth > 128) {
        print_box_prefix("FAIL[assemble]: exceeded recursion depth near ", lo, hi);
        return 1;
    }
    if (nidx == 0) {
        print_box_prefix("FAIL[assemble]: missing refinement leaves for ", lo, hi);
        return 1;
    }

    long exact = -1;
    for (long i = 0; i < nidx; i++) {
        cert_leaf *lf = &leaf[idx[i]];
        if (!leaf_inside_node(lf, lo, hi)) {
            fprintf(stderr, "FAIL[assemble]: line %ld is outside expected survivor node\n",
                    lf->line_no);
            print_box_prefix("  survivor node: ", lo, hi);
            print_box_prefix("  refinement leaf: ", lf->lo, lf->hi);
            return 1;
        }
        if (box_equal(lf->lo, lf->hi, lo, hi)) exact = i;
    }

    if (exact >= 0) {
        if (nidx != 1) {
            fprintf(stderr,
                    "FAIL[assemble]: line %ld covers a node that also has %ld descendant/overlapping leaves\n",
                    leaf[idx[exact]].line_no, nidx - 1);
            print_box_prefix("  node: ", lo, hi);
            return 1;
        }
        if (leaf[idx[exact]].used) {
            fprintf(stderr, "FAIL[assemble]: refinement leaf reused at line %ld\n",
                    leaf[idx[exact]].line_no);
            return 1;
        }
        leaf[idx[exact]].used = 1;
        return 0;
    }

    int wc = widest_coord_cert(lo, hi);
    double mid = 0.5 * (lo[wc] + hi[wc]);
    if (!(lo[wc] < mid && mid < hi[wc])) {
        print_box_prefix("FAIL[assemble]: missing unsplittable node ", lo, hi);
        return 1;
    }

    double l0[4], h0[4], l1[4], h1[4];
    for (int k = 0; k < 4; k++) {
        l0[k] = lo[k]; h0[k] = hi[k];
        l1[k] = lo[k]; h1[k] = hi[k];
    }
    h0[wc] = mid;
    l1[wc] = mid;

    long *left = malloc((size_t)nidx * sizeof(*left));
    long *right = malloc((size_t)nidx * sizeof(*right));
    if (!left || !right) {
        free(left); free(right);
        fprintf(stderr, "FAIL[assemble]: out of memory while partitioning node\n");
        return 1;
    }
    long nl = 0, nr = 0;
    for (long i = 0; i < nidx; i++) {
        cert_leaf *lf = &leaf[idx[i]];
        if (lf->hi[wc] <= mid) {
            left[nl++] = idx[i];
        } else if (lf->lo[wc] >= mid) {
            right[nr++] = idx[i];
        } else {
            fprintf(stderr,
                    "FAIL[assemble]: line %ld crosses deterministic split coordinate %d at %.17g\n",
                    lf->line_no, wc, mid);
            print_box_prefix("  node: ", lo, hi);
            print_box_prefix("  refinement leaf: ", lf->lo, lf->hi);
            free(left); free(right);
            return 1;
        }
    }
    if (nl == 0) {
        print_box_prefix("FAIL[assemble]: missing left child ", l0, h0);
        free(left); free(right);
        return 1;
    }
    if (nr == 0) {
        print_box_prefix("FAIL[assemble]: missing right child ", l1, h1);
        free(left); free(right);
        return 1;
    }

    int fail = cover_node_slice_structure(leaf, left, nl, l0, h0, depth + 1);
    if (!fail) fail = cover_node_slice_structure(leaf, right, nr, l1, h1, depth + 1);
    free(left); free(right);
    return fail;
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

static int close_double(double a, double b) {
    double scale = fmax(1.0, fmax(fabs(a), fabs(b)));
    return fabs(a - b) <= 1e-14 * scale;
}

static int meta_root_current(const cert_meta *m) {
    if (!m->has_root) return 0;
    double lo[4], hi[4];
    search_root_box(lo, hi);
    for (int k = 0; k < 4; k++) {
        if (!close_double(m->root_lo[k], lo[k]) ||
            !close_double(m->root_hi[k], hi[k]))
            return 0;
    }
    return 1;
}

static int theta_literals_equal(const char *a, const char *b) {
    fmpq_t qa, qb;
    fmpq_init(qa);
    fmpq_init(qb);
    int ok = !threshold_parse_fmpq(qa, a) &&
             !threshold_parse_fmpq(qb, b) &&
             fmpq_cmp(qa, qb) == 0;
    fmpq_clear(qa);
    fmpq_clear(qb);
    return ok;
}

static int meta_compatible(const cert_meta *a, const cert_meta *b,
                           const char *where) {
    int fail = 0;
    if (!a->present || !b->present) {
        fprintf(stderr, "FAIL[meta]: missing metadata while checking %s\n", where);
        return 1;
    }
    if (a->schema != b->schema) {
        fprintf(stderr, "FAIL[meta]: schema mismatch in %s (%d vs %d)\n",
                where, a->schema, b->schema);
        fail = 1;
    }
    if (!theta_literals_equal(a->theta_str, b->theta_str)) {
        fprintf(stderr, "FAIL[meta]: theta mismatch in %s (%s vs %s)\n",
                where, a->theta_str, b->theta_str);
        fail = 1;
    }
    if (a->form != b->form) {
        fprintf(stderr, "FAIL[meta]: form mismatch in %s (%s vs %s)\n",
                where, form_name(a->form), form_name(b->form));
        fail = 1;
    }
    if (a->half != b->half) {
        fprintf(stderr, "FAIL[meta]: half mismatch in %s (%d vs %d)\n",
                where, a->half, b->half);
        fail = 1;
    }
    if (!meta_root_current(a) || !meta_root_current(b)) {
        fprintf(stderr, "FAIL[meta]: root box mismatch in %s\n", where);
        fail = 1;
    }
    return fail;
}

int cert_verify(const char *path, const char *theta_str, int theta_set,
                enclosure_form form, int form_set,
                int half, int half_set, slong prec) {
    FILE *fp = fopen(path, "r");
    if (!fp) { fprintf(stderr, "cert_verify: cannot open %s\n", path); return -1; }

    char *line = NULL;
    size_t cap = 0;
    long n = 0, fails = 0, n_disc = 0, n_cert = 0, n_flat = 0;
    long n_nonopt = 0, n_surv = 0;
    int coverage_checked = 0;
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
    cert_meta meta;
    cert_meta_init(&meta);

    long physical_line = 0;
    while (getline(&line, &cap, fp) != -1) {
        physical_line++;
        cert_meta parsed_meta;
        int mp = parse_meta_line(line, &parsed_meta);
        if (mp < 0) {
            fails++;
            fprintf(stderr, "FAIL[meta]: malformed metadata at line %ld\n",
                    physical_line);
            continue;
        }
        if (mp > 0) {
            if (meta.present) {
                fails++;
                fprintf(stderr, "FAIL[meta]: duplicate metadata at line %ld\n",
                        physical_line);
            } else {
                meta = parsed_meta;
            }
            continue;
        }
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

    if (!meta.present) {
        fails++;
        fprintf(stderr,
                "FAIL[meta]: certificate has no metadata line; regenerate or assemble with this version\n");
    } else {
        if (meta.schema != 3) {
            fails++;
            fprintf(stderr, "FAIL[meta]: unsupported certificate schema %d\n",
                    meta.schema);
        }
        if (!meta_root_current(&meta)) {
            fails++;
            fprintf(stderr, "FAIL[meta]: certificate root box does not match this verifier\n");
        }
        if (theta_set && !theta_literals_equal(theta_str, meta.theta_str)) {
            fails++;
            fprintf(stderr,
                    "FAIL[meta]: command-line theta %s does not match certificate theta %s\n",
                    theta_str, meta.theta_str);
        }
        if (form_set && form != meta.form) {
            fails++;
            fprintf(stderr,
                    "FAIL[meta]: command-line form %s does not match certificate form %s\n",
                    form_name(form), form_name(meta.form));
        }
        if (half_set && half != meta.half) {
            fails++;
            fprintf(stderr,
                    "FAIL[meta]: command-line half %d does not match certificate half %d\n",
                    half, meta.half);
        }
        theta_str = meta.theta_str;
        form = meta.form;
        half = meta.half;
    }

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
        long *idx = identity_index(n);
        if (!idx) {
            fails++;
            fprintf(stderr, "FAIL[cover]: out of memory while preparing coverage audit\n");
        } else {
            coverage_checked = 1;
            int cover_fail = verify_cover_node_slice(leaf, idx, n, root_lo, root_hi,
                                                      theta_str, form, half, prec, fenc,
                                                      &worst_cert_fhi,
                                                      &n_disc, &n_cert, &n_flat,
                                                      &n_nonopt, &n_surv, 0);
            fails += cover_fail;
            free(idx);
        }
        if (!fails) {
            for (long i = 0; i < n; i++) {
                if (!leaf[i].used) {
                    fails++;
                    fprintf(stderr, "FAIL[cover]: unused or overlapping leaf at line %ld\n",
                            leaf[i].line_no);
                    break;
                }
            }
        }
    }

    arb_clear(fenc);
    free(leaf);

    printf("verify: %ld leaves  (discard %ld, certify %ld, flat_area %ld, nonoptimal %ld, survivor %ld)  prec=%ld\n",
           n, n_disc, n_cert, n_flat, n_nonopt, n_surv, (long)prec);
    printf("verify: dyadic partition coverage of the full root box: %s\n",
           coverage_checked ? (fails ? "FAILED" : "PASSED") : "SKIPPED");
    printf("verify: worst re-checked certified f_hi = %.15g  (theta = %s)\n",
           worst_cert_fhi, theta_str ? theta_str : "(none)");
    printf("verify: %ld failures -> %s\n", fails, fails ? "AUDIT FAILED" : "AUDIT PASSED");
    return (int)fails;
}

typedef struct {
    cert_leaf leaf;
    char *line;
} cert_record;

static void free_records(cert_record *rec, long n) {
    if (!rec) return;
    for (long i = 0; i < n; i++) free(rec[i].line);
    free(rec);
}

static int load_records(const char *path, cert_record **out, long *nout,
                        cert_meta *meta) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "assemble: cannot open %s\n", path);
        return 1;
    }
    cert_meta_init(meta);

    char *line = NULL;
    size_t cap = 0;
    long n = 0, alloc = 1024, physical_line = 0;
    cert_record *rec = malloc((size_t)alloc * sizeof(*rec));
    if (!rec) {
        fclose(fp);
        fprintf(stderr, "assemble: out of memory\n");
        return 1;
    }

    while (getline(&line, &cap, fp) != -1) {
        physical_line++;
        cert_meta parsed_meta;
        int mp = parse_meta_line(line, &parsed_meta);
        if (mp < 0) {
            fprintf(stderr, "FAIL[assemble]: malformed metadata in %s line %ld\n",
                    path, physical_line);
            free(line); fclose(fp); free_records(rec, n);
            return 1;
        }
        if (mp > 0) {
            if (meta->present) {
                fprintf(stderr, "FAIL[assemble]: duplicate metadata in %s line %ld\n",
                        path, physical_line);
                free(line); fclose(fp); free_records(rec, n);
                return 1;
            }
            *meta = parsed_meta;
            continue;
        }
        double lo[4], hi[4];
        char status[16];
        int item;
        if (!parse_line(line, lo, hi, status, &item)) {
            fprintf(stderr, "FAIL[assemble]: could not parse %s line %ld\n",
                    path, physical_line);
            free(line); fclose(fp); free_records(rec, n);
            return 1;
        }
        int st = parse_status(status);
        if (st < 0) {
            fprintf(stderr, "FAIL[assemble]: unknown status '%s' in %s line %ld\n",
                    status, path, physical_line);
            free(line); fclose(fp); free_records(rec, n);
            return 1;
        }
        if (n == alloc) {
            alloc *= 2;
            cert_record *tmp = realloc(rec, (size_t)alloc * sizeof(*rec));
            if (!tmp) {
                free(line); fclose(fp); free_records(rec, n);
                fprintf(stderr, "assemble: out of memory\n");
                return 1;
            }
            rec = tmp;
        }
        memset(&rec[n], 0, sizeof(rec[n]));
        for (int k = 0; k < 4; k++) {
            rec[n].leaf.lo[k] = lo[k];
            rec[n].leaf.hi[k] = hi[k];
            if (!(lo[k] <= hi[k])) {
                fprintf(stderr, "FAIL[assemble]: inverted interval in %s line %ld\n",
                        path, physical_line);
                free(line); fclose(fp); free_records(rec, n);
                return 1;
            }
        }
        rec[n].leaf.status = st;
        rec[n].leaf.item = item;
        rec[n].leaf.used = 0;
        rec[n].leaf.line_no = physical_line;
        rec[n].line = strdup(line);
        if (!rec[n].line) {
            free(line); fclose(fp); free_records(rec, n);
            fprintf(stderr, "assemble: out of memory\n");
            return 1;
        }
        n++;
    }

    free(line);
    fclose(fp);
    if (!meta->present) {
        fprintf(stderr,
                "FAIL[meta]: %s has no metadata line; regenerate it with this version\n",
                path);
        free_records(rec, n);
        return 1;
    }
    if (meta->schema != 3 || !meta_root_current(meta)) {
        fprintf(stderr, "FAIL[meta]: unsupported metadata in %s\n", path);
        free_records(rec, n);
        return 1;
    }
    *out = rec;
    *nout = n;
    return 0;
}

int cert_assemble(const char **paths, size_t npaths, const char *out_path) {
    if (npaths == 0 || !out_path) {
        fprintf(stderr, "assemble: need at least one input certificate and --out FILE\n");
        return 1;
    }

    cert_record *cur = NULL;
    long ncur = 0;
    cert_meta out_meta;
    if (load_records(paths[0], &cur, &ncur, &out_meta)) return 1;
    if (ncur == 0) {
        fprintf(stderr, "assemble: %s is empty\n", paths[0]);
        free_records(cur, ncur);
        return 1;
    }

    for (size_t step = 1; step < npaths; step++) {
        cert_record *ref = NULL;
        long nref = 0;
        cert_meta ref_meta;
        if (load_records(paths[step], &ref, &nref, &ref_meta)) {
            free_records(cur, ncur);
            return 1;
        }
        if (meta_compatible(&out_meta, &ref_meta, paths[step])) {
            free_records(cur, ncur); free_records(ref, nref);
            return 1;
        }
        out_meta.pair_eq_cert = out_meta.pair_eq_cert || ref_meta.pair_eq_cert;
        out_meta.flat_area_cert = out_meta.flat_area_cert || ref_meta.flat_area_cert;
        if (out_meta.run_prec < 0 || (ref_meta.run_prec >= 0 && ref_meta.run_prec < out_meta.run_prec))
            out_meta.run_prec = ref_meta.run_prec;
        if (nref == 0) {
            fprintf(stderr, "assemble: %s is empty\n", paths[step]);
            free_records(cur, ncur); free_records(ref, nref);
            return 1;
        }

        long nsurv = 0;
        long keep = 0;
        for (long i = 0; i < ncur; i++) {
            if (cur[i].leaf.status == LEAF_SURVIVOR) nsurv++;
            else keep++;
        }
        if (nsurv == 0) {
            fprintf(stderr, "FAIL[assemble]: no survivor boxes remain before %s\n",
                    paths[step]);
            free_records(cur, ncur); free_records(ref, nref);
            return 1;
        }

        long ncombo = keep + nref;
        cert_leaf *combo = malloc((size_t)ncombo * sizeof(*combo));
        if (!combo) {
            fprintf(stderr, "assemble: out of memory\n");
            free_records(cur, ncur); free_records(ref, nref);
            return 1;
        }
        long c = 0;
        for (long i = 0; i < ncur; i++) {
            if (cur[i].leaf.status != LEAF_SURVIVOR) combo[c++] = cur[i].leaf;
        }
        for (long j = 0; j < nref; j++) {
            combo[c++] = ref[j].leaf;
        }
        long *idx = identity_index(ncombo);
        if (!idx) {
            fprintf(stderr, "assemble: out of memory\n");
            free(combo); free_records(cur, ncur); free_records(ref, nref);
            return 1;
        }
        double root_lo[4], root_hi[4];
        search_root_box(root_lo, root_hi);
        int cover_fail = cover_node_slice_structure(combo, idx, ncombo, root_lo, root_hi, 0);
        free(idx);
        if (cover_fail) {
            fprintf(stderr,
                    "FAIL[assemble]: old non-survivors plus %s do not form a full dyadic tiling\n",
                    paths[step]);
            free(combo); free_records(cur, ncur); free_records(ref, nref);
            return 1;
        }
        free(combo);

        cert_record *next = malloc((size_t)(keep + nref) * sizeof(*next));
        if (!next) {
            fprintf(stderr, "assemble: out of memory\n");
            free_records(cur, ncur); free_records(ref, nref);
            return 1;
        }
        long nnext = 0;
        for (long i = 0; i < ncur; i++) {
            if (cur[i].leaf.status == LEAF_SURVIVOR) {
                free(cur[i].line);
            } else {
                next[nnext++] = cur[i];
                cur[i].line = NULL;
            }
        }
        for (long j = 0; j < nref; j++) {
            next[nnext++] = ref[j];
            ref[j].line = NULL;
        }
        free(cur);
        free(ref);
        cur = next;
        ncur = nnext;
        fprintf(stderr, "assembled %s -> %ld leaves\n", paths[step], ncur);
    }

    FILE *out = fopen(out_path, "w");
    if (!out) {
        fprintf(stderr, "assemble: cannot open %s for writing\n", out_path);
        free_records(cur, ncur);
        return 1;
    }
    cert_write_meta_struct(out, &out_meta);
    for (long i = 0; i < ncur; i++) {
        fputs(cur[i].line, out);
        size_t len = strlen(cur[i].line);
        if (len == 0 || cur[i].line[len - 1] != '\n') fputc('\n', out);
    }
    fclose(out);
    printf("assemble: wrote %ld leaves to %s\n", ncur, out_path);
    free_records(cur, ncur);
    return 0;
}
