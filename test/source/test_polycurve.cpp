#include <doctest/doctest.h>
#include <filestorm/filefrag.h>
#include <unistd.h>

#include <cstdio>
#include <exception>
#include <fstream>
#include <stdexcept>
#include <vector>

// file: test_polycurve.cpp
#include <doctest/doctest.h>
#include <filestorm/polycurve.h>

#include <Eigen/Dense>
#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

// Helper function to capture std::cout output.
std::string captureOutput(std::function<void()> func) {
  std::streambuf* old = std::cout.rdbuf();
  std::ostringstream oss;
  std::cout.rdbuf(oss.rdbuf());
  func();
  std::cout.rdbuf(old);
  return oss.str();
}

TEST_CASE("PolyCurve constructor validates parameters") {
  // Degree must be non-negative.
  CHECK_THROWS_AS(PolyCurve(-1, 1), std::invalid_argument);
  // Subsampling must be at least 1.
  CHECK_THROWS_AS(PolyCurve(1, 0), std::invalid_argument);
}

TEST_CASE("Adding points without subsampling (subsampling==1)") {
  PolyCurve curve(0, 1);  // degree 0, no subsampling
  CHECK(curve.getPointCount() == 0);

  curve.addPoint(5.0f);
  CHECK(curve.getPointCount() == 1);
  curve.addPoint(7.0f);
  CHECK(curve.getPointCount() == 2);

  // Maximum value should be the maximum of added points.
  // (We assume internal maximum tracking works correctly.)
  curve.addPoint(6.0f);
  CHECK(curve.getPointCount() == 3);
}

TEST_CASE("Adding points with subsampling > 1") {
  const int subsampling = 3;
  PolyCurve curve(0, subsampling);  // degree 0, with buffering
  // Initially no complete points available.
  CHECK(curve.getPointCount() == 0);

  // Add 2 points; still in buffer.
  curve.addPoint(2.0f);
  curve.addPoint(4.0f);
  CHECK(curve.getPointCount() == 0);

  // Add the third point. Now the buffer should flush an averaged point.
  curve.addPoint(6.0f);
  CHECK(curve.getPointCount() == 1);

  // The average of 2,4,6 is 4.0.
  curve.fitPolyCurve();  // degree 0: constant fit; should give value 4.
  float val = curve.evaluate(0.0f);
  CHECK(val == doctest::Approx(4.0f));
}

TEST_CASE("Point removal functions: popOldestPoint, popOldestPoints, clearPoints") {
  PolyCurve curve(0, 1);
  // Add several points.
  curve.addPoint(1.0f);
  curve.addPoint(2.0f);
  curve.addPoint(3.0f);
  curve.addPoint(4.0f);
  CHECK(curve.getPointCount() == 4);

  // Remove one point.
  curve.popOldestPoint();
  CHECK(curve.getPointCount() == 3);

  // Remove two oldest points.
  curve.popOldestPoints(2);
  CHECK(curve.getPointCount() == 1);

  // Removing more points than available clears the curve.
  curve.popOldestPoints(5);
  CHECK(curve.getPointCount() == 0);

  // Test clearPoints (also clears any subsample buffers)
  curve.addPoint(10.0f);
  curve.addPoint(20.0f);
  CHECK(curve.getPointCount() == 2);
  curve.clearPoints();
  CHECK(curve.getPointCount() == 0);
}

TEST_CASE("Polynomial fitting: constant function fit") {
  // With a constant function, degree 0.
  PolyCurve curve(0, 1);
  // Add same point repeatedly.
  for (int i = 0; i < 10; i++) {
    curve.addPoint(5.0f);
  }
  // Must have enough points (for degree 0, 1 point is enough)
  CHECK_NOTHROW(curve.fitPolyCurve());
  // Evaluate at any x should return 5.
  float y = curve.evaluate(10.0f);
  CHECK(y == doctest::Approx(5.0f).epsilon(0.001));

  // Derivative should be zero.
  float deriv = curve.derivative(10.0f);
  CHECK(deriv == doctest::Approx(0.0f).epsilon(0.001));

  // Tangent angle should be 90 degrees (since derivative==0 => theta=0 from x-axis, so angle=pi/2 relative to y-axis).
  float angle = curve.getTangentAngle(10.0f);
  CHECK(angle == doctest::Approx(90.0f).epsilon(0.001));
}

TEST_CASE("Polynomial fitting: insufficient points throws") {
  // For degree 2 we need at least 3 points.
  PolyCurve curve(2, 1);
  curve.addPoint(1.0f);
  curve.addPoint(2.0f);
  // Fitting should throw because number of points (2) is less than required (3).
  CHECK_THROWS_AS(curve.fitPolyCurve(), std::runtime_error);
}

TEST_CASE("Evaluation and derivative require fitted polynomial") {
  PolyCurve curve(1, 1);
  curve.addPoint(2.0f);
  curve.addPoint(4.0f);
  // Before fitting, evaluate and derivative should throw.
  CHECK_THROWS_AS(curve.evaluate(1.0f), std::runtime_error);
  CHECK_THROWS_AS(curve.derivative(1.0f), std::runtime_error);
  // Also, getCoefficients should throw.
  CHECK_THROWS_AS(curve.getCoefficients(), std::runtime_error);

  // Now fit the polynomial.
  curve.fitPolyCurve();
  // Now evaluate and derivative should work without exceptions.
  CHECK_NOTHROW(curve.evaluate(0.5f));
  CHECK_NOTHROW(curve.derivative(0.5f));

  // Test slope and slopeAngle.
  // For a degree 1 polynomial, slope() should correspond to the linear coefficient.
  const Eigen::VectorXd& coeffs = curve.getCoefficients();
  if (coeffs.size() >= 2) {
    float slope = curve.slope();
    // Compare slope with the coefficient a1.
    CHECK(slope == doctest::Approx(coeffs[1]).epsilon(0.001));

    float angle = curve.slopeAngle();
    // Angle computed from slope.
    float expected_angle = std::atan(slope) * 180.0f / float(M_PI);
    CHECK(angle == doctest::Approx(expected_angle).epsilon(0.001));
  }
}

TEST_CASE("PrintPoints outputs stored points") {
  PolyCurve curve(0, 1);
  curve.addPoint(3.14f);
  curve.addPoint(2.71f);
  // Capture the console output.
  std::string output = captureOutput([&curve]() { curve.printPoints(); });
  // Check that both point values appear in the output.
  CHECK(output.find("3.14") != std::string::npos);
  CHECK(output.find("2.71") != std::string::npos);
}
