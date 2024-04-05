import pandas as pd
import numpy as np
import os
import json
import logging
import argparse
import matplotlib.pyplot as plt

log = logging.getLogger(__name__)


parser = argparse.ArgumentParser()
parser.add_argument("--file", type=str, required=True)
args = parser.parse_args()

with open(args.file) as fd:
    data = pd.read_json(fd)
    print(data)

create_file = data[data["action"] == "CREATE_FILE"]
create_file["duration_per_size"] = create_file["duration"] / create_file["size"]
# original duration is in nanoseconds
create_file["duration_seconds"] = create_file["duration"] / 1e9
create_file["speed_mbs"] = create_file["size"] / create_file["duration_seconds"] / 1024 / 1024
plt.plot(create_file["iteration"], create_file["speed_mbs"], 'ro', markersize=1)
plt.xlabel('Size')
plt.ylabel('Speed MB/s')
plt.title('Create file')
plt.show()