import pandas as pd
import numpy as np
import os
import json
import logging
import argparse

import matplotlib.pyplot as plt

log = logging.getLogger(__name__)

parser = argparse.ArgumentParser()
parser.add_argument("--file", nargs='+', required=True, help="List of JSON files")
parser.add_argument("--iterations", type=int, default=70000, help="Number of iterations")
parser.add_argument("--operations", nargs='+', required=True, help="List of operations to visualize")
args = parser.parse_args()

# Proložení křivek dat
def fit_curve(x, y, degree=10):
    coefficients = np.polyfit(x, y, degree)
    polynomial = np.poly1d(coefficients)
    return polynomial

colors = ['Reds', 'Blues', 'Greens', 'Purples', 'Oranges', 'Greys']

fig, axes = plt.subplots(len(args.operations), 1, figsize=(10, 6 * len(args.operations)))

for i, file_path in enumerate(args.file):
    print(f">>>>>>>>>>>>>>>>> Processing {file_path}")
    with open(file_path) as fd:
        data = pd.read_json(fd)
    
    data = data[data["iteration"] <= args.iterations]

    for j, operation in enumerate(args.operations):
        operation_data = data[data["action"] == operation]
        operation_data["duration_per_size"] = operation_data["duration"] / operation_data["size"]
        operation_data["duration_seconds"] = operation_data["duration"] / 1e9
        operation_data["speed_mbs"] = operation_data["size"] / operation_data["duration_seconds"] / 1024 / 1024

        marker_size = operation_data["size"] / 1024 / 1024 / 20  # Adjust the division factor as needed

        try:
            marker_color = operation_data["file_extent_count"]
        except KeyError:
            marker_color = operation_data["_file_extent_count"]

        axes[j].scatter(operation_data["iteration"], operation_data["speed_mbs"], s=marker_size, c=marker_color, cmap=colors[i], alpha=0.5, label=f"{file_path} - {operation}")
        axes[j].set_xlabel('Time')
        axes[j].set_ylabel('Speed MB/s')
        axes[j].set_title(f'{operation} Speed Analysis')
        axes[j].legend()

        # Fit curve for each operation
        polynomial = fit_curve(operation_data["iteration"], operation_data["speed_mbs"])
        axes[j].plot(operation_data["iteration"], polynomial(operation_data["iteration"]), label=f'{file_path} - {operation} Fitted Curve')

plt.tight_layout()
plt.show()
