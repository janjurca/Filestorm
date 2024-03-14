#include <doctest/doctest.h>
#include <filestorm/filefrag.h>
#include <unistd.h>

#include <cstdio>
#include <exception>
#include <fstream>
#include <stdexcept>
#include <vector>

// Utility function to create a temporary file and return its path
std::string create_temp_file(const std::string& content) {
  char temp_filename[] = "/tmp/testfileXXXXXX";
  int fd = mkstemp(temp_filename);
  if (fd == -1) {
    throw std::runtime_error("Failed to create temporary file.");
  }

  // Write some content to the file
  write(fd, content.c_str(), content.size());
  close(fd);

  // Return the path to the temporary file
  return std::string(temp_filename);
}

TEST_CASE("Successful retrieval of file extents") {
  // Setup: Create a temporary file for testing
  std::string tempFilePath = create_temp_file("This is a test file.");

  SUBCASE("File with temporary content") {
#ifdef __linux__
    auto extents = get_extents(tempFilePath.c_str());
    CHECK(!extents.empty());  // Assuming the file system does report extents for our temp file
#else
#  warning "Skipping test for non-Linux systems"
#endif
    // Further checks can verify the extents' details if known.
  }

  // Cleanup: Remove the temporary file after the test
  std::remove(tempFilePath.c_str());
}

TEST_CASE("Error handling for non-existent file") {
  // Setup: Specifying a file path that doesn't exist
  const char* invalidFilePath = "/path/to/nonexistent/file.txt";

  SUBCASE("Throws runtime_error on missing file") { CHECK_THROWS_AS(get_extents(invalidFilePath), std::runtime_error); }
}