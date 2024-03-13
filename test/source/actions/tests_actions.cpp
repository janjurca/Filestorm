#include <doctest/doctest.h>
#include <filestorm/actions/actions.h>

TEST_CASE("FileActionAttributes functionality") {
  FileActionAttributes faa(true, DataSize<DataUnit::B>(4096), DataSize<DataUnit::KB>(512), "/example/path", std::chrono::seconds(300));

  SUBCASE("Check if action is time-based") { CHECK(faa.is_time_based() == true); }

  SUBCASE("Block size retrieval") {
    auto block_size = faa.get_block_size();
    CHECK(block_size.get_value() == 4096);
  }

  SUBCASE("File size retrieval") {
    auto file_size = faa.get_file_size();
    CHECK(file_size.get_value() == 512);
  }

  SUBCASE("File path retrieval") { CHECK(faa.get_file_path() == "/example/path"); }

  SUBCASE("Time limit retrieval") {
    auto time_limit = faa.get_time_limit();
    CHECK(time_limit == std::chrono::seconds(300));
  }

  SUBCASE("Constructor and getters integration") {
    FileActionAttributes faa_integration(false, DataSize<DataUnit::B>(2048), DataSize<DataUnit::KB>(256), "/integration/test", std::chrono::seconds(600));
    CHECK(faa_integration.is_time_based() == false);
    CHECK(faa_integration.get_block_size().get_value() == 2048);
    CHECK(faa_integration.get_file_size().get_value() == 256);
    CHECK(faa_integration.get_file_path() == "/integration/test");
    CHECK(faa_integration.get_time_limit() == std::chrono::seconds(600));
  }
}
