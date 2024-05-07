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
args = parser.parse_args()

# Proložení křivek dat
def fit_curve(x, y, degree=10):
    coefficients = np.polyfit(x, y, degree)
    polynomial = np.poly1d(coefficients)
    return polynomial

colors = ['Reds', 'Blues', 'Greens', 'Purples', 'Oranges', 'Greys']

for i, file_path in enumerate(args.file):
    print(f">>>>>>>>>>>>>>>>> Processing {file_path}")
    with open(file_path) as fd:
        data = pd.read_json(fd)
    data = data[data["iteration"] <= args.iterations]
    create_file = data[data["action"] == "CREATE_FILE"]
    create_file["duration_per_size"] = create_file["duration"] / create_file["size"]
    # original duration is in nanoseconds
    create_file["duration_seconds"] = create_file["duration"] / 1e9
    create_file["speed_mbs"] = create_file["size"] / create_file["duration_seconds"] / 1024 / 1024

    marker_size = 10 #create_file["_file_extent_count"] 
    marker_size = create_file["size"] / 1024/1024/20  # Adjust the division factor as needed

    try:
        marker_color = create_file["file_extent_count"]
    except KeyError:
        marker_color = create_file["_file_extent_count"]

    plt.scatter(create_file["iteration"], create_file["speed_mbs"], s=marker_size, c=marker_color, cmap=colors[i], alpha=0.5, label=file_path)
    colorbar = plt.colorbar()
    colorbar.set_label('File Extent Count')

    # Fit curve
    polynomial = fit_curve(create_file["iteration"], create_file["speed_mbs"])
    plt.plot(create_file["iteration"], polynomial(create_file["iteration"]), label=f'{file_path} - Fitted Curve')

plt.xlabel('Time')
plt.ylabel('Speed MB/s')
plt.title('Create file - sync 64k write')
plt.legend()

## Add colorbar legend

plt.show()
