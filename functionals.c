/* functionals.c -- the six fence bounds as jets, with natural/centered
 * enclosures.  All arithmetic is rigorous arb; floating point only guides the
 * Form1/Form2 branch choice (both forms are exact and equal). */
#include "functionals.h"
#include "series.h"
#include <string.h>

/* ---------- small jet helpers ---------- */

/* out = sqrt( atan2( |cross(P-V,Q-V)|, dot(P-V,Q-V) ) )  -- a vertex bound. */
static void vbound(jet *out, const jpt *V, const jpt *P, const jpt *Q, slong prec) {
    jpt u, w;
    jet cr, dt, acr, ang;
    jpt_init(&u); jpt_init(&w);
    jet_init(&cr); jet_init(&dt); jet_init(&acr); jet_init(&ang);
    jpt_sub(&u, P, V, prec);
    jpt_sub(&w, Q, V, prec);
    jet_cross(&cr, &u, &w, prec);
    jet_dot(&dt, &u, &w, prec);
    jet_abs(&acr, &cr, prec);
    jet_atan2(&ang, &acr, &dt, prec);
    /* If the box contains the degenerate vertex (u or w = 0) the atan2 enclosure
     * is indeterminate.  The interior angle is nonetheless in [0,pi], so clamp
     * the value enclosure to [0,pi] (a sound, loose bound) and mark the
     * derivatives indeterminate so the centered form falls back to natural. */
    if (!arb_is_finite(ang.v)) {
        arb_t pi;
        arf_t z, pu;
        arb_init(pi); arf_init(z); arf_init(pu);
        arb_const_pi(pi, prec);
        arf_zero(z);
        arb_get_ubound_arf(pu, pi, prec);
        arb_set_interval_arf(ang.v, z, pu, prec);        /* [0, pi] */
        for (int k = 0; k < TRAP_NV; k++) arb_indeterminate(ang.d[k]);
        arb_clear(pi); arf_clear(z); arf_clear(pu);
    }
    jet_sqrt(out, &ang, prec);
    jpt_clear(&u); jpt_clear(&w);
    jet_clear(&cr); jet_clear(&dt); jet_clear(&acr); jet_clear(&ang);
}

/* out = distance(P, line through Aline with direction dir) = |cross(P-Aline,dir)|/dlen */
static void pdist(jet *out, const jpt *P, const jpt *Aline, const jpt *dir,
                  const jet *dlen, slong prec) {
    jpt v;
    jet cr, acr;
    jpt_init(&v);
    jet_init(&cr); jet_init(&acr);
    jpt_sub(&v, P, Aline, prec);
    jet_cross(&cr, &v, dir, prec);
    jet_abs(&acr, &cr, prec);
    jet_div(out, &acr, dlen, prec);
    jpt_clear(&v);
    jet_clear(&cr); jet_clear(&acr);
}

/* triangle area(P,Q,O) = 0.5 |cross(Q-P, O-P)| as a jet. */
static void tri_area(jet *out, const jpt *P, const jpt *Q, const jpt *O, slong prec) {
    jpt a, b;
    jet cr, acr;
    jpt_init(&a); jpt_init(&b);
    jet_init(&cr); jet_init(&acr);
    jpt_sub(&a, Q, P, prec);
    jpt_sub(&b, O, P, prec);
    jet_cross(&cr, &a, &b, prec);
    jet_abs(&acr, &cr, prec);
    arb_mul_2exp_si(acr.v, acr.v, -1);
    for (int k = 0; k < TRAP_NV; k++) arb_mul_2exp_si(acr.d[k], acr.d[k], -1);
    jet_set(out, &acr);
    jpt_clear(&a); jpt_clear(&b);
    jet_clear(&cr); jet_clear(&acr);
}

/* Opposite-pair bound.  Pair lines: L1 through A1 with direction d1v, L2 through
 * A2 with direction d2v.  The two non-pair edges are edge a = (ea_l1 on L1,
 * ea_l2 on L2) and edge b = (eb_l1 on L1, eb_l2 on L2).  AQ = quad area.
 *
 *  gamma = atan2(|d1 x d2|, |d1 . d2|)  in [0, pi/2].
 *  Form1 (inf gamma > GAMMA_SPLIT):  O = apex; T0,T1 = triangle areas of the
 *     non-pair edges with apex O;  Fpair^2 = gamma (T0+T1)/AQ.
 *  Form2 (inf gamma <= GAMMA_SPLIT, parallel-safe, symmetric):
 *     Fpair^2 = (gamma/sin gamma) (prodA + prodB) / (2 AQ),
 *     prodX = d(X_L1, L2) d(X_L2, L1).
 *  The two forms are algebraically identical (see README); out = sqrt(Fpair^2).
 */
