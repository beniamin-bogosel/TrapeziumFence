#!/usr/bin/env python3
"""Summarize a fence_validate certificate.

The certificate is an exclusion certificate.  On a certified box the program has
proved that one of the six rigorous upper bounds for the normalized shortest
fence is at most theta, hence the true shortest fence is at most theta there.
Survivor boxes are unresolved: they are the current outer neighborhood in which
the value may still exceed theta.  Pair-incompatible leaves fail the
opposite-pair equality hypothesis of Proposition 17; that is a conditional
exclusion, not a global nonoptimality claim.
"""
import argparse
import json
import math
import sys
from fractions import Fraction

# Public notation: D=(0,0), C=(1,0), B=(b1,b2), A=(a1,a2).
# The JSONL coordinate order is still the implementation order
# (b1,b2,a1,a2).
TSTAR = (0.6417451566, 0.7071006812, 0.3582548434, 0.7071006812)


def mirror(pt):
    c1, c2, d1, d2 = pt
    return (1.0 - d1, d2, 1.0 - c1, c2)


TSTAR_M = mirror(TSTAR)


def box_volume_exact(box):
    """Exact volume of the binary64 endpoint box used by the C verifier."""
    volume = Fraction(1)
    for lo, hi in box:
        volume *= Fraction.from_float(hi) - Fraction.from_float(lo)
    return volume


def volume_sum(rows):
    return sum((box_volume_exact(row["box"]) for row in rows), Fraction(0))


def box_dist(box, ref):
    dmin2 = dmax2 = 0.0
    for (lo, hi), r in zip(box, ref):
        near = 0.0 if lo <= r <= hi else min(abs(r - lo), abs(r - hi))
        far = max(abs(r - lo), abs(r - hi))
        dmin2 += near * near
        dmax2 += far * far
    return math.sqrt(dmin2), math.sqrt(dmax2)


def certainly_admissible(box):
    """Prove the box lies in the admissible region using exact rationals.

    JSON endpoints are read as binary64, exactly as in the C verifier.  The
    relevant cross products are multiaffine, so their extrema occur at box
    corners; squared-distance maxima are attained at coordinate endpoints.
    """
    fbox = [[Fraction.from_float(x) for x in interval] for interval in box]
    (c1l, c1h), (c2l, c2h), (d1l, d1h), (d2l, d2h) = fbox
    if c2l <= 0 or d2l <= 0:
        return False

    for c1 in (c1l, c1h):
        for c2 in (c2l, c2h):
            for d1 in (d1l, d1h):
                for d2 in (d2l, d2h):
                    bcx, bcy = c1 - 1, c2
                    cdx, cdy = d1 - c1, d2 - c2
                    if bcx * cdy - bcy * cdx <= 0:
                        return False
                    if c1 * d2 - d1 * c2 <= 0:
                        return False

    def sqmax(al, ah, ref):
        return max((al - ref) ** 2, (ah - ref) ** 2)

    if sqmax(c1l, c1h, 1) + sqmax(c2l, c2h, 0) > 1:
        return False
    if sqmax(d1l, d1h, 0) + sqmax(d2l, d2h, 0) > 1:
        return False
    cd_x = max((c1l - d1l) ** 2, (c1l - d1h) ** 2,
               (c1h - d1l) ** 2, (c1h - d1h) ** 2)
    cd_y = max((c2l - d2l) ** 2, (c2l - d2h) ** 2,
               (c2h - d2l) ** 2, (c2h - d2h) ** 2)
    return cd_x + cd_y <= 1


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
    metadata = next((o for o in rows if o.get("type") == "meta"), {})
    leaves = [o for o in rows if o.get("type") != "meta" and "status" in o]
    return metadata, leaves


