import matplotlib.pyplot as plt
import numpy as np

# Load data from file
filename = "data.angles"  # Change this to your actual file name
data = np.loadtxt(filename, delimiter=",")

# Extract columns
x = data[:, 0]
y = data[:, 1]
y1 = data[:, 2]

# Compute ratios
# Ratio for subplot 2: x/y (make sure y != 0)
ratio_x_over_y = x / y
# Ratio for subplot 3: y divided by (x/y) which is y^2/x (make sure x != 0)
ratio_y_div_ratio = y / ratio_x_over_y

# Create a figure with 3 subplots (vertically arranged)
fig, axs = plt.subplots(3, 1, figsize=(10, 15))

# -------------------
# Subplot 1: y and y1
# -------------------
ax1 = axs[0]
ax1.plot(x, y, "b-", label="y")
ax1.set_xlabel("x")
ax1.set_ylabel("y", color="b")
ax1.tick_params(axis="y", labelcolor="b")
ax1.set_title("Plot 1: y and y1")

# Create a secondary y-axis for y1
ax1_twin = ax1.twinx()
ax1_twin.plot(x, y1, "r-", label="y1")
ax1_twin.set_ylabel("y1", color="r")
ax1_twin.tick_params(axis="y", labelcolor="r")

# --------------------
# Subplot 2: y and x/y
# --------------------
ax2 = axs[1]
ax2.plot(x, y, "b-", label="y")
ax2.set_xlabel("x")
ax2.set_ylabel("y", color="b")
ax2.tick_params(axis="y", labelcolor="b")
ax2.set_title("Plot 2: y and x/y")

# Create a secondary y-axis for x/y
ax2_twin = ax2.twinx()
ax2_twin.plot(x, ratio_x_over_y, "g-", label="x/y")
ax2_twin.set_ylabel("x/y", color="g")
ax2_twin.tick_params(axis="y", labelcolor="g")

# -----------------------------------------
# Subplot 3: y and (y divided by (x/y))
# -----------------------------------------
ax3 = axs[2]
ax3.plot(x, y, "b-", label="y")
ax3.set_xlabel("x")
ax3.set_ylabel("y", color="b")
ax3.tick_params(axis="y", labelcolor="b")
ax3.set_title("Plot 3: y and y/(x/y)")

# Create a secondary y-axis for y/(x/y) (i.e. y^2/x)
ax3_twin = ax3.twinx()
ax3_twin.plot(x, ratio_y_div_ratio, "m-", label="y/(x/y)")
ax3_twin.set_ylabel("y/(x/y)", color="m")
ax3_twin.tick_params(axis="y", labelcolor="m")

# Adjust layout for clarity
plt.tight_layout()
plt.show()
