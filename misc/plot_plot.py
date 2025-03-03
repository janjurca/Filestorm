import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import math
import sys


def main():
    # Read the data from file (assumes one numeric value per line)
    with open("extents.txt", "r") as f:
        data = [float(line.strip()) for line in f if line.strip()]

    if not data:
        print("No data found!")
        return

    fig, ax = plt.subplots()
    (line,) = ax.plot([], [], "b", label="Data Points")  # Markers for visibility
    (curve_line,) = ax.plot([], [], "r-", label="Linear Fit")  # Red line for linear fit
    angle_text = ax.text(0.05, 0.95, "", transform=ax.transAxes, fontsize=12, verticalalignment="top")

    current_data = []

    def init():
        line.set_data([], [])
        curve_line.set_data([], [])
        angle_text.set_text("")
        return line, curve_line, angle_text

    def animate(i):
        step = 100
        current_data.append(sum(data[i * step : (i + 1) * step]) / step)
        n = len(current_data)
        current_max = max(current_data)

        if n > 1:
            x_vals = np.linspace(0, current_max, n)
        else:
            x_vals = np.array([0])
        y_vals = np.array(current_data)

        # Update the main data line
        line.set_data(x_vals, y_vals)

        # Fit a linear line if we have at least 2 points
        if n >= 2:
            coeffs = np.polyfit(x_vals, y_vals, int(sys.argv[1]))  # Fit linear polynomial
            poly_func = np.poly1d(coeffs)  # Create polynomial function

            # Generate smooth x values for plotting the curve
            smooth_x = np.linspace(0, current_max, 200)
            smooth_y = poly_func(smooth_x)

            curve_line.set_data(smooth_x, smooth_y)  # Update the curve line

            # Compute angle with respect to y-axis
            slope = coeffs[0]  # Slope of the fitted line
            angle_rad = math.atan(slope)  # Angle in radians
            angle_deg = math.degrees(angle_rad)  # Convert to degrees

            angle_text.set_text(f"Angle: {angle_deg:.2f}° slope: {round(slope, 3)}")

        ax.set_xlim(0, current_max)
        ax.set_ylim(min(current_data), max(current_data))
        return line, curve_line, angle_text

    ani = animation.FuncAnimation(fig, animate, frames=len(data) // 100, init_func=init, blit=False, interval=1, repeat=False)

    plt.title("Animated Value Progression with Linear Fit")
    # plt.legend()
    plt.show()


if __name__ == "__main__":
    main()
