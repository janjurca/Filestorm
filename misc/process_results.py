import pandas as pd
import numpy as np
import os
import json
import logging
import argparse
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import seaborn as sns

log = logging.getLogger(__name__)

parser = argparse.ArgumentParser()
parser.add_argument("--file", nargs="+", required=True, help="List of JSON files")
parser.add_argument("--iterations", type=int, default=7000000, help="Number of iterations")
parser.add_argument("--operations", nargs="+", required=True, help="List of operations to visualize")
args = parser.parse_args()


def fit_curve(x, y, degree=10):
    coefficients = np.polyfit(x, y, degree)
    polynomial = np.poly1d(coefficients)
    return polynomial


color_palette = sns.color_palette("tab10", len(args.file))

fig, axes = plt.subplots(len(args.operations), 2, figsize=(15, 4 * len(args.operations)))

if len(args.operations) == 1:
    axes = [axes]

for i, file_path in enumerate(args.file):
    print(f">>>>>>>>>>>>>>>>> Processing {file_path}")
    with open(file_path) as fd:
        data = pd.read_json(fd)

    data = data[data["iteration"] <= args.iterations]

    for j, operation in enumerate(args.operations):
        print(f"================== Processing {operation}")
        operation_data = data[data["action"] == operation]
        operation_data["duration_per_size"] = operation_data["duration"] / operation_data["size"]
        operation_data["duration_seconds"] = operation_data["duration"] / 1e9
        operation_data["speed_mbs"] = operation_data["size"] / operation_data["duration_seconds"] / 1024 / 1024

        marker_size = operation_data["size"] / 1024 / 1024 / 20
        color = color_palette[i]

        try:
            marker_color = operation_data["file_extent_count"]
        except KeyError:
            marker_color = operation_data["_file_extent_count"]

        # Scatter plot
        scatter = axes[j][0].scatter(operation_data["iteration"], operation_data["speed_mbs"], s=marker_size, c=[color], alpha=0.5, label=f"{file_path} - {operation}")
        axes[j][0].set_xlabel("Iterations")
        axes[j][0].set_ylabel("Speed MB/s")
        axes[j][0].set_title(f"{operation} Speed Analysis")

        # Fit curve
        polynomial = fit_curve(operation_data["iteration"], operation_data["speed_mbs"])
        axes[j][0].plot(operation_data["iteration"], polynomial(operation_data["iteration"]), color=color)

        # Add legend only for scatter plots
        axes[j][0].legend(loc="upper right")

        # Boxplot for statistical analysis (separate series per file with matching color)
        box = axes[j][1].boxplot(operation_data["speed_mbs"].dropna(), vert=True, patch_artist=True, positions=[i], widths=0.6)
        for patch in box["boxes"]:
            patch.set_facecolor(color)

        axes[j][1].set_title(f"{operation} Speed Distribution")
        axes[j][1].set_ylabel("Speed MB/s")
        axes[j][1].set_xticks(range(len(args.file)))
        axes[j][1].set_xticklabels(args.file, rotation=45, ha="right")

plt.tight_layout()
plt.show()
