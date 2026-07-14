/* fence_validate.c -- CLI driver for the interval fence validator.
 *
 * Usage:
 *   fence_validate [--theta T] [--wfloor W] [--prec P] [--maxprec M]
 *                  [--form natural|centered] [--half] [--serial]
 *                  [--adaptive] [--max-leaves N] [--out cert.jsonl] [--stats]
 *   fence_validate --verify cert.jsonl [--theta T] [--form ...] [--half]
 *                  [--prec P]
 *
 * Certifies boxes where an upper bound for the normalized shortest fence is
 * at most theta, leaving an outer survivor neighborhood of possible values
 * above theta.
 */
#define _POSIX_C_SOURCE 200809L
#include "search.h"
#include "cert.h"
#include "series.h"
#include "threshold.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void usage(const char *prog) {
    fprintf(stderr,
"Usage: %s [options]\n"
"  --theta T        exact decimal threshold (default 1.04)\n"
"  --wfloor W       survivor scaled-width floor (default 1e-2)\n"
"  --prec P         starting arb precision in bits (default 53)\n"
"  --maxprec M      adaptive precision cap (default 256)\n"
"  --form F         natural | centered   (default centered)\n"
"  --half           quotient the x->1-x, C<->D reflection symmetry\n"
"  --serial         force single-threaded\n"
"  --adaptive       retry marginal undecided boxes at higher precision\n"
"  --pair-eq-cert   certify nonoptimal boxes when opposite-pair intervals are disjoint\n"
"  --flat-area-cert certify low boxes with area upper bound <= theta^2/4\n"
"  --max-leaves N   debugging cap; truncated output will not verify fully\n"
"  --out FILE       write JSONL certificate stream\n"
"  --stats          print summary statistics\n"
"  --verify FILE    re-check metadata, local claims, and full dyadic coverage\n"
"  --assemble FILE [FILE ...]\n"
"                   assemble base/refinement certificates into one full JSONL\n"
"                   certificate; requires --out FILE\n"
"  --eval c1 c2 d1 d2   print the six items and f at a single point\n"
"  --refine FILE    seed the search from FILE's survivor boxes (use a smaller\n"
"                   --wfloor); output + FILE's certified/discarded leaves form\n"
"                   a complete certificate of the original domain\n",
        prog);
}