def print_public_rectangles(box):
    # Certificate endpoints are binary64 dyadics.  Seventeen significant
    # digits round-trip them exactly; shorter formatting can round a lower
    # endpoint upward or an upper endpoint downward.
    print(f"  A: [{box[2][0]:.17g}, {box[2][1]:.17g}] x "
          f"[{box[3][0]:.17g}, {box[3][1]:.17g}]")
    print(f"  B: [{box[0][0]:.17g}, {box[0][1]:.17g}] x "
          f"[{box[1][0]:.17g}, {box[1][1]:.17g}]")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("cert")
    ap.add_argument("--theta", type=float, default=None,
                    help="displayed threshold; verification uses fence_validate")
    ap.add_argument("--no-mirror", action="store_true",
                    help="do not fold distances by x -> 1-x, C <-> D")
    args = ap.parse_args()

    metadata, leaves = load(args.cert)
    surv = [o for o in leaves if o["status"] == "survivor"]
    cert = [o for o in leaves if o["status"] in ("certified", "low")]
    flat = [o for o in leaves if o["status"] == "flat_area"]
    disc = [o for o in leaves if o["status"] == "discarded"]
    # schema-3 files used "nonoptimal".  New files use the semantically
    # accurate name "pair_incompatible"; accept both during migration.
    pair_incompat = [o for o in leaves
                     if o["status"] in ("pair_incompatible", "nonoptimal")]

    vol_total = volume_sum(leaves)
    vol_cert = volume_sum(cert)
    vol_flat = volume_sum(flat)
    vol_surv = volume_sum(surv)
    vol_disc = volume_sum(disc)
    vol_pair_incompat = volume_sum(pair_incompat)

    print(f"leaves: {len(leaves)}")
    print(f"  certified low boxes : {len(cert)}  volume ~{float(vol_cert):.12g}")
    print(f"  flat-area low boxes : {len(flat)}  volume ~{float(vol_flat):.12g}")
    print(f"  discarded boxes     : {len(disc)}  volume ~{float(vol_disc):.12g}")
    print(f"  pair-incompatible   : {len(pair_incompat)}  volume ~{float(vol_pair_incompat):.12g}")
    print(f"  unresolved survivors: {len(surv)}  volume ~{float(vol_surv):.12g}")
    print(f"  exact survivor volume: {vol_surv}")
    print(f"  exact total tiled volume: {vol_total}")
    if args.theta is not None:
        print(f"\nCertified boxes prove shortest fence / sqrt(area) <= {args.theta:g}.")
    if metadata.get("half", False):
        print("Discarded boxes are inadmissible or outside the selected symmetry half-domain.")
    else:
        print("Discarded boxes contain no normalized admissible quadrilateral.")
    if pair_incompat:
        print("Pair-incompatible boxes fail the opposite-pair equality hypothesis "
              "of Proposition 17.")

    if not surv:
        print("\nNo survivors: every normalized admissible quadrilateral is either "
              "certified low, flat-area low, or pair-incompatible.")
        if pair_incompat:
            print("Consequently, within the Proposition 17 equality class (including "
                  "the active-pair analytic cases), no above-threshold candidate remains.")
        return 0

    b = bbox(surv)
    print("\nUnresolved survivor union:")
    print_public_rectangles(b)

    print("  (Distances below are ordinary floating-point diagnostics, not "
          "validated certificate bounds.)")
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
        core_volume = volume_sum(core)
        print(f"  boxes: {len(core)}  volume ~{float(core_volume):.12g} "
              f"(exact {core_volume})")
        print_public_rectangles(core_box)
        print(f"  farthest box point from T*: {max(d[1] for d in core_d):.8g}")
    print("\nBoundary-straddling survivors:")
    shell_volume = volume_sum(shell)
    print(f"  boxes: {len(shell)}  volume ~{float(shell_volume):.12g} "
          f"(exact {shell_volume})")

    if pair_incompat:
        print("\nSound reading: outside the survivor union, every admissible normalized "
              "quadrilateral is either certified below theta or fails the opposite-pair "
              "equality hypothesis.  Thus the exclusion of above-threshold candidates "
              "is conditional on Proposition 17 or the active-pair analytic reduction.")
    else:
        print("\nSound reading: outside the survivor union, every admissible normalized "
              "quadrilateral has a certified fence no longer than theta.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
