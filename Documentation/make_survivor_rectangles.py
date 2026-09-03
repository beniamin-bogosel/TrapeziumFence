#!/usr/bin/env python3
"""Render survivor-rectangle PNG figures in the public D,C,B,A notation."""
from pathlib import Path

import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle


# Public notation: D=(0,0), C=(1,0), B is the right mobile vertex and A is the
# left mobile vertex.  Internally, B is stored in the old C-slots and A in the
# old D-slots.
B_RECT = ((0.6337890625, 0.64990234375),
          (0.692138671875, 0.721923828125))
A_RECT = ((0.349609375, 0.3662109375),
          (0.692138671875, 0.721923828125))
BSTAR = (0.6417451566, 0.7071006812)
ASTAR = (0.3582548434, 0.7071006812)
FIG_DIR = Path(__file__).resolve().parent / "figures"
RECT_OUT = FIG_DIR / "current_survivor_rectangles.png"
OVERVIEW_OUT = FIG_DIR / "survivor_rectangles_overview.png"
THREE_PANEL_OUT = FIG_DIR / "survivor_rectangles_three_panel.png"


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
        rf"${xvar}\in[{xlo:.12f},\,{xhi:.12f}]$" + "\n" +
        rf"${yvar}\in[{ylo:.12f},\,{yhi:.12f}]$"
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


def add_compact_zoom_panel(ax, rect, star, panel_title, point_label, xvar, yvar):
    add_survivor_panel(ax, rect, star, point_label, xvar, yvar)
    ax.set_title(panel_title, fontsize=11, pad=4)
    ax.set_xlabel(rf"${xvar}$", fontsize=10, labelpad=2)
    ax.set_ylabel(rf"${yvar}$", fontsize=10, labelpad=2)
    ax.tick_params(axis="both", labelsize=8.5, pad=1.5)
    ax.texts[-1].set_position((0.5, -0.27))
    ax.texts[-1].set_fontsize(10)
    for text in ax.texts[:-1]:
        text.set_fontsize(10)


def add_trapezium_panel(ax, show_coordinate_labels, publication=False):
    label_size = 10 if publication else 16
    axis_label_size = 10 if publication else 18
    tick_size = 8.5 if publication else 11
    star_size = 64 if publication else 170
    point_size = 18 if publication else 42
    line_width = 1.25 if publication else 2.2
    rect_width = 1.0 if publication else 2.0

    d = (0.0, 0.0)
    c = (1.0, 0.0)
    poly_x = [d[0], c[0], BSTAR[0], ASTAR[0], d[0]]
    poly_y = [d[1], c[1], BSTAR[1], ASTAR[1], d[1]]
    ax.plot(poly_x, poly_y, color="#1f2933", linewidth=line_width)
    ax.fill(poly_x, poly_y, color="#eef6f9", alpha=0.85)

    fixed_labels = (
        (r"$D=(0,0)$" if show_coordinate_labels else r"$D$", d),
        (r"$C=(1,0)$" if show_coordinate_labels else r"$C$", c),
    )
    for label, point in fixed_labels:
        ax.scatter([point[0]], [point[1]], marker="o", s=point_size, color="#1f2933", zorder=4)
        offset = (0, 9) if publication else ((-24, 9) if point[0] < 0.5 else (8, 9))
        ax.annotate(label, xy=point, xytext=offset, textcoords="offset points",
                    ha="center" if publication else "left",
                    fontsize=label_size, color="#1f2933")

    for label, point in ((r"$A^\ast$", ASTAR), (r"$B^\ast$", BSTAR)):
        ax.scatter([point[0]], [point[1]], marker="*", s=star_size, color="#b3261e",
                   edgecolor="#5c0f0a", linewidth=0.8, zorder=4)
        offset = (-24, 6) if point[0] < 0.5 else (7, 6)
        ax.annotate(label, xy=point, xytext=offset, textcoords="offset points",
                    fontsize=label_size, color="#b3261e")

    for rect, edge, label in ((A_RECT, "#0b5f79", r"$\mathcal{R}_A$"),
                              (B_RECT, "#0b5f79", r"$\mathcal{R}_B$")):
        (xlo, xhi), (ylo, yhi) = rect
        ax.add_patch(Rectangle((xlo, ylo), xhi - xlo, yhi - ylo,
                               facecolor="#8ecae6", edgecolor=edge,
                               linewidth=rect_width, alpha=0.45, zorder=3))
        ax.annotate(label, xy=((xlo + xhi) / 2, yhi),
                    xytext=(0, 6 if publication else 8), textcoords="offset points",
                    ha="center", fontsize=label_size, color=edge)

    ax.set_aspect("equal", adjustable="box")
    ax.set_xlim(-0.08, 1.08)
    ax.set_ylim(-0.07, 0.88 if publication else 0.84)
    ax.set_xlabel(r"$x$", fontsize=axis_label_size, labelpad=2)
    ax.set_ylabel(r"$y$", fontsize=axis_label_size, labelpad=2)
    ax.tick_params(axis="both", labelsize=tick_size, pad=1.5)
    ax.grid(True, color="#d8dee3", linewidth=0.45 if publication else 0.8)


