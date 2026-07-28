#!/usr/bin/env python3
"""Distance-to-T* summary for refinement survivor files.

For each certificate/refinement JSONL file, this script reads the unresolved
survivor leaves and computes simple Euclidean upper bounds on the distance to
the reference trapezoid T*.  The public notation is D=(0,0), C=(1,0), with
mobile vertices B=(b1,b2) and A=(a1,a2).  The JSONL files still store these
coordinates in implementation order (b1,b2,a1,a2).

The displayed rectangle bound uses the coordinate bounding box of all survivors
in that file:

    R_A = [a1_min,a1_max] x [a2_min,a2_max]
    R_B = [b1_min,b1_max] x [b2_min,b2_max].

The A and B bounds are the farthest-corner distances from A* and B* to those
rectangles.  The 4D bound is sqrt(A_bound^2 + B_bound^2).
"""
import argparse
import json
import math
import os
import sys

BSTAR = (0.6417451566, 0.7071006812)
ASTAR = (0.3582548434, 0.7071006812)

DEFAULT_FILES = [
    "flat_pair_cert_w0025.jsonl",
    "flat_pair_refine_w00125.jsonl",
    "flat_pair_refine_w000625.jsonl",
    "flat_pair_refine_w0003125.jsonl",
    "flat_pair_refine_w00015625.jsonl",
    "flat_pair_refine_w000078125.jsonl",
    "flat_pair_refine_w0000390625.jsonl",
]


def load_leaves(path):
    leaves = []
    with open(path) as fh:
        for line in fh:
            obj = json.loads(line)
            if obj.get("type") == "meta" or "status" not in obj:
                continue
            leaves.append(obj)
    return leaves


def bbox(boxes):
    if not boxes:
        return None
    return [(min(box[k][0] for box in boxes), max(box[k][1] for box in boxes))
            for k in range(4)]


def farthest_corner_distance(rect, point):
    (xl, xh), (yl, yh) = rect
    px, py = point
    dx = max(abs(xl - px), abs(xh - px))
    dy = max(abs(yl - py), abs(yh - py))
    return math.hypot(dx, dy)


def box_distance_4d(box):
    b = farthest_corner_distance(box[:2], BSTAR)
    a = farthest_corner_distance(box[2:], ASTAR)
    return math.hypot(a, b)


def fmt_interval(pair):
    return f"[{pair[0]:.10g}, {pair[1]:.10g}]"


def analyze(path):
    leaves = load_leaves(path)
    survivors = [leaf["box"] for leaf in leaves if leaf["status"] == "survivor"]
    if not survivors:
        return {
            "path": path,
            "leaves": len(leaves),
            "survivors": 0,
            "bbox": None,
            "a_bound": 0.0,
            "b_bound": 0.0,
            "bound4": 0.0,
            "leaf4": 0.0,
        }

    b = bbox(survivors)
    b_bound = farthest_corner_distance(b[:2], BSTAR)
    a_bound = farthest_corner_distance(b[2:], ASTAR)
    bound4 = math.hypot(a_bound, b_bound)
    leaf4 = max(box_distance_4d(box) for box in survivors)
    return {
        "path": path,
        "leaves": len(leaves),
        "survivors": len(survivors),
        "bbox": b,
        "a_bound": a_bound,
        "b_bound": b_bound,
        "bound4": bound4,
        "leaf4": leaf4,
    }


def print_table(rows):
    print("Distance-to-T* upper bounds from survivor rectangles")
    print(f"A* = ({ASTAR[0]:.10g}, {ASTAR[1]:.10g})")
    print(f"B* = ({BSTAR[0]:.10g}, {BSTAR[1]:.10g})")
    print()
    print("| file | survivors | A rectangle bound | B rectangle bound | 4D product bound | max leaf 4D bound |")
    print("| --- | ---: | ---: | ---: | ---: | ---: |")
    for row in rows:
        name = os.path.basename(row["path"])
        print(
            f"| {name} | {row['survivors']} | "
            f"{row['a_bound']:.9g} | {row['b_bound']:.9g} | "
            f"{row['bound4']:.9g} | {row['leaf4']:.9g} |"
        )
    print()
    for row in rows:
        if row["bbox"] is None:
            continue
        b = row["bbox"]
        print(os.path.basename(row["path"]))
        print(f"  R_A = {fmt_interval(b[2])} x {fmt_interval(b[3])}")
        print(f"  R_B = {fmt_interval(b[0])} x {fmt_interval(b[1])}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("certs", nargs="*",
                    help="certificate/refinement JSONL files; defaults to the recorded refinement chain")
    args = ap.parse_args()

    paths = args.certs or DEFAULT_FILES
    missing = [path for path in paths if not os.path.exists(path)]
    if missing:
        for path in missing:
            print(f"missing: {path}", file=sys.stderr)
        return 1

    print_table([analyze(path) for path in paths])
    return 0


if __name__ == "__main__":
    sys.exit(main())