/* force: -1 = auto (branch on inf gamma), 0 = Form1, 1 = Form2 (for tests). */
static void pair_bound(jet *out,
                       const jpt *A1, const jpt *d1v, const jpt *A2, const jpt *d2v,
                       const jpt *ea_l1, const jpt *ea_l2,
                       const jpt *eb_l1, const jpt *eb_l2,
                       const jet *AQ, slong prec, int force) {
    jet crossd, dotd, acrossd, adotd, gamma, fp2;
    jet_init(&crossd); jet_init(&dotd); jet_init(&acrossd); jet_init(&adotd);
    jet_init(&gamma); jet_init(&fp2);

    jet_cross(&crossd, d1v, d2v, prec);
    jet_dot(&dotd, d1v, d2v, prec);
    jet_abs(&acrossd, &crossd, prec);
    jet_abs(&adotd, &dotd, prec);
    jet_atan2(&gamma, &acrossd, &adotd, prec);

    /* branch on the lower bound of gamma (unless forced) */
    int use_form1;
    if (force == 0)      use_form1 = 1;
    else if (force == 1) use_form1 = 0;
    else {
        arf_t glb;
        arf_init(glb);
        arb_get_lbound_arf(glb, gamma.v, prec);
        use_form1 = (arf_cmp_d(glb, GAMMA_SPLIT) > 0);
        arf_clear(glb);
    }

    if (use_form1) {
        /* O = A1 + r d1v,  r = cross(A2-A1, d2v) / cross(d1v,d2v) */
        jpt w, O;
        jet r, num, tx, ty;
        jpt_init(&w); jpt_init(&O);
        jet_init(&r); jet_init(&num); jet_init(&tx); jet_init(&ty);
        jpt_sub(&w, A2, A1, prec);
        jet_cross(&num, &w, d2v, prec);
        jet_div(&r, &num, &crossd, prec);
        jet_mul(&tx, &r, &d1v->x, prec);
        jet_mul(&ty, &r, &d1v->y, prec);
        jet_add(&O.x, &A1->x, &tx, prec);
        jet_add(&O.y, &A1->y, &ty, prec);

        jet T0, T1, Tsum, tmp;
        jet_init(&T0); jet_init(&T1); jet_init(&Tsum); jet_init(&tmp);
        tri_area(&T0, ea_l1, ea_l2, &O, prec);
        tri_area(&T1, eb_l1, eb_l2, &O, prec);
        jet_add(&Tsum, &T0, &T1, prec);
        jet_mul(&tmp, &gamma, &Tsum, prec);
        jet_div(&fp2, &tmp, AQ, prec);

        jpt_clear(&w); jpt_clear(&O);
        jet_clear(&r); jet_clear(&num); jet_clear(&tx); jet_clear(&ty);
        jet_clear(&T0); jet_clear(&T1); jet_clear(&Tsum); jet_clear(&tmp);
    } else {
        /* parallel-safe symmetric form */
        jet g, dot1, dot2, len1, len2;
        jet_init(&g); jet_init(&dot1); jet_init(&dot2); jet_init(&len1); jet_init(&len2);
        jet_gamma_over_sin(&g, &gamma, prec);
        jet_dot(&dot1, d1v, d1v, prec);  jet_sqrt(&len1, &dot1, prec);
        jet_dot(&dot2, d2v, d2v, prec);  jet_sqrt(&len2, &dot2, prec);

        jet da1, da2, db1, db2, prodA, prodB, psum, tmp;
        jet_init(&da1); jet_init(&da2); jet_init(&db1); jet_init(&db2);
        jet_init(&prodA); jet_init(&prodB); jet_init(&psum); jet_init(&tmp);
        /* edge a: ea_l1 on L1 -> dist to L2 ; ea_l2 on L2 -> dist to L1 */
        pdist(&da1, ea_l1, A2, d2v, &len2, prec);
        pdist(&da2, ea_l2, A1, d1v, &len1, prec);
        jet_mul(&prodA, &da1, &da2, prec);
        pdist(&db1, eb_l1, A2, d2v, &len2, prec);
        pdist(&db2, eb_l2, A1, d1v, &len1, prec);
        jet_mul(&prodB, &db1, &db2, prec);
        jet_add(&psum, &prodA, &prodB, prec);
        jet_mul(&tmp, &g, &psum, prec);
        jet_div(&fp2, &tmp, AQ, prec);
        /* divide by 2 */
        arb_mul_2exp_si(fp2.v, fp2.v, -1);
        for (int k = 0; k < TRAP_NV; k++) arb_mul_2exp_si(fp2.d[k], fp2.d[k], -1);

        jet_clear(&g); jet_clear(&dot1); jet_clear(&dot2); jet_clear(&len1); jet_clear(&len2);
        jet_clear(&da1); jet_clear(&da2); jet_clear(&db1); jet_clear(&db2);
        jet_clear(&prodA); jet_clear(&prodB); jet_clear(&psum); jet_clear(&tmp);
    }
    jet_sqrt(out, &fp2, prec);

    jet_clear(&crossd); jet_clear(&dotd); jet_clear(&acrossd); jet_clear(&adotd);
    jet_clear(&gamma); jet_clear(&fp2);
}

