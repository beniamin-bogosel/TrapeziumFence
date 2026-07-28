#!/usr/bin/env python3
"""Render survivor-rectangle PNG figures in the public D,C,B,A notation."""
from pathlib import Path

import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle


# Public notation: D=(0,0), C=(1,0), B is the right mobile vertex and A is the
# left mobile vertex.  Internally, B is stored in the old C-slots and A in the
# old D-slots.
B_RECT = ((0.6337890625, 0.6499023438), (0.6921386719, 0.7219238281))
A_RECT = ((0.349609375, 0.3662109375), (0.6921386719, 0.7219238281))
BSTAR = (0.6417451566, 0.7071006812)
ASTAR = (0.3582548434, 0.7071006812)
FIG_DIR = Path(__file__).resolve().parent / "figures"
RECT_OUT = FIG_DIR / "current_survivor_rectangles.png"
OVERVIEW_OUT = FIG_DIR / "survivor_rectangles_overview.png"


def add_survivor_panel(ax, rect, star, title, xvar, yvar):
    (xlo, xhi), (ylo, yhi) = rect
    width = xhi - xlo
    height = yhi - ylo

    ax.add_patch(
        Rectangle(
            (xlo, ylo),
            width,
            height,
            facecolor="#8ecae6",
            edgecolor="#0b5f79",
            linewidth=2.2,
            alpha=0.34,
        )
    )
    ax.scatter([star[0]], [star[1]], marker="*", s=170, color="#b3261e",
               edgecolor="#5c0f0a", linewidth=0.8, zorder=3)
    ax.annotate(
        title,
        xy=star,
        xytext=(7, 8),
        textcoords="offset points",
        fontsize=16,
        color="#5c0f0a",
    )

    pad = 0.035
    ax.set_xlim(min(xlo, star[0]) - pad, max(xhi, star[0]) + pad)
    ax.set_ylim(min(ylo, star[1]) - pad, max(yhi, star[1]) + pad)
    ax.set_aspect("equal", adjustable="box")
    ax.set_xlabel(rf"${xvar}$", fontsize=20)
    ax.set_ylabel(rf"${yvar}$", fontsize=20)
    ax.tick_params(axis="both", labelsize=12)
    ax.grid(True, color="#d8dee3", linewidth=0.8)

    text = (
        rf"${xvar}\in[{xlo:.9f},\,{xhi:.9f}]$" + "\n" +
        rf"${yvar}\in[{ylo:.9f},\,{yhi:.9f}]$"
    )
    ax.text(
        0.5,
        -0.088,
        text,
        transform=ax.transAxes,
        ha="center",
        va="top",
        fontsize=17,
        bbox={
            "boxstyle": "round,pad=0.28",
            "facecolor": "white",
            "edgecolor": "#8a97a3",
            "linewidth": 0.9,
        },
    )


def add_zoom_panel(ax, rect, star, panel_title, point_label, xvar, yvar):
    add_survivor_panel(ax, rect, star, point_label, xvar, yvar)
    ax.set_title(panel_title, fontsize=17, pad=5)
    ax.texts[-1].set_fontsize(13)


def add_overview():
    fig = plt.figure(figsize=(15, 9.5), dpi=100)
    gs = fig.add_gridspec(2, 2, height_ratios=[1.45, 1.0], hspace=0.34, wspace=0.18)
    ax = fig.add_subplot(gs[0, :])

    d = (0.0, 0.0)
    c = (1.0, 0.0)
    poly_x = [d[0], c[0], BSTAR[0], ASTAR[0], d[0]]
    poly_y = [d[1], c[1], BSTAR[1], ASTAR[1], d[1]]
    ax.plot(poly_x, poly_y, color="#1f2933", linewidth=2.2)
    ax.fill(poly_x, poly_y, color="#eef6f9", alpha=0.85)

    for label, point, color in (
        (r"$D=(0,0)$", d, "#1f2933"),
        (r"$C=(1,0)$", c, "#1f2933"),
        (r"$A^\ast$", ASTAR, "#b3261e"),
        (r"$B^\ast$", BSTAR, "#b3261e"),
    ):
        marker = "*" if "ast" in label else "o"
        size = 170 if marker == "*" else 42
        ax.scatter([point[0]], [point[1]], marker=marker, s=size, color=color,
                   edgecolor="#5c0f0a" if marker == "*" else color, zorder=4)
        offset = (-38, 9) if point[0] < 0.5 else (9, 9)
        if point[1] == 0:
            offset = (-42, 9) if point[0] < 0.5 else (8, 9)
        ax.annotate(label, xy=point, xytext=offset, textcoords="offset points",
                    fontsize=16, color=color)

    for rect, edge, label in ((A_RECT, "#0b5f79", r"$R_A$"),
                              (B_RECT, "#0b5f79", r"$R_B$")):
        (xlo, xhi), (ylo, yhi) = rect
        ax.add_patch(Rectangle((xlo, ylo), xhi - xlo, yhi - ylo,
                               facecolor="#8ecae6", edgecolor=edge,
                               linewidth=2.0, alpha=0.45, zorder=3))
        ax.annotate(label, xy=((xlo + xhi) / 2, yhi),
                    xytext=(0, 8), textcoords="offset points",
                    ha="center", fontsize=16, color=edge)

    ax.set_aspect("equal", adjustable="box")
    ax.set_xlim(-0.08, 1.08)
    ax.set_ylim(-0.07, 0.84)
    ax.set_xlabel(r"$x$", fontsize=18)
    ax.set_ylabel(r"$y$", fontsize=18)
    ax.tick_params(axis="both", labelsize=12)
    ax.grid(True, color="#d8dee3", linewidth=0.8)
    ax.set_title(r"Survivor rectangles inside the conjectured trapezium neighborhood",
                 fontsize=22, pad=12)

    left = fig.add_subplot(gs[1, 0])
    right = fig.add_subplot(gs[1, 1])
    add_zoom_panel(left, A_RECT, ASTAR, r"$R_A$ near $A^\ast$", r"$A^\ast$", r"a_1", r"a_2")
    add_zoom_panel(right, B_RECT, BSTAR, r"$R_B$ near $B^\ast$", r"$B^\ast$", r"b_1", r"b_2")

    fig.subplots_adjust(left=0.06, right=0.985, top=0.94, bottom=0.09)
    fig.savefig(OVERVIEW_OUT, dpi=100)
    plt.close(fig)


def main():
    plt.rcParams.update({
        "font.family": "serif",
        "font.serif": ["Computer Modern Roman", "DejaVu Serif"],
        "mathtext.fontset": "cm",
        "axes.unicode_minus": False,
    })

    fig, axes = plt.subplots(1, 2, figsize=(15, 9.5), dpi=100)
    add_survivor_panel(axes[0], A_RECT, ASTAR, r"$A^\ast$", r"a_1", r"a_2")
    add_survivor_panel(axes[1], B_RECT, BSTAR, r"$B^\ast$", r"b_1", r"b_2")

    fig.suptitle(r"Current survivor rectangles near $T^\ast$", fontsize=22, y=0.965)
    fig.subplots_adjust(left=0.07, right=0.985, top=0.9, bottom=0.125, wspace=0.2)
    FIG_DIR.mkdir(parents=True, exist_ok=True)
    fig.savefig(RECT_OUT, dpi=100)
    plt.close(fig)
    add_overview()


if __name__ == "__main__":
    main()
