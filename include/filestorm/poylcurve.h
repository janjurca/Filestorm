#ifndef POLYCURVE_H
#define POLYCURVE_H

#include <Eigen/Dense>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

class PolyCurve {
public:
  /// \brief Construct a Curve object with a specified polynomial degree.
  /// \param degree The degree of the polynomial to fit.
  PolyCurve(int degree);

  /// \brief Add a data point (x, y) to the PolyCurve.
  /// \param x The x-coordinate.
  /// \param y The y-coordinate.
  void addPoint(double x, double y);

  /// \brief Remove all data points.
  void clearPoints();

  /// \brief Fit a polynomial PolyCurve to the stored data points using least squares.
  /// Throws a runtime error if there are not enough points.
  void fitPolyCurve();

  /// \brief Evaluate the fitted polynomial at a given x value.
  /// \param x The x-coordinate where the PolyCurve is evaluated.
  /// \return The computed y value.
  double evaluate(double x);

  /// \brief Compute the first derivative of the fitted polynomial at a given x.
  /// \param x The x-coordinate.
  /// \return The derivative f'(x).
  double derivative(double x);

  /// \brief Get the tangent angle (in degrees) relative to the y-axis at a given x.
  ///
  /// The method computes the slope (dy/dx) at x, finds the angle (θ) the tangent makes with the x‑axis,
  /// and then returns 90° − θ.
  /// \param x The x-coordinate.
  /// \return The tangent angle relative to the y-axis.
  double getTangentAngle(double x);

  /// \brief Return the polynomial coefficients (from lowest to highest order).
  /// \return An Eigen::VectorXd of coefficients.
  Eigen::VectorXd getCoefficients();

  /// \brief Print all stored data points.
  void printPoints() const;

private:
  std::vector<double> x_points;  ///< Container for x values.
  std::vector<double> y_points;  ///< Container for y values.
  int poly_degree;               ///< Degree of the polynomial to be fitted.
  Eigen::VectorXd coeffs;        ///< Polynomial coefficients (a0, a1, ..., a_degree).
  bool fitted;                   ///< Flag indicating whether the current fit is up-to-date.
};

#endif  // PolyCurve_H
