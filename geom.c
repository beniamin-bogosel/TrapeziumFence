/* geom.c -- jet (forward-mode AD over arb) arithmetic and scalar geometry. */
#include "geom.h"

void jet_init(jet *a) {
    arb_init(a->v);
    for (int k = 0; k < TRAP_NV; k++) arb_init(a->d[k]);
}
void jet_clear(jet *a) {
    arb_clear(a->v);
    for (int k = 0; k < TRAP_NV; k++) arb_clear(a->d[k]);
}
void jet_set(jet *r, const jet *a) {
    if (r == a) return;
    arb_set(r->v, a->v);
    for (int k = 0; k < TRAP_NV; k++) arb_set(r->d[k], a->d[k]);
}
void jet_set_var(jet *r, const arb_t val, int k) {
    arb_set(r->v, val);
    for (int j = 0; j < TRAP_NV; j++) arb_zero(r->d[j]);
    arb_one(r->d[k]);
}
void jet_set_const(jet *r, const arb_t c) {
    arb_set(r->v, c);
    for (int j = 0; j < TRAP_NV; j++) arb_zero(r->d[j]);
}
void jet_set_si(jet *r, slong c) {
    arb_set_si(r->v, c);
    for (int j = 0; j < TRAP_NV; j++) arb_zero(r->d[j]);
}

void jet_add(jet *r, const jet *a, const jet *b, slong prec) {
    arb_add(r->v, a->v, b->v, prec);
    for (int k = 0; k < TRAP_NV; k++) arb_add(r->d[k], a->d[k], b->d[k], prec);
}
void jet_sub(jet *r, const jet *a, const jet *b, slong prec) {
    arb_sub(r->v, a->v, b->v, prec);
    for (int k = 0; k < TRAP_NV; k++) arb_sub(r->d[k], a->d[k], b->d[k], prec);
}
void jet_neg(jet *r, const jet *a) {
    arb_neg(r->v, a->v);
    for (int k = 0; k < TRAP_NV; k++) arb_neg(r->d[k], a->d[k]);
}

void jet_mul(jet *r, const jet *a, const jet *b, slong prec) {
    /* d(ab) = a' b + a b'.  Use temporaries so r may alias a or b. */
    arb_t t1, t2, rv;
    arb_init(t1); arb_init(t2); arb_init(rv);
    arb_mul(rv, a->v, b->v, prec);
    for (int k = 0; k < TRAP_NV; k++) {
        arb_mul(t1, a->d[k], b->v, prec);
        arb_mul(t2, a->v, b->d[k], prec);
        arb_add(r->d[k], t1, t2, prec);
    }
    arb_set(r->v, rv);
    arb_clear(t1); arb_clear(t2); arb_clear(rv);
}

void jet_div(jet *r, const jet *a, const jet *b, slong prec) {
    /* d(a/b) = (a' b - a b') / b^2 */
    arb_t b2, t1, t2, rv;
    arb_init(b2); arb_init(t1); arb_init(t2); arb_init(rv);
    arb_mul(b2, b->v, b->v, prec);
    arb_div(rv, a->v, b->v, prec);
    for (int k = 0; k < TRAP_NV; k++) {
        arb_mul(t1, a->d[k], b->v, prec);
        arb_mul(t2, a->v, b->d[k], prec);
        arb_sub(t1, t1, t2, prec);
        arb_div(r->d[k], t1, b2, prec);
    }
    arb_set(r->v, rv);
    arb_clear(b2); arb_clear(t1); arb_clear(t2); arb_clear(rv);
}

void jet_sqr(jet *r, const jet *a, slong prec) {
    /* d(a^2) = 2 a a' */
    arb_t rv;
    arb_init(rv);
    arb_mul(rv, a->v, a->v, prec);
    for (int k = 0; k < TRAP_NV; k++) {
        arb_mul(r->d[k], a->v, a->d[k], prec);
        arb_mul_2exp_si(r->d[k], r->d[k], 1);
    }
    arb_set(r->v, rv);
    arb_clear(rv);
}

void jet_sqrt(jet *r, const jet *a, slong prec) {
    /* d(sqrt a) = a' / (2 sqrt a) */
    arb_t s, den;
    arb_init(s); arb_init(den);
    arb_sqrt(s, a->v, prec);
    arb_mul_2exp_si(den, s, 1);      /* 2 sqrt a */
    for (int k = 0; k < TRAP_NV; k++)
        arb_div(r->d[k], a->d[k], den, prec);
    arb_set(r->v, s);
    arb_clear(s); arb_clear(den);
}

