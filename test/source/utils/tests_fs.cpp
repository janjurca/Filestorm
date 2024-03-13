#include <doctest/doctest.h>
#include <filestorm/utils/fs.h>

#include <fstream>

TEST_CASE("Testing fs_utils functions") {
  // Setup a temporary file for testing
  const std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
  const std::filesystem::path temp_file = temp_dir / "temp_file_for_testing.txt";

  SUBCASE("file_size returns correct size for a known file") {
    // Create a temporary file with some content
    std::ofstream out(temp_file);
    const std::string content = "Hello, world!";
    out << content;
    out.close();

    // Test file_size function
    uint64_t size = fs_utils::file_size(temp_file);
    CHECK(size == content.size());

    // Cleanup
    std::filesystem::remove(temp_file);
  }

  SUBCASE("get_fs_status returns valid space_info object") {
    // Use the temp directory for this test
    auto info = fs_utils::get_fs_status(temp_dir);

    // Check if the returned space_info object contains plausible values
    CHECK(info.capacity > 0);
    CHECK(info.free <= info.capacity);
    CHECK(info.available <= info.capacity);

    // Specific checks for the condition where capacity should not be less than free or available
    CHECK_FALSE(info.capacity < info.free);
    CHECK_FALSE(info.capacity < info.available);
  }
}
