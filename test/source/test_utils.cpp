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

  // Add more test cases here if needed
}