int main(int argc, char **argv) {
    search_params p;
    snprintf(p.theta_str, sizeof p.theta_str, "1.04");
    p.theta = threshold_to_double(p.theta_str);
    p.wfloor = 1e-2;
    p.prec = 53;
    p.maxprec = 256;
    p.half = 0;
    p.adaptive_prec = 0;
    p.pair_eq_cert = 0;
    p.flat_area_cert = 0;
    p.form = FORM_CENTERED;
    p.max_leaves = 0;
    p.serial = 0;

    const char *out_path = NULL;
    const char *verify_path = NULL;
    const char *refine_path = NULL;
    const char **assemble_paths = malloc((size_t)argc * sizeof(*assemble_paths));
    size_t n_assemble = 0;
    int do_stats = 0;
    int do_eval = 0;
    int theta_set = 0, form_set = 0, half_set = 0;
    double evalpt[4] = {0, 0, 0, 0};

    if (!assemble_paths) {
        fprintf(stderr, "out of memory\n");
        return 2;
    }

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--theta") && i + 1 < argc) {
            const char *s = argv[++i];
            fmpq_t q;
            fmpq_init(q);
            int bad = threshold_parse_fmpq(q, s);
            fmpq_clear(q);
            if (bad || strlen(s) >= sizeof p.theta_str) {
                fprintf(stderr, "invalid --theta literal %s\n", s);
                return 2;
            }
            snprintf(p.theta_str, sizeof p.theta_str, "%s", s);
            p.theta = threshold_to_double(p.theta_str);
            theta_set = 1;
        }
        else if (!strcmp(argv[i], "--wfloor") && i + 1 < argc) p.wfloor = atof(argv[++i]);
        else if (!strcmp(argv[i], "--prec") && i + 1 < argc) p.prec = atol(argv[++i]);
        else if (!strcmp(argv[i], "--maxprec") && i + 1 < argc) p.maxprec = atol(argv[++i]);
        else if (!strcmp(argv[i], "--form") && i + 1 < argc) {
            const char *f = argv[++i];
            if (!strcmp(f, "natural")) p.form = FORM_NATURAL;
            else if (!strcmp(f, "centered")) p.form = FORM_CENTERED;
            else { fprintf(stderr, "unknown --form %s\n", f); return 2; }
            form_set = 1;
        }
        else if (!strcmp(argv[i], "--half")) {
            p.half = 1;
            half_set = 1;
        }
        else if (!strcmp(argv[i], "--serial")) p.serial = 1;
        else if (!strcmp(argv[i], "--adaptive")) p.adaptive_prec = 1;
        else if (!strcmp(argv[i], "--pair-eq-cert")) p.pair_eq_cert = 1;
        else if (!strcmp(argv[i], "--flat-area-cert")) p.flat_area_cert = 1;
        else if (!strcmp(argv[i], "--max-leaves") && i + 1 < argc) p.max_leaves = atol(argv[++i]);
        else if (!strcmp(argv[i], "--out") && i + 1 < argc) out_path = argv[++i];
        else if (!strcmp(argv[i], "--stats")) do_stats = 1;
        else if (!strcmp(argv[i], "--verify") && i + 1 < argc) verify_path = argv[++i];
        else if (!strcmp(argv[i], "--assemble")) {
            while (i + 1 < argc && strncmp(argv[i + 1], "--", 2) != 0) {
                assemble_paths[n_assemble++] = argv[++i];
            }
            if (n_assemble == 0) {
                fprintf(stderr, "--assemble requires at least one input file\n");
                free(assemble_paths);
                return 2;
            }
        }
        else if (!strcmp(argv[i], "--refine") && i + 1 < argc) refine_path = argv[++i];
        else if (!strcmp(argv[i], "--eval") && i + 4 < argc) {
            do_eval = 1;
            for (int k = 0; k < 4; k++) evalpt[k] = atof(argv[++i]);
        }
        else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) { usage(argv[0]); return 0; }
        else { fprintf(stderr, "unknown argument: %s\n", argv[i]); usage(argv[0]); return 2; }
    }

    if (n_assemble > 0) {
        if (!out_path) {
            fprintf(stderr, "--assemble requires --out FILE\n");
            free(assemble_paths);
            return 2;
        }
        int fails = cert_assemble(assemble_paths, n_assemble, out_path);
        free(assemble_paths);
        series_cleanup();
        return fails ? 1 : 0;
    }
    free(assemble_paths);

    if (do_eval) {
        static const char *nm[6] = {"vertA", "vertB", "vertC", "vertD",
                                    "pair(AB,CD)", "pair(BC,DA)"};
        arb_t e[6], f;
        for (int i = 0; i < 6; i++) arb_init(e[i]);
        arb_init(f);
        functionals_eval(e, evalpt, evalpt, FORM_NATURAL, p.prec);
        int act = functionals_min(f, evalpt, evalpt, FORM_NATURAL, p.prec);
        printf("point: c1=%.17g c2=%.17g d1=%.17g d2=%.17g\n",
               evalpt[0], evalpt[1], evalpt[2], evalpt[3]);
        for (int i = 0; i < 6; i++) {
            printf("  %-12s = ", nm[i]);
            arb_printn(e[i], 20, 0);
            printf("\n");
        }
        printf("  f = min      = ");
        arb_printn(f, 20, 0);
        printf("   (active item %d: %s)\n", act, act >= 0 ? nm[act] : "none");
        for (int i = 0; i < 6; i++) arb_clear(e[i]);
        arb_clear(f);
        series_cleanup();
        return 0;
    }

    if (verify_path) {
        int fails = cert_verify(verify_path, p.theta_str, theta_set,
                                p.form, form_set, p.half, half_set, p.prec);
        series_cleanup();
        return fails ? 1 : 0;
    }

    FILE *out = NULL;
    cert_writer w = { NULL };
    leaf_fn cb = NULL;
    if (out_path) {
        out = fopen(out_path, "w");
        if (!out) { fprintf(stderr, "cannot open %s for writing\n", out_path); return 2; }
        cert_write_meta(out, &p);
        w.fp = out;
        cb = cert_write_leaf;
    }

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    search_stats st;
    if (refine_path) {
        double (*slo)[4] = NULL, (*shi)[4] = NULL;
        size_t nseeds = cert_load_survivors(refine_path, &slo, &shi);
        if (nseeds == 0) {
            fprintf(stderr, "no survivor boxes found in %s\n", refine_path);
            if (out) fclose(out);
            free(slo); free(shi);
            return 2;
        }
        fprintf(stderr, "refining %zu survivor boxes from %s\n", nseeds, refine_path);
        search_run_seeded(&p, &st, cb, cb ? (void *)&w : NULL,
                          (const double (*)[4])slo, (const double (*)[4])shi,
                          nseeds);
        free(slo); free(shi);
    } else {
        search_run(&p, &st, cb, cb ? (void *)&w : NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double wall = (t1.tv_sec - t0.tv_sec) + 1e-9 * (t1.tv_nsec - t0.tv_nsec);

    if (out) fclose(out);

    if (do_stats || !out_path) {
        printf("=== fence_validate summary ===\n");
        printf("theta=%g  wfloor=%g  prec=%ld  form=%s  half=%d  adaptive=%d  pair_eq_cert=%d  flat_area_cert=%d\n",
               p.theta, p.wfloor, (long)p.prec,
               p.form == FORM_CENTERED ? "centered" : "natural", p.half,
               p.adaptive_prec, p.pair_eq_cert, p.flat_area_cert);
        printf("leaves        : %ld  (internal nodes %ld)\n", st.n_leaves, st.n_internal);
        printf("  discarded   : %ld\n", st.n_discard);
        printf("  certified   : %ld\n", st.n_certified);
        printf("  flat_area   : %ld\n", st.n_flat_area);
        printf("  nonoptimal  : %ld\n", st.n_nonoptimal);
        printf("  survivors   : %ld\n", st.n_survivor);
        if (st.have_survivor) {
            printf("survivor bbox : c1[%.6g,%.6g] c2[%.6g,%.6g] d1[%.6g,%.6g] d2[%.6g,%.6g]\n",
                   st.surv_lo[0], st.surv_hi[0], st.surv_lo[1], st.surv_hi[1],
                   st.surv_lo[2], st.surv_hi[2], st.surv_lo[3], st.surv_hi[3]);
            printf("survivor vol  : <= %.6g (physical, sum of box volumes)\n", st.surv_vol);
            if (st.surv_vol > 0)
                printf("survivor centroid (vol-weighted): c1=%.4f c2=%.4f d1=%.4f d2=%.4f\n",
                       st.surv_centroid[0]/st.surv_vol, st.surv_centroid[1]/st.surv_vol,
                       st.surv_centroid[2]/st.surv_vol, st.surv_centroid[3]/st.surv_vol);
        } else {
            printf("survivor bbox : (none)\n");
        }
        printf("max prec used : %ld bits\n", (long)st.max_prec_used);
        printf("max certified f_hi (self-audit, must be <= theta): %.15g  -> %s\n",
               st.max_cert_fhi,
               (st.n_certified == 0 || st.max_cert_fhi <= p.theta) ? "OK" : "VIOLATION");
        printf("wall time     : %.3f s\n", wall);
        if (p.half)
            printf("note: --half active; full survivor set is the union of this result "
                   "with its mirror under x->1-x, C<->D.\n");
    }

    series_cleanup();
    return 0;
}