/* Build the six item jets from the four seeded coordinate jets. */
static void compute_items(jet item[6], const jet var[4], slong prec) {
    jpt A, B, C, D;
    jpt_init(&A); jpt_init(&B); jpt_init(&C); jpt_init(&D);
    jet_set_si(&A.x, 0); jet_set_si(&A.y, 0);
    jet_set_si(&B.x, 1); jet_set_si(&B.y, 0);
    jet_set(&C.x, &var[0]); jet_set(&C.y, &var[1]);
    jet_set(&D.x, &var[2]); jet_set(&D.y, &var[3]);

    /* A_Q = (c2 + c1 d2 - d1 c2)/2 */
    jet AQ, t1, t2;
    jet_init(&AQ); jet_init(&t1); jet_init(&t2);
    jet_mul(&t1, &var[0], &var[3], prec);   /* c1 d2 */
    jet_mul(&t2, &var[2], &var[1], prec);   /* d1 c2 */
    jet_add(&AQ, &var[1], &t1, prec);
    jet_sub(&AQ, &AQ, &t2, prec);
    arb_mul_2exp_si(AQ.v, AQ.v, -1);
    for (int k = 0; k < TRAP_NV; k++) arb_mul_2exp_si(AQ.d[k], AQ.d[k], -1);
    jet_clear(&t1); jet_clear(&t2);

    /* vertex bounds: at V, edges to its two neighbours */
    vbound(&item[0], &A, &D, &B, prec);   /* vertex A: neighbours D,B */
    vbound(&item[1], &B, &A, &C, prec);   /* vertex B: neighbours A,C */
    vbound(&item[2], &C, &B, &D, prec);   /* vertex C: neighbours B,D */
    vbound(&item[3], &D, &C, &A, prec);   /* vertex D: neighbours C,A */

    /* pair (AB,CD): L1 through A dir AB=B-A; L2 through C dir CD=D-C.
     * non-pair edges: BC=(B on L1, C on L2), DA=(A on L1, D on L2). */
    jpt dAB, dCD;
    jpt_init(&dAB); jpt_init(&dCD);
    jpt_sub(&dAB, &B, &A, prec);
    jpt_sub(&dCD, &D, &C, prec);
    pair_bound(&item[4], &A, &dAB, &C, &dCD,
               &B, &C,      /* edge BC: L1pt=B, L2pt=C */
               &A, &D,      /* edge DA: L1pt=A, L2pt=D */
               &AQ, prec, -1);

    /* pair (BC,DA): L1 through B dir BC=C-B; L2 through D dir DA=A-D.
     * non-pair edges: AB=(B on L1, A on L2), CD=(C on L1, D on L2). */
    jpt dBC, dDA;
    jpt_init(&dBC); jpt_init(&dDA);
    jpt_sub(&dBC, &C, &B, prec);
    jpt_sub(&dDA, &A, &D, prec);
    pair_bound(&item[5], &B, &dBC, &D, &dDA,
               &B, &A,      /* edge AB: L1pt=B, L2pt=A */
               &C, &D,      /* edge CD: L1pt=C, L2pt=D */
               &AQ, prec, -1);

    jpt_clear(&dAB); jpt_clear(&dCD); jpt_clear(&dBC); jpt_clear(&dDA);
    jet_clear(&AQ);
    jpt_clear(&A); jpt_clear(&B); jpt_clear(&C); jpt_clear(&D);
}

