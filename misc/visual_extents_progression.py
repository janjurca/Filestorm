import pandas as pd
import numpy as np
import os
import json
import logging
import argparse

import matplotlib.pyplot as plt

log = logging.getLogger(__name__)

parser = argparse.ArgumentParser()
parser.add_argument("--file", nargs="+", required=True, help="List of JSON files")
args = parser.parse_args()


# Proložení křivek dat
def fit_curve(x, y, degree=10):
    coefficients = np.polyfit(x, y, degree)
    polynomial = np.poly1d(coefficients)
    return polynomial


colors = ["Reds", "Blues", "Greens", "Purples", "Oranges", "Greys"]

fig, axes = plt.subplots(1, 1, figsize=(10, 4))

for i, file_path in enumerate(args.file):
    print(f">>>>>>>>>>>>>>>>> Processing {file_path}")
    with open(file_path) as fd:
        data = pd.read_json(fd)

    data["smoothed"] = data["total_extents_count"].rolling(window=10000, min_periods=1).median()

    print(data)
    axes.scatter(data["iteration"], data["total_extents_count"], s=1, c="blue", cmap=colors[i], alpha=0.5, label=f"{file_path}")
    axes.scatter(data["iteration"], data["smoothed"], s=1, c="red", cmap=colors[i], alpha=0.5, label=f"{file_path}")

    axes.set_xlabel("Iterations")
    axes.set_ylabel("extent count")
    # axes[j].legend()
    # Add color bar

    # Fit curve for each operation
    polynomial = fit_curve(data["iteration"], data["total_extents_count"])
    axes.plot(data["iteration"], polynomial(data["iteration"]), label=f"{file_path} - Fitted Curve")

plt.tight_layout()
plt.show()
