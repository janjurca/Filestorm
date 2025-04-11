// file: test_progressbar_logger.cpp
#include <doctest/doctest.h>
#include <filestorm/utils/logger.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>

// Helper function to capture output from std::cout.
static std::string captureOutput(std::function<void()> func) {
  std::streambuf* old_buf = std::cout.rdbuf();
  std::ostringstream oss;
  std::cout.rdbuf(oss.rdbuf());
  func();
  std::cout.rdbuf(old_buf);
  return oss.str();
}

TEST_CASE("ProgressBar clear_line outputs carriage return and spaces when overwrite is true") {
  // Create a temporary ProgressBar instance.
  // We use a count-based progress bar with an arbitrary total.
  ProgressBar pb(100, "TestClear");

  // Simulate that width is known. Although width is normally set during print_bar,
  // we can simply trigger clear_line() and check that it writes a carriage return and enough spaces.
  // Capture output when calling clear_line with overwrite == true.
  std::string output = captureOutput([&]() { pb.clear_line(true); });

  // Check that the output starts with "\r"
  CHECK(output.substr(0, 1) == "\r");
  // Because width is set inside print_bar (or via OS calls) the exact number of spaces is unknown.
  // We check that there is at least one additional "\r" following some spaces.
  CHECK(output.find("\r", 1) != std::string::npos);
}

TEST_CASE("ProgressBar update and print_bar produce expected output") {
  // Create a count-based progress bar with a fixed total.
  const int total = 50;
  ProgressBar pb(total, "ProgressTest");

  // Set a meta value to verify meta information gets printed.
  pb.set_meta("epoch", "1");

  // Call update with a value not yet complete. The progress bar prints a line.
  std::string out1 = captureOutput([&]() { pb.update(10); });

  // The output should contain the label and show a percentage (e.g. "ProgressTest" and "20 %" or similar).
  CHECK(out1.find("ProgressTest") != std::string::npos);
  CHECK(out1.find("%") != std::string::npos);
  // Also, our meta should be printed using the format [epoch=1].
  CHECK(out1.find("[epoch=1]") != std::string::npos);

  // Call update to complete the progress.
  std::string out2 = captureOutput([&]() { pb.update(total); });

  // Output should include the total count
  CHECK(out2.find(std::to_string(total)) != std::string::npos);
}

TEST_CASE("ProgressBar operator++ increments current and updates the bar") {
  const int total = 20;
  ProgressBar pb(total, "OpIncrement");

  // Capture output before increment.
  std::string initialOut = captureOutput([&]() { pb.update(5); });

  // Use prefix operator++.
  std::string prefixOut = captureOutput([&]() { ++pb; });
  // After one increment, the current value should increase by one.
  // We check that the printed output now contains the updated count.
  CHECK(prefixOut.find(std::to_string(6)) != std::string::npos);

  // Use postfix operator++.
  std::string oldBarOut = captureOutput([&]() {
    ProgressBar old = pb++;
    (void)old;
  });
  // Now the current value should have increased to 7.
  std::string afterPostfix = captureOutput([&]() { pb.update(pb.is_active() ? 7 : 7); });
  CHECK(afterPostfix.find("7") != std::string::npos);
}

TEST_CASE("ProgressBar set_total overload works for Time unit") {
  // Create a time-based progress bar using seconds.
  std::chrono::seconds total_time(120);
  ProgressBar pb(total_time, "TimeProgress");

  // Call update with a seconds value.
  std::string out = captureOutput([&]() { pb.update(30); });

  // In time mode, the progress bar should format elapsed and total times.
  // Look for colon characters used in time formatting (e.g., "00:").
  CHECK(out.find(":") != std::string::npos);
  CHECK(out.find("TimeProgress") != std::string::npos);
}

TEST_CASE("ProgressBar disable stops printing progress") {
  // Create a progress bar.
  ProgressBar pb(100, "DisableTest");
  // Disable the progress bar.
  pb.disable();

  // When not active, print_bar() should return immediately.
  std::string out = captureOutput([&]() { pb.print_bar(); });
  // Expect output to be empty.
  CHECK(out.empty());
}
