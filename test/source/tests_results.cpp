#include <doctest/doctest.h>
#include <filestorm/result.h>

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

TEST_CASE("Testing Result class functionality") {
  // Test actionToString and operationToString
  SUBCASE("actionToString converts actions to string representations") {
    CHECK(Result::actionToString(Result::Action::CREATE_FILE) == std::string("CREATE_FILE"));
    CHECK(Result::actionToString(Result::Action::DELETE_FILE) == std::string("DELETE_FILE"));
    // Add checks for all actions...
  }

  SUBCASE("operationToString converts operations to string representations") {
    CHECK(Result::operationToString(Result::Operation::WRITE) == std::string("WRITE"));
    CHECK(Result::operationToString(Result::Operation::TRIM) == std::string("TRIM"));
    // Add checks for all operations...
  }

  // Test getters and setters
  SUBCASE("getters and setters work correctly") {
    Result r;
    r.setIteration(1);
    r.setAction(Result::CREATE_FILE);
    r.setOperation(Result::WRITE);
    r.setPath("/test/path");
    r.setSize(DataSize<DataUnit::B>(1024));
    r.setDuration(std::chrono::nanoseconds(100));
    r.setExtentsCount(5);

    CHECK(r.getIteration() == 1);
    CHECK(r.getAction() == Result::CREATE_FILE);
    CHECK(r.getOperation() == Result::WRITE);
    CHECK(r.getPath() == "/test/path");
    CHECK(r.getSize().get_value() == 1024);
    CHECK(r.getDuration().count() == 100);
    CHECK(r.getExtentsCount() == 5);
  }

  // Test commit and save
  SUBCASE("commit adds result to results and save persists them") {
    // Cleanup before test
    Result::results.clear();
    std::string filename = "test_results.json";
    std::filesystem::remove(filename);

    Result r(1, Result::CREATE_DIR, Result::WRITE, "/dir/path", DataSize<DataUnit::B>(2048), std::chrono::nanoseconds(200), 10, 0);
    r.commit();

    CHECK(Result::results.size() == 1);  // Ensure the result is committed

    // Now test save functionality
    Result::save(filename);
    std::ifstream file(filename);
    CHECK(file.good());  // Check if file exists

    // Cleanup after test
    std::filesystem::remove(filename);
  }
}