void functionals_eval(arb_t encl[6], const double lo[4], const double hi[4],
                      enclosure_form form, slong prec) {
    jet vbox[4], vmid[4], ibox[6], imid[6];
    arb_t box_k, mid_k, rad_k;
    arb_init(box_k); arb_init(mid_k); arb_init(rad_k);
    for (int k = 0; k < 4; k++) { jet_init(&vbox[k]); jet_init(&vmid[k]); }
    for (int i = 0; i < 6; i++) { jet_init(&ibox[i]); jet_init(&imid[i]); }

    for (int k = 0; k < 4; k++) {
        arf_t L, U;
        arf_init(L); arf_init(U);
        arf_set_d(L, lo[k]); arf_set_d(U, hi[k]);
        arb_set_interval_arf(box_k, L, U, prec);
        jet_set_var(&vbox[k], box_k, k);
        /* midpoint (thin) and radius */
        arb_set_arf(mid_k, L);
        {
            arb_t hb; arb_init(hb); arb_set_arf(hb, U);
            arb_add(mid_k, mid_k, hb, prec);
            arb_mul_2exp_si(mid_k, mid_k, -1);          /* (lo+hi)/2 */
            arb_clear(hb);
        }
        jet_set_var(&vmid[k], mid_k, k);
        arf_clear(L); arf_clear(U);
    }

    compute_items(ibox, vbox, prec);
    if (form == FORM_CENTERED) compute_items(imid, vmid, prec);

    for (int i = 0; i < 6; i++) {
        if (form == FORM_NATURAL) {
            arb_set(encl[i], ibox[i].v);
        } else {
            /* centered: item(mid) + sum_k J_k(B) * [-rad_k, rad_k] */
            arb_t acc, term, radint;
            arb_init(acc); arb_init(term); arb_init(radint);
            arb_set(acc, imid[i].v);
            for (int k = 0; k < 4; k++) {
                arf_t L, U, r;
                arf_init(L); arf_init(U); arf_init(r);
                arf_set_d(L, lo[k]); arf_set_d(U, hi[k]);
                arf_sub(r, U, L, prec, ARF_RND_UP);
                arf_mul_2exp_si(r, r, -1);              /* rad_k >= (hi-lo)/2 */
                arf_t nr; arf_init(nr); arf_neg(nr, r);
                arb_set_interval_arf(radint, nr, r, prec);
                arb_mul(term, ibox[i].d[k], radint, prec);
                arb_add(acc, acc, term, prec);
                arf_clear(L); arf_clear(U); arf_clear(r); arf_clear(nr);
            }
            /* Combine with the natural enclosure.  If the mean-value form is
             * non-finite (e.g. angle->0 makes a sqrt derivative blow up), fall
             * back to the natural enclosure.  If both are finite but fail to
             * overlap, keep their hull: this is conservative and makes such a
             * disagreement visible as a wider box instead of silently trusting
             * one enclosure. */
            if (!arb_is_finite(acc)) {
                arb_set(encl[i], ibox[i].v);
            } else if (!arb_is_finite(ibox[i].v)) {
                arb_set(encl[i], acc);
            } else if (!arb_intersection(encl[i], acc, ibox[i].v, prec)) {
                arb_union(encl[i], acc, ibox[i].v, prec);
            }
            arb_clear(acc); arb_clear(term); arb_clear(radint);
        }
    }

    for (int k = 0; k < 4; k++) { jet_clear(&vbox[k]); jet_clear(&vmid[k]); }
    for (int i = 0; i < 6; i++) { jet_clear(&ibox[i]); jet_clear(&imid[i]); }
    arb_clear(box_k); arb_clear(mid_k); arb_clear(rad_k);
}

/* Test hook: evaluate a single pair bound at the point coord[4], forcing the
 * form (0=Form1 apex, 1=Form2 parallel-safe).  pair=0 -> (AB,CD), 1 -> (BC,DA). */
