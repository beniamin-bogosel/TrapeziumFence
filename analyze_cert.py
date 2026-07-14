#!/usr/bin/env python3
"""Summarize a fence_validate certificate.

The certificate is an exclusion certificate.  On a certified box the program has
proved that one of the six rigorous upper bounds for the normalized shortest
fence is at most theta, hence the true shortest fence is at most theta there.
Survivor boxes are unresolved: they are the current outer neighborhood in which
the value may still exceed theta.
"""
import argparse
import json
import math
import sys

TSTAR = (0.6417451566, 0.7071006812, 0.3582548434, 0.7071006812)


def mirror(pt):
    c1, c2, d1, d2 = pt
    return (1.0 - d1, d2, 1.0 - c1, c2)


TSTAR_M = mirror(TSTAR)


def box_volume(box):
    return math.prod(hi - lo for lo, hi in box)


def box_dist(box, ref):
    dmin2 = dmax2 = 0.0
    for (lo, hi), r in zip(box, ref):
        near = 0.0 if lo <= r <= hi else min(abs(r - lo), abs(r - hi))
        far = max(abs(r - lo), abs(r - hi))
        dmin2 += near * near
        dmax2 += far * far
    return math.sqrt(dmin2), math.sqrt(dmax2)


def certainly_admissible(box, eps=1e-12):
    (c1l, c1h), (c2l, c2h), (d1l, d1h), (d2l, d2h) = box
    if c2l <= eps or d2l <= eps:
        return False

    for c1 in (c1l, c1h):
        for c2 in (c2l, c2h):
            for d1 in (d1l, d1h):
                for d2 in (d2l, d2h):
                    bcx, bcy = c1 - 1.0, c2
                    cdx, cdy = d1 - c1, d2 - c2
                    if bcx * cdy - bcy * cdx <= eps:
                        return False
                    if c1 * d2 - d1 * c2 <= eps:
                        return False

    def sqmax(al, ah, ref):
        return max((al - ref) ** 2, (ah - ref) ** 2)

    if sqmax(c1l, c1h, 1.0) + sqmax(c2l, c2h, 0.0) > 1.0 - eps:
        return False
    if sqmax(d1l, d1h, 0.0) + sqmax(d2l, d2h, 0.0) > 1.0 - eps:
        return False
    cd_x = max((c1l - d1l) ** 2, (c1l - d1h) ** 2,
               (c1h - d1l) ** 2, (c1h - d1h) ** 2)
    cd_y = max((c2l - d2l) ** 2, (c2l - d2h) ** 2,
               (c2h - d2l) ** 2, (c2h - d2h) ** 2)
    return cd_x + cd_y <= 1.0 - eps


def folded_dist(box, no_mirror):
    d = box_dist(box, TSTAR)
    if no_mirror:
        return d
    dm = box_dist(box, TSTAR_M)
    return dm if dm[0] < d[0] else d


def bbox(boxes):
    if not boxes:
        return None
    return [(min(o["box"][k][0] for o in boxes),
             max(o["box"][k][1] for o in boxes)) for k in range(4)]


def load(path):
    with open(path) as fh:
        rows = [json.loads(line) for line in fh]
    return [o for o in rows if o.get("type") != "meta" and "status" in o]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("cert")
    ap.add_argument("--theta", type=float, default=None,
                    help="displayed threshold; verification uses fence_validate")
    ap.add_argument("--no-mirror", action="store_true",
                    help="do not fold distances by x -> 1-x, C <-> D")
    args = ap.parse_args()

    leaves = load(args.cert)
    surv = [o for o in leaves if o["status"] == "survivor"]
    cert = [o for o in leaves if o["status"] in ("certified", "low")]
    flat = [o for o in leaves if o["status"] == "flat_area"]
    disc = [o for o in leaves if o["status"] == "discarded"]
    nonopt = [o for o in leaves if o["status"] == "nonoptimal"]

    vol_total = sum(box_volume(o["box"]) for o in leaves)
    vol_cert = sum(box_volume(o["box"]) for o in cert)
    vol_flat = sum(box_volume(o["box"]) for o in flat)
    vol_surv = sum(box_volume(o["box"]) for o in surv)
    vol_disc = sum(box_volume(o["box"]) for o in disc)
    vol_nonopt = sum(box_volume(o["box"]) for o in nonopt)

    print(f"leaves: {len(leaves)}")
    print(f"  certified low boxes : {len(cert)}  volume {vol_cert:.12g}")
    print(f"  flat-area low boxes : {len(flat)}  volume {vol_flat:.12g}")
    print(f"  discarded boxes     : {len(disc)}  volume {vol_disc:.12g}")
    print(f"  nonoptimal boxes    : {len(nonopt)}  volume {vol_nonopt:.12g}")
    print(f"  unresolved survivors: {len(surv)}  volume {vol_surv:.12g}")
    print(f"  total tiled volume  : {vol_total:.12g}")
    if args.theta is not None:
        print(f"\nCertified boxes prove shortest fence / sqrt(area) <= {args.theta:g}.")
    print("Discarded boxes contain no normalized admissible quadrilateral.")
    if nonopt:
        print("Nonoptimal boxes fail the necessary equality of the two opposite-pair fences.")

    if not surv:
        print("\nNo survivors: every normalized admissible quadrilateral is either "
              "certified low, flat-area low, or certified nonoptimal.")
        return 0

    b = bbox(surv)
    names = ("c1", "c2", "d1", "d2")
    print("\nUnresolved survivor union:")
    for name, (lo, hi) in zip(names, b):
        print(f"  {name}: [{lo:.10g}, {hi:.10g}]")

    dists = [folded_dist(o["box"], args.no_mirror) for o in surv]
    near = min(d[0] for d in dists)
    far = max(d[1] for d in dists)
    print(f"  nearest distance to T*: {near:.8g}")
    print(f"  farthest box point from T*: {far:.8g}")

    core = [o for o in surv if certainly_admissible(o["box"])]
    shell = [o for o in surv if not certainly_admissible(o["box"])]
    if core:
        core_box = bbox(core)
        core_d = [folded_dist(o["box"], args.no_mirror) for o in core]
        print("\nCertainly-admissible survivor core:")
        print(f"  boxes: {len(core)}  volume {sum(box_volume(o['box']) for o in core):.12g}")
        for name, (lo, hi) in zip(names, core_box):
            print(f"  {name}: [{lo:.10g}, {hi:.10g}]")
        print(f"  farthest box point from T*: {max(d[1] for d in core_d):.8g}")
    print("\nBoundary-straddling survivors:")
    print(f"  boxes: {len(shell)}  volume {sum(box_volume(o['box']) for o in shell):.12g}")

    if nonopt:
        print("\nSound reading: outside the survivor union, every admissible normalized "
              "quadrilateral is either certified below theta or certified nonoptimal.")
    else:
        print("\nSound reading: outside the survivor union, every admissible normalized "
              "quadrilateral has a certified fence no longer than theta.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
