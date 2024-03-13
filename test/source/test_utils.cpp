#include <doctest/doctest.h>
#include <filestorm/utils.h>

TEST_CASE("split function test") {
  SUBCASE("splitting a string with a single delimiter") {
    std::string str = "Hello,World";
    char delimiter = ',';
    std::vector<std::string> expected = {"Hello", "World"};
    std::vector<std::string> result = split(str, delimiter);
    CHECK(result == expected);
  }

  SUBCASE("splitting a string with multiple delimiters") {
    std::string str = "Hello|World|GitHub|Copilot";
    char delimiter = '|';
    std::vector<std::string> expected = {"Hello", "World", "GitHub", "Copilot"};
    std::vector<std::string> result = split(str, delimiter);
    CHECK(result == expected);
  }
}

// Example test cases for the split function
TEST_CASE("Testing split function") {
  CHECK(split("one,two,three", ',') == std::vector<std::string>{"one", "two", "three"});
  CHECK(split("hello", ',').size() == 1);
  CHECK(split("", ',').empty());
}

// Test cases for strip function
TEST_CASE("Testing strip function") {
  CHECK(strip("  hello  ") == "hello");
  CHECK(strip("hello") == "hello");
  CHECK(strip("") == "");
}

// Test cases for strip function with character parameter
TEST_CASE("Testing strip function with character parameter") {
  CHECK(strip("!!hello!!", '!') == "hello");
  CHECK(strip("hello", '!') == "hello");
  CHECK(strip("!!!!", '!') == "");
}

// Test cases for toLower function
TEST_CASE("Testing toLower function") {
  CHECK(toLower("HELLO") == "hello");
  CHECK(toLower("hello") == "hello");
  CHECK(toLower("HeLLo") == "hello");
}

// Test cases for toUpper function
TEST_CASE("Testing toUpper function") {
  CHECK(toUpper("hello") == "HELLO");
  CHECK(toUpper("HELLO") == "HELLO");
  CHECK(toUpper("HeLLo") == "HELLO");
}

// Test cases for ceilTo function
TEST_CASE("Testing ceilTo function") {
  double tolerance = 0.001;  // Define a suitable tolerance
  CHECK(std::abs(ceilTo(3.14159, 2) - 3.15) < tolerance);
  CHECK(std::abs(ceilTo(0.1, 0) - 1.0) < tolerance);
  CHECK(std::abs(ceilTo(123.456, 1) - 123.5) < tolerance);
}
// Test cases for stringToChrono function
TEST_CASE("Testing stringToChrono function") {
  using namespace std::chrono;
  CHECK(stringToChrono("10h") == hours(10));
  CHECK(stringToChrono("30m") == minutes(30));
  CHECK(stringToChrono("20s") == seconds(20));
  CHECK_THROWS_AS(stringToChrono("10x"), std::invalid_argument);
}

TEST_CASE("Testing d_rand function") {
  double min = 0.0;
  double max = 1.0;
  double result = d_rand(min, max);
  CHECK(result >= min);
  CHECK(result <= max);
}