#ifndef POLYCURVE_H
#define POLYCURVE_H

#include <Eigen/Dense>
#include <cmath>
#include <iostream>
#include <numeric>  // for std::reduce (C++17) or std::accumulate
#include <stdexcept>
#include <vector>

class PolyCurve {
public:
  // Constructor: specify the polynomial degree.
  // The second parameter is the subsampling count (default 1 means no subsampling).
  explicit PolyCurve(int degree, int subsampling = 1);

  // Add a data point (x, y). When subsampling > 1 the point is buffered and only every
  // "subsampling"-th point (averaged over the last 'subsampling' calls) is added.
  void addPoint(float y);

  // Remove the oldest data point (from the front).
  void popOldestPoint();

  // Remove the specified number of oldest data points.
  // If count is greater than the number of points, all points will be removed.
  void popOldestPoints(size_t count);

  // Clear all data points.
  void clearPoints();

  // Fit the polynomial to the current data points using least squares.
  // Throws an exception if there are not enough points.
  void fitPolyCurve();

  // Evaluate the fitted polynomial at a given x.
  float evaluate(float x);

  // Evaluate the first derivative f'(x) at a given x.
  float derivative(float x);

  // Compute and return the tangent angle (in degrees) relative to the y-axis at a given x.
  float getTangentAngle(float x);

  // Return the polynomial coefficients (ensures the curve is fitted).
  const Eigen::VectorXd& getCoefficients() const;

  // Print all stored data points.
  void printPoints() const;

  // Return the number of data points currently stored.
  size_t getPointCount() const;

  float slope() const;

  float slopeAngle() const;

private:
  // Main data storage for points that will be used for fitting.
  std::vector<float> y_points;

  // Subsampling buffers.
  // (Since we want to average both x and y values, we buffer x too.)
  std::vector<float> y_subsample_buffer;

  int poly_degree;         // Degree of the polynomial to be fitted.
  Eigen::VectorXd coeffs;  // Polynomial coefficients (from constant term to highest degree).
  bool fitted;             // Flag indicating whether the current coefficients are up-to-date.
  int subsampling;         // How many points to average before adding a new point.

  float maximum_value = 0;
  // Helper function to compute the average of the values in a vector.
  float average(std::vector<float> const& v) {
    if (v.empty()) {
      return 0;
    }
    auto const count = static_cast<float>(v.size());
    // Using std::reduce (available in C++17); you could also use std::accumulate.
    return std::reduce(v.begin(), v.end()) / count;
  }
};

#endif  // POLYCURVE_H
