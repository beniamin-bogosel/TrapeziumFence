#!/usr/bin/env python3
"""Distance-to-T* summary for refinement survivor files.

For each certificate/refinement JSONL file, this script reads the unresolved
survivor leaves and computes simple Euclidean upper bounds on the distance to
the reference trapezoid T*.  The displayed rectangle bound uses the coordinate
bounding box of all survivors in that file:

    R_C = [c1_min,c1_max] x [c2_min,c2_max]
    R_D = [d1_min,d1_max] x [d2_min,d2_max].

The C and D bounds are the farthest-corner distances from C* and D* to those
rectangles.  The 4D bound is sqrt(C_bound^2 + D_bound^2).  This is exactly the
diagonal-style upper bound associated with the currently displayed survivor
rectangles.
"""
import argparse
import json
import math
import os
import sys

CSTAR = (0.6417451566, 0.7071006812)
DSTAR = (0.3582548434, 0.7071006812)

DEFAULT_FILES = [
    "flat_pair_cert_w0025.jsonl",
    "flat_pair_refine_w00125.jsonl",
    "flat_pair_refine_w000625.jsonl",
    "flat_pair_refine_w0003125.jsonl",
    "flat_pair_refine_w00015625.jsonl",
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
    c = farthest_corner_distance(box[:2], CSTAR)
    d = farthest_corner_distance(box[2:], DSTAR)
    return math.hypot(c, d)


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
            "c_bound": 0.0,
            "d_bound": 0.0,
            "bound4": 0.0,
            "leaf4": 0.0,
        }

    b = bbox(survivors)
    c_bound = farthest_corner_distance(b[:2], CSTAR)
    d_bound = farthest_corner_distance(b[2:], DSTAR)
    bound4 = math.hypot(c_bound, d_bound)
    leaf4 = max(box_distance_4d(box) for box in survivors)
    return {
        "path": path,
        "leaves": len(leaves),
        "survivors": len(survivors),
        "bbox": b,
        "c_bound": c_bound,
        "d_bound": d_bound,
        "bound4": bound4,
        "leaf4": leaf4,
    }


def print_table(rows):
    print("Distance-to-T* upper bounds from survivor rectangles")
    print(f"C* = ({CSTAR[0]:.10g}, {CSTAR[1]:.10g})")
    print(f"D* = ({DSTAR[0]:.10g}, {DSTAR[1]:.10g})")
    print()
    print("| file | survivors | C rectangle bound | D rectangle bound | 4D product bound | max leaf 4D bound |")
    print("| --- | ---: | ---: | ---: | ---: | ---: |")
    for row in rows:
        name = os.path.basename(row["path"])
        print(
            f"| {name} | {row['survivors']} | "
            f"{row['c_bound']:.9g} | {row['d_bound']:.9g} | "
            f"{row['bound4']:.9g} | {row['leaf4']:.9g} |"
        )
    print()
    for row in rows:
        if row["bbox"] is None:
            continue
        b = row["bbox"]
        print(os.path.basename(row["path"]))
        print(f"  R_C = {fmt_interval(b[0])} x {fmt_interval(b[1])}")
        print(f"  R_D = {fmt_interval(b[2])} x {fmt_interval(b[3])}")


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