def add_overview():
    fig = plt.figure(figsize=(15, 9.5), dpi=100)
    gs = fig.add_gridspec(2, 2, height_ratios=[1.45, 1.0], hspace=0.34, wspace=0.18)
    ax = fig.add_subplot(gs[0, :])
    add_trapezium_panel(ax, show_coordinate_labels=True)
    ax.set_title(r"Survivor rectangles for the Proposition 17 equality class",
                 fontsize=22, pad=12)

    left = fig.add_subplot(gs[1, 0])
    right = fig.add_subplot(gs[1, 1])
    add_zoom_panel(left, A_RECT, ASTAR, r"$\mathcal{R}_A$ near $A^\ast$", r"$A^\ast$", r"a_1", r"a_2")
    add_zoom_panel(right, B_RECT, BSTAR, r"$\mathcal{R}_B$ near $B^\ast$", r"$B^\ast$", r"b_1", r"b_2")

    fig.subplots_adjust(left=0.06, right=0.985, top=0.94, bottom=0.09)
    fig.savefig(OVERVIEW_OUT, dpi=100)
    plt.close(fig)


def add_three_panel():
    fig = plt.figure(figsize=(7.2, 2.92), dpi=300)
    gs = fig.add_gridspec(1, 3, width_ratios=[1.08, 1.32, 1.08], wspace=0.12)
    left = fig.add_subplot(gs[0, 0])
    middle = fig.add_subplot(gs[0, 1])
    right = fig.add_subplot(gs[0, 2])

    add_compact_zoom_panel(left, A_RECT, ASTAR, r"$\mathcal{R}_A$ near $A^\ast$",
                           r"$A^\ast$", r"a_1", r"a_2")
    add_trapezium_panel(middle, show_coordinate_labels=False, publication=True)
    add_compact_zoom_panel(right, B_RECT, BSTAR, r"$\mathcal{R}_B$ near $B^\ast$",
                           r"$B^\ast$", r"b_1", r"b_2")

    fig.suptitle(r"Survivors in the Proposition 17 equality class near $T^\ast$",
                 fontsize=12.5, y=0.96)
    fig.subplots_adjust(left=0.045, right=0.992, top=0.80, bottom=0.29)
    fig.savefig(THREE_PANEL_OUT, dpi=300)
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

    fig.suptitle(r"Survivor rectangles for Proposition 17 near $T^\ast$",
                 fontsize=22, y=0.965)
    fig.subplots_adjust(left=0.07, right=0.985, top=0.9, bottom=0.125, wspace=0.2)
    FIG_DIR.mkdir(parents=True, exist_ok=True)
    fig.savefig(RECT_OUT, dpi=100)
    plt.close(fig)
    add_overview()
    add_three_panel()


if __name__ == "__main__":
    main()
