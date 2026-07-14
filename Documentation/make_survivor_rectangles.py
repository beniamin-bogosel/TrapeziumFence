#!/usr/bin/env python3
"""Render the current survivor rectangles as a PNG figure."""
from pathlib import Path

import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle


C_RECT = ((0.626953125, 0.669921875), (0.677734375, 0.7353515625))
D_RECT = ((0.330078125, 0.373046875), (0.677734375, 0.7353515625))
CSTAR = (0.6417451566, 0.7071006812)
DSTAR = (0.3582548434, 0.7071006812)
OUT = Path(__file__).resolve().parent / "figures" / "current_survivor_rectangles.png"


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


def main():
    plt.rcParams.update({
        "font.family": "serif",
        "font.serif": ["Computer Modern Roman", "DejaVu Serif"],
        "mathtext.fontset": "cm",
        "axes.unicode_minus": False,
    })

    fig, axes = plt.subplots(1, 2, figsize=(15, 9.5), dpi=100)
    add_survivor_panel(axes[0], C_RECT, CSTAR, r"$C^\ast$", r"c_1", r"c_2")
    add_survivor_panel(axes[1], D_RECT, DSTAR, r"$D^\ast$", r"d_1", r"d_2")

    fig.suptitle(r"Current survivor rectangles near $T^\ast$", fontsize=22, y=0.965)
    fig.subplots_adjust(left=0.07, right=0.985, top=0.9, bottom=0.125, wspace=0.2)
    OUT.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(OUT, dpi=100)
    plt.close(fig)


if __name__ == "__main__":
    main()
