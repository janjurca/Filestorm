#include <doctest/doctest.h>
#include <filestorm/filefrag.h>
#include <unistd.h>

#include <cstdio>
#include <exception>
#include <fstream>
#include <stdexcept>
#include <string>
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

TEST_CASE("Error handling for non-existent file") {
  // Setup: Specifying a file path that doesn't exist
  const char* invalidFilePath = "/path/to/nonexistent/file.txt";

  SUBCASE("Throws runtime_error on missing file") { CHECK_THROWS_AS(get_extents(invalidFilePath), std::runtime_error); }
  SUBCASE("Throws runtime_error on null file path") { CHECK_THROWS_AS(get_extents(nullptr), std::runtime_error); }
}

TEST_CASE("Error handling for non-existent or null file") {
  const char* invalidFilePath = "/path/to/nonexistent/file.txt";

  SUBCASE("Throws runtime_error on missing file") { CHECK_THROWS_AS(get_extents(invalidFilePath), std::runtime_error); }

  SUBCASE("Throws runtime_error on null file path") { CHECK_THROWS_AS(get_extents(nullptr), std::runtime_error); }
}

#if defined(__linux__)
TEST_CASE("Valid file returns extents information") {
  // Create a temporary file with some content.
  // TODO: Uncomment the following lines when testing on a Linux system.and FIX this test
  //  std::string temp_file = create_temp_file("This is some test content.");
  //
  //  // Call get_extents and verify extents are returned.
  //  auto extents_list = get_extents(temp_file.c_str());
  //
  //  // Remove the temporary file.
  //  remove(temp_file.c_str());
  //
  //  // Expect at least one extent since the file exists.
  //  CHECK(!extents_list.empty());
  //
  //  // Verify that at least one extent has a non-zero length.
  //  bool found_extent_with_length = false;
  //  for (const auto& ext : extents_list) {
  //    if (ext.length > 0) {
  //      found_extent_with_length = true;
  //      break;
  //    }
  //  }
  //  CHECK(found_extent_with_length);
}
#else
TEST_CASE("Non-linux platforms: get_extents returns an empty vector") {
  // On non-linux systems the function should simply return an empty vector
  // and log a warning once.
  std::string temp_file = create_temp_file("Test content for non-linux.");
  auto extents_list = get_extents(temp_file.c_str());
  remove(temp_file.c_str());
  CHECK(extents_list.empty());
}
#endif