void jet_abs(jet *r, const jet *a, slong prec) {
    (void) prec;
    if (arb_is_nonnegative(a->v)) {
        jet_set(r, a);
    } else if (arb_is_nonpositive(a->v)) {
        jet_neg(r, a);
    } else {
        /* Straddles zero: value = [0, max|a|]; each partial gets sign
         * uncertainty, enclosed as a symmetric ball of radius |a'|. */
        arb_t hi;
        arb_init(hi);
        arb_abs(hi, a->v);                 /* [0, max|a|] after next step */
        /* arb_abs on a straddling ball yields [0, sup|a|]; good. */
        arb_set(r->v, hi);
        for (int k = 0; k < TRAP_NV; k++) {
            mag_t m;
            mag_init(m);
            arb_get_mag(m, a->d[k]);       /* upper bound on |a'| */
            arb_zero(r->d[k]);
            arb_add_error_mag(r->d[k], m);  /* [-|a'|, |a'|] */
            mag_clear(m);
        }
        arb_clear(hi);
    }
}

void jet_atan2(jet *r, const jet *y, const jet *x, slong prec) {
    /* atan2(y,x); d = (x y' - y x')/(x^2 + y^2) */
    arb_t den, x2, y2, t1, t2, rv;
    arb_init(den); arb_init(x2); arb_init(y2);
    arb_init(t1); arb_init(t2); arb_init(rv);
    arb_atan2(rv, y->v, x->v, prec);
    arb_mul(x2, x->v, x->v, prec);
    arb_mul(y2, y->v, y->v, prec);
    arb_add(den, x2, y2, prec);
    for (int k = 0; k < TRAP_NV; k++) {
        arb_mul(t1, x->v, y->d[k], prec);
        arb_mul(t2, y->v, x->d[k], prec);
        arb_sub(t1, t1, t2, prec);
        arb_div(r->d[k], t1, den, prec);
    }
    arb_set(r->v, rv);
    arb_clear(den); arb_clear(x2); arb_clear(y2);
    arb_clear(t1); arb_clear(t2); arb_clear(rv);
}

void jet_mul_si(jet *r, const jet *a, slong c, slong prec) {
    arb_mul_si(r->v, a->v, c, prec);
    for (int k = 0; k < TRAP_NV; k++) arb_mul_si(r->d[k], a->d[k], c, prec);
}
void jet_mul_arb(jet *r, const jet *a, const arb_t s, slong prec) {
    arb_mul(r->v, a->v, s, prec);
    for (int k = 0; k < TRAP_NV; k++) arb_mul(r->d[k], a->d[k], s, prec);
}

/* ---- points ---- */
void jpt_init(jpt *p) { jet_init(&p->x); jet_init(&p->y); }
void jpt_clear(jpt *p) { jet_clear(&p->x); jet_clear(&p->y); }
void jpt_sub(jpt *r, const jpt *a, const jpt *b, slong prec) {
    jet_sub(&r->x, &a->x, &b->x, prec);
    jet_sub(&r->y, &a->y, &b->y, prec);
}
void jet_cross(jet *r, const jpt *a, const jpt *b, slong prec) {
    jet t1, t2;
    jet_init(&t1); jet_init(&t2);
    jet_mul(&t1, &a->x, &b->y, prec);
    jet_mul(&t2, &a->y, &b->x, prec);
    jet_sub(r, &t1, &t2, prec);
    jet_clear(&t1); jet_clear(&t2);
}
void jet_dot(jet *r, const jpt *a, const jpt *b, slong prec) {
    jet t1, t2;
    jet_init(&t1); jet_init(&t2);
    jet_mul(&t1, &a->x, &b->x, prec);
    jet_mul(&t2, &a->y, &b->y, prec);
    jet_add(r, &t1, &t2, prec);
    jet_clear(&t1); jet_clear(&t2);
}

/* ---- scalar geometry ---- */
void trap_cross(arb_t r, const arb_t ax, const arb_t ay,
                const arb_t bx, const arb_t by, slong prec) {
    arb_t t;
    arb_init(t);
    arb_mul(r, ax, by, prec);
    arb_mul(t, ay, bx, prec);
    arb_sub(r, r, t, prec);
    arb_clear(t);
}
void trap_dot(arb_t r, const arb_t ax, const arb_t ay,
              const arb_t bx, const arb_t by, slong prec) {
    arb_t t;
    arb_init(t);
    arb_mul(r, ax, bx, prec);
    arb_mul(t, ay, by, prec);
    arb_add(r, r, t, prec);
    arb_clear(t);
}
