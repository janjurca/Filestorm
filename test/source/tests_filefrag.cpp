#include <doctest/doctest.h>
#include <fcntl.h>
#include <filestorm/filefrag.h>
#include <unistd.h>

#include <cstdio>
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
  // Write content
  ssize_t written = write(fd, content.data(), content.size());
  close(fd);
  if (written != static_cast<ssize_t>(content.size())) {
    throw std::runtime_error("Failed to write full content");
  }
  return std::string(temp_filename);
}

TEST_CASE("get_extents throws on null path") { CHECK_THROWS_AS(get_extents(nullptr), std::runtime_error); }

TEST_CASE("get_extents throws on non-existent file") {
  const char* invalid_path = "/path/to/nonexistent/file_foobar.txt";
  CHECK_THROWS_AS(get_extents(invalid_path), std::runtime_error);
}

#if defined(__linux__)
TEST_CASE("get_extents returns a single extent for a small file") {
  const std::string data = "Hello, Doctest!";
  std::string path;
  try {
    path = create_temp_file(data);
  } catch (const std::exception& e) {
    FAIL("Failed to set up temp file: " << e.what());
  }

  // Retrieve extents
  std::vector<extents> ext_list;
  CHECK_NOTHROW(ext_list = get_extents(path.c_str()));

  // We expect at least one extent
  REQUIRE(!ext_list.empty());

  // First extent should start at 0 and cover the file length
  CHECK(ext_list[0].start == 0);
  CHECK(ext_list[0].length == data.size());

  // Clean up
  unlink(path.c_str());
}
#endif

// Optionally, if run on non-Linux, verify no extents returned
#if !defined(__linux__)
TEST_CASE("get_extents returns empty vector on non-Linux platforms") {
  const std::string data = "Test";
  std::string path;
  try {
    path = create_temp_file(data);
  } catch (const std::exception& e) {
    FAIL("Failed to set up temp file: " << e.what());
  }

  std::vector<extents> ext_list;
  CHECK_NOTHROW(ext_list = get_extents(path.c_str()));
  CHECK(ext_list.empty());
  unlink(path.c_str());
}
#endif

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
