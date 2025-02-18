#include <filestorm/polycurve.h>

#include <Eigen/Dense>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

// Constructor: set the polynomial degree and subsampling count.
// Throws if degree < 0 or subsampling < 1.
PolyCurve::PolyCurve(int degree, int subsampling) : poly_degree(degree), subsampling(subsampling), fitted(false), maximum_value(-std::numeric_limits<float>::infinity()) {
  if (degree < 0) throw std::invalid_argument("Degree must be non-negative");
  if (subsampling < 1) throw std::invalid_argument("Subsampling must be at least 1");

  // Initialize the coefficients vector to zeros (size = degree+1).
  coeffs = Eigen::VectorXd::Zero(poly_degree + 1);

  // Reserve some space to reduce reallocation overhead.
  y_points.reserve(1024);
  y_subsample_buffer.reserve(subsampling);
}

// Add a new point. If subsampling is enabled (subsampling > 1),
// buffer incoming values until 'subsampling' points arrive, then average.
void PolyCurve::addPoint(float y) {
  if (subsampling == 1) {
    y_points.push_back(y);
    if (y > maximum_value) maximum_value = y;
    fitted = false;
  } else {
    y_subsample_buffer.push_back(y);
    if (y_subsample_buffer.size() == static_cast<size_t>(subsampling)) {
      // Compute the average manually to avoid extra function calls.
      float sum = 0.0f;
      for (float v : y_subsample_buffer) sum += v;
      float avg = sum / subsampling;
      y_points.push_back(avg);
      if (avg > maximum_value) maximum_value = avg;
      y_subsample_buffer.clear();
      fitted = false;
    }
  }
}

// Remove the oldest data point.
// (If many removals occur, consider using std::deque instead of std::vector.)
void PolyCurve::popOldestPoint() {
  if (!y_points.empty()) {
    y_points.erase(y_points.begin());
    fitted = false;
  }
}

// Remove the specified number of oldest points.
void PolyCurve::popOldestPoints(size_t count) {
  if (count >= y_points.size())
    y_points.clear();
  else
    y_points.erase(y_points.begin(), y_points.begin() + count);
  fitted = false;
}

// Remove all stored points and clear any subsampling buffers.
void PolyCurve::clearPoints() {
  y_points.clear();
  y_subsample_buffer.clear();
  fitted = false;
}

// Fit the polynomial using the normal equations.
// We compute the sums S[m] = Σ xᵢ^m for m=0..2*d and
// T[k] = Σ yᵢ·xᵢ^k for k=0..d, where xᵢ = i*x_step (with x_step = maximum_value/n).
// Then we solve for the coefficients of f(x) = a₀ + a₁x + … + a_d x^d.
void PolyCurve::fitPolyCurve() {
  size_t n = y_points.size();
  int d = poly_degree;
  if (n < static_cast<size_t>(d + 1)) throw std::runtime_error("Not enough points to fit the polynomial");

  // Use maximum_value (set from added y-points) to define the x-range.
  double x_step = static_cast<double>(maximum_value) / static_cast<double>(n);

  // Precompute sums: S[m] = sum_{i=0}^{n-1} (x_i)^m for m = 0 ... 2*d,
  // and T[k] = sum_{i=0}^{n-1} y_points[i]*(x_i)^k for k = 0 ... d.
  std::vector<double> S(2 * d + 1, 0.0);
  std::vector<double> T(d + 1, 0.0);

  for (size_t i = 0; i < n; ++i) {
    double x = x_step * i;
    double p = 1.0;  // p will hold x^m for m = 0,1,...
    for (int m = 0; m <= 2 * d; ++m) {
      if (m <= d) T[m] += y_points[i] * p;
      S[m] += p;
      p *= x;
    }
  }

  // Build the symmetric (d+1)x(d+1) normal matrix M where M(j,k) = S[j+k].
  Eigen::MatrixXd M(d + 1, d + 1);
  for (int j = 0; j <= d; ++j) {
    for (int k = 0; k <= d; ++k) {
      M(j, k) = S[j + k];
    }
  }

  // Build the right-hand side vector.
  Eigen::VectorXd rhs(d + 1);
  for (int j = 0; j <= d; ++j) rhs(j) = T[j];

  // Solve the system M * a = rhs.
  // Using LDLT is fast for small systems (and typically sufficient if x-values are well-spaced).
  coeffs = M.ldlt().solve(rhs);
  fitted = true;
}

// Evaluate the fitted polynomial at a given x using Horner's method.
float PolyCurve::evaluate(float x) {
  if (!fitted) throw std::runtime_error("Polynomial is not fitted");

  double result = coeffs[coeffs.size() - 1];
  for (int i = coeffs.size() - 2; i >= 0; --i) result = result * x + coeffs[i];
  return static_cast<float>(result);
}

// Evaluate the derivative f'(x). For f(x) = a₀ + a₁x + a₂x² + …,
// f'(x) = a₁ + 2*a₂*x + 3*a₃*x² + ….
float PolyCurve::derivative(float x) {
  if (!fitted) throw std::runtime_error("Polynomial is not fitted");

  double result = 0.0;
  double x_power = 1.0;  // Holds x^(i-1) for the i-th term.
  for (int i = 1; i < coeffs.size(); ++i) {
    result += i * coeffs[i] * x_power;
    x_power *= x;
  }
  return static_cast<float>(result);
}

// Compute the tangent angle (in degrees) relative to the y-axis at x.
// The derivative gives the slope (dy/dx), so theta = atan(dy/dx) is the angle from the x-axis.
// Thus, the angle from the y-axis is (90° - theta).
float PolyCurve::getTangentAngle(float x) {
  float deriv = derivative(x);
  double theta = std::atan(deriv);        // angle in radians (from x-axis)
  double angle_rad = (M_PI / 2) - theta;  // relative to y-axis
  double angle_deg = angle_rad * 180.0 / M_PI;
  return static_cast<float>(angle_deg);
}

// Return the coefficients by const-reference to avoid copying.
// Throws if the polynomial has not yet been fitted.
const Eigen::VectorXd& PolyCurve::getCoefficients() const {
  if (!fitted) throw std::runtime_error("Polynomial is not fitted");
  return coeffs;
}

// Print all stored points.
void PolyCurve::printPoints() const {
  for (size_t i = 0; i < y_points.size(); ++i) std::cout << "(" << y_points[i] << ")\n";
}

// Return the number of stored points.
size_t PolyCurve::getPointCount() const { return y_points.size(); }

// Return the slope (first-order coefficient) if available.
float PolyCurve::slope() const {
  if (poly_degree >= 1) return coeffs[1];
  return 0.0f;
}

// Return the slope angle in degrees.
float PolyCurve::slopeAngle() const {
  double angle_rad = std::atan(slope());
  double angle_deg = angle_rad * 180.0 / M_PI;
  return static_cast<float>(angle_deg);
}
