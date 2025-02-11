
#include <filestorm/polyPolyCurve.h>

PolyCurve::PolyCurve(int degree) : poly_degree(degree), fitted(false) {}

void PolyCurve::addPoint(double x, double y) {
  x_points.push_back(x);
  y_points.push_back(y);
  fitted = false;
}

void PolyCurve::clearPoints() {
  x_points.clear();
  y_points.clear();
  fitted = false;
}

void PolyCurve::fitPolyCurve() {
  const int n = static_cast<int>(x_points.size());
  if (n < poly_degree + 1) {
    throw std::runtime_error("Not enough points to fit a polynomial of degree " + std::to_string(poly_degree));
  }

  // Build the design matrix A where each row is [1, x, x^2, ..., x^(poly_degree)]
  Eigen::MatrixXd A(n, poly_degree + 1);
  Eigen::VectorXd b(n);
  for (int i = 0; i < n; ++i) {
    double xi = x_points[i];
    for (int j = 0; j <= poly_degree; ++j) {
      A(i, j) = std::pow(xi, j);
    }
    b(i) = y_points[i];
  }

  // Solve the least squares problem for the polynomial coefficients.
  coeffs = A.householderQr().solve(b);
  fitted = true;
}

double PolyCurve::evaluate(double x) {
  if (!fitted) {
    fitPolyCurve();
  }
  double result = 0.0;
  for (int i = 0; i < coeffs.size(); ++i) {
    result += coeffs[i] * std::pow(x, i);
  }
  return result;
}

double PolyCurve::derivative(double x) {
  if (!fitted) {
    fitPolyCurve();
  }
  double result = 0.0;
  // The derivative of a_i * x^i is i * a_i * x^(i-1)
  for (int i = 1; i < coeffs.size(); ++i) {
    result += i * coeffs[i] * std::pow(x, i - 1);
  }
  return result;
}

double PolyCurve::getTangentAngle(double x) {
  double slope = derivative(x);
  // Handle near-zero slope (horizontal tangent)
  if (std::fabs(slope) < 1e-10) {
    return 90.0;
  }
  double theta_x = std::atan(std::fabs(slope)) * 180.0 / M_PI;  // Angle relative to x-axis in degrees
  double theta_y = 90.0 - theta_x;                              // Angle relative to y-axis
  return theta_y;
}

Eigen::VectorXd PolyCurve::getCoefficients() {
  if (!fitted) {
    fitPolyCurve();
  }
  return coeffs;
}

void PolyCurve::printPoints() const {
  std::cout << "Data Points:" << std::endl;
  for (size_t i = 0; i < x_points.size(); ++i) {
    std::cout << "(" << x_points[i] << ", " << y_points[i] << ")" << std::endl;
  }
}