void functionals_pair_forced(arb_t out, const double coord[4], int pair,
                             int force, slong prec) {
    jet var[4];
    arb_t v;
    arb_init(v);
    for (int k = 0; k < 4; k++) {
        jet_init(&var[k]);
        arb_set_d(v, coord[k]);
        jet_set_var(&var[k], v, k);
    }
    jpt A, B, C, D;
    jpt_init(&A); jpt_init(&B); jpt_init(&C); jpt_init(&D);
    jet_set_si(&A.x, 0); jet_set_si(&A.y, 0);
    jet_set_si(&B.x, 1); jet_set_si(&B.y, 0);
    jet_set(&C.x, &var[0]); jet_set(&C.y, &var[1]);
    jet_set(&D.x, &var[2]); jet_set(&D.y, &var[3]);

    jet AQ, t1, t2;
    jet_init(&AQ); jet_init(&t1); jet_init(&t2);
    jet_mul(&t1, &var[0], &var[3], prec);
    jet_mul(&t2, &var[2], &var[1], prec);
    jet_add(&AQ, &var[1], &t1, prec);
    jet_sub(&AQ, &AQ, &t2, prec);
    arb_mul_2exp_si(AQ.v, AQ.v, -1);
    for (int k = 0; k < TRAP_NV; k++) arb_mul_2exp_si(AQ.d[k], AQ.d[k], -1);

    jet res;
    jet_init(&res);
    if (pair == 0) {
        jpt dAB, dCD;
        jpt_init(&dAB); jpt_init(&dCD);
        jpt_sub(&dAB, &B, &A, prec);
        jpt_sub(&dCD, &D, &C, prec);
        pair_bound(&res, &A, &dAB, &C, &dCD, &B, &C, &A, &D, &AQ, prec, force);
        jpt_clear(&dAB); jpt_clear(&dCD);
    } else {
        jpt dBC, dDA;
        jpt_init(&dBC); jpt_init(&dDA);
        jpt_sub(&dBC, &C, &B, prec);
        jpt_sub(&dDA, &A, &D, prec);
        pair_bound(&res, &B, &dBC, &D, &dDA, &B, &A, &C, &D, &AQ, prec, force);
        jpt_clear(&dBC); jpt_clear(&dDA);
    }
    arb_set(out, res.v);
    jet_clear(&res);
    jet_clear(&AQ); jet_clear(&t1); jet_clear(&t2);
    jpt_clear(&A); jpt_clear(&B); jpt_clear(&C); jpt_clear(&D);
    for (int k = 0; k < 4; k++) jet_clear(&var[k]);
    arb_clear(v);
}

int functionals_opposite_pairs_disjoint(const double lo[4], const double hi[4],
                                        enclosure_form form, slong prec) {
    arb_t encl[6];
    for (int i = 0; i < 6; i++) arb_init(encl[i]);
    functionals_eval(encl, lo, hi, form, prec);

    int disjoint = arb_is_finite(encl[4]) && arb_is_finite(encl[5])
                   && !arb_overlaps(encl[4], encl[5]);

    for (int i = 0; i < 6; i++) arb_clear(encl[i]);
    return disjoint;
}

int functionals_min(arb_t fenc, const double lo[4], const double hi[4],
                    enclosure_form form, slong prec) {
    arb_t encl[6];
    for (int i = 0; i < 6; i++) arb_init(encl[i]);
    functionals_eval(encl, lo, hi, form, prec);

    /* f = min over items.  Non-finite item enclosures (failed to bound, e.g.
     * pair bound where A_Q straddles 0) are dropped: this can only enlarge the
     * min, so certification (f_hi <= theta) stays sound.  If every item is
     * non-finite, f is set to +inf so the caller must split, never certify. */
    arf_t flo, fhi, li, ui;
    arf_init(flo); arf_init(fhi); arf_init(li); arf_init(ui);
    int active = -1, any = 0;
    for (int i = 0; i < 6; i++) {
        if (!arb_is_finite(encl[i])) continue;
        arb_get_interval_arf(li, ui, encl[i], prec);
        if (!any) { arf_set(flo, li); arf_set(fhi, ui); active = i; any = 1; }
        else {
            if (arf_cmp(li, flo) < 0) arf_set(flo, li);
            if (arf_cmp(ui, fhi) < 0) { arf_set(fhi, ui); active = i; }
        }
    }
    if (any) arb_set_interval_arf(fenc, flo, fhi, prec);
    else     arb_pos_inf(fenc);

    arf_clear(flo); arf_clear(fhi); arf_clear(li); arf_clear(ui);
    for (int i = 0; i < 6; i++) arb_clear(encl[i]);
    return active;
}
