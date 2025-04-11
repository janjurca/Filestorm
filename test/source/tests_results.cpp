#include <doctest/doctest.h>
#include <filestorm/result.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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

// A helper function to remove a file if it exists.
void remove_file(const std::string& filename) {
  if (std::filesystem::exists(filename)) {
    std::filesystem::remove(filename);
  }
}

TEST_CASE("Testing Result class functionality") {
  // Test actionToString and operationToString
  SUBCASE("actionToString converts actions to string representations") {
    CHECK(Result::actionToString(Result::Action::CREATE_FILE) == std::string("CREATE_FILE"));
    CHECK(Result::actionToString(Result::Action::DELETE_FILE) == std::string("DELETE_FILE"));
    CHECK(Result::actionToString(Result::Action::CREATE_FILE_FALLOCATE) == std::string("CREATE_FILE_FALLOCATE"));
    CHECK(Result::actionToString(Result::Action::CREATE_DIR) == std::string("CREATE_DIR"));
    CHECK(Result::actionToString(Result::Action::CREATE_FILE_OVERWRITE) == std::string("CREATE_FILE_OVERWRITE"));
    CHECK(Result::actionToString(Result::Action::CREATE_FILE_READ) == std::string("CREATE_FILE_READ"));
    CHECK(Result::actionToString(Result::Action::ALTER_SMALLER_TRUNCATE) == std::string("ALTER_SMALLER_TRUNCATE"));
    CHECK(Result::actionToString(Result::Action::ALTER_SMALLER_FALLOCATE) == std::string("ALTER_SMALLER_FALLOCATE"));
    CHECK(Result::actionToString(Result::Action::ALTER_BIGGER) == std::string("ALTER_BIGGER"));
    CHECK(Result::actionToString(Result::Action::ALTER_BIGGER_WRITE) == std::string("ALTER_BIGGER_WRITE"));
    CHECK(Result::actionToString(Result::Action::ALTER_BIGGER_FALLOCATE) == std::string("ALTER_BIGGER_FALLOCATE"));
    CHECK(Result::actionToString(Result::Action::NONE) == std::string("NONE"));
  }

  SUBCASE("operationToString converts operations to string representations") {
    CHECK(Result::operationToString(Result::Operation::WRITE) == std::string("WRITE"));
    CHECK(Result::operationToString(Result::Operation::TRIM) == std::string("TRIM"));
    CHECK(Result::operationToString(Result::Operation::OVERWRITE) == std::string("OVERWRITE"));
    CHECK(Result::operationToString(Result::Operation::READ) == std::string("READ"));
    CHECK(Result::operationToString(Result::Operation::FALLOCATE) == std::string("FALLOCATE"));
  }

  // Test getters and setters
  SUBCASE("getters and setters work correctly") {
    Result r;
    r.setIteration(1);
    r.setAction(Result::Action::CREATE_FILE);
    r.setOperation(Result::Operation::WRITE);
    r.setPath("/test/path");
    r.setSize(DataSize<DataUnit::B>(1024));
    r.setDuration(std::chrono::nanoseconds(100));
    r.setExtentsCount(5);
    r.setFileExtentCount(3);

    CHECK(r.getIteration() == 1);
    CHECK(r.getAction() == Result::Action::CREATE_FILE);
    CHECK(r.getOperation() == Result::Operation::WRITE);
    CHECK(r.getPath() == "/test/path");
    CHECK(r.getSize().get_value() == 1024);
    CHECK(r.getDuration().count() == 100);
    CHECK(r.getExtentsCount() == 5);
    CHECK(r.getFileExtentCount() == 3);
  }

  // Test throughput calculation based on duration and size
  SUBCASE("throughput calculation is correct") {
    // Let's calculate throughput for 2048 Bytes in 200 ns.
    Result r(1, Result::Action::CREATE_DIR, Result::Operation::WRITE, "/dir/path", DataSize<DataUnit::B>(2048), std::chrono::nanoseconds(200), 10, 0);
    DataSize<DataUnit::B> throughput = r.getThroughput();
    // Throughput is calculated as: size / (duration in seconds)
    // For 2048 B in 200 ns, the expected throughput is:
    // 2048 / (200/1e9) = 2048 / 2e-7 = 1.024e10 Bytes/s
    double expected = 2048.0 / (200.0 / 1e9);
    CHECK(doctest::Approx(throughput.get_value()).epsilon(0.001) == expected);
  }

  // Test commit and save
  SUBCASE("commit adds result to results and save persists them") {
    // Cleanup before test
    Result::results.clear();
    std::string filename = "test_results.json";
    remove_file(filename);

    Result r(1, Result::Action::CREATE_DIR, Result::Operation::WRITE, "/dir/path", DataSize<DataUnit::B>(2048), std::chrono::nanoseconds(200), 10, 0);
    r.commit();

    CHECK(Result::results.size() == 1);  // Ensure the result is committed

    // Now test save functionality
    Result::save(filename);
    std::ifstream file(filename);
    CHECK(file.good());  // Check if file exists

    // Optionally, load the JSON file and check its content here.
    file.close();

    // Cleanup after test
    remove_file(filename);
  }

  // Test getActionResults functionality and statistics computation
  SUBCASE("getActionResults returns only matching results and computes statistics correctly") {
    // Clear previous results and add multiple results
    Result::results.clear();

    // Add several results with two different actions
    Result r1(1, Result::Action::CREATE_FILE, Result::Operation::WRITE, "/path1", DataSize<DataUnit::B>(1024), std::chrono::nanoseconds(100), 5, 2);
    Result r2(2, Result::Action::CREATE_FILE, Result::Operation::WRITE, "/path2", DataSize<DataUnit::B>(2048), std::chrono::nanoseconds(200), 6, 3);
    Result r3(3, Result::Action::DELETE_FILE, Result::Operation::TRIM, "/path3", DataSize<DataUnit::B>(4096), std::chrono::nanoseconds(400), 8, 4);
    r1.commit();
    r2.commit();
    r3.commit();

    // Retrieve results with action CREATE_FILE
    auto createFileResults = Result::getActionResults(Result::Action::CREATE_FILE);
    CHECK(createFileResults.size() == 2);

    // Calculate basic statistics on throughput; note that throughput for each is: size in bytes / (duration in seconds)
    // r1: 1024 / (100/1e9) = 1.024e10, r2: 2048 / (200/1e9) = 1.024e10.
    auto stats = Result::getStatistics<double>(createFileResults, "throughput");
    // Since both computed throughputs are equal, mean, median, min and max should all be the same.
    CHECK(stats.mean == stats.min);
    CHECK(stats.mean == stats.max);
    CHECK(stats.stddev == 0);
  }
}

TEST_CASE("Testing BasicResult class functionality") {
  // Clear existing BasicResult results
  BasicResult::results.clear();
  std::string filename = "basic_test_results.json";
  remove_file(filename);

  SUBCASE("Adding results and commit functionality") {
    BasicResult basic;
    basic.addResult("key1", "value1");
    basic.addResult("key2", "value2");
    basic.setTimestamp(std::chrono::nanoseconds(500));
    basic.commit();

    CHECK(BasicResult::results.size() == 1);
  }

  SUBCASE("Save BasicResult to JSON file") {
    // Commit one basic result
    BasicResult basic;
    basic.addResult("status", "success");
    basic.setTimestamp(std::chrono::nanoseconds(600));
    basic.commit();

    // Save the basic results
    BasicResult::save(filename);
    std::ifstream file(filename);
    CHECK(file.good());

    // Optionally, you could parse and inspect the JSON contents here
    file.close();
    remove_file(filename);
  }
}
