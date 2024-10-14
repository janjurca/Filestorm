import pandas as pd
import matplotlib.pyplot as plt
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--file', action="append", default=[], help='The file to read')
parser.add_argument('--title', action="append", default=[], help='The file to read')
parser.add_argument('--columns', action="append", default=["r/s","w/s", "%util"], help='cols to plot')
args = parser.parse_args()

fig, ax = plt.subplots(len(args.file), 1, figsize=(10, 6))

for i, (f,title) in enumerate(zip(args.file, args.title)):
    df = pd.read_csv(f, delim_whitespace=True)
    print(df)
    for column in args.columns:
        ax[i].plot(df[column], label=column)
        ax[i].set_title(title)
        ax[i].set_xlabel('Time')
        ax[i].set_ylabel('Value')
        ax[i].legend()
        ax[i].set_yscale('log')

# Show the plot
plt.show()