#pragma once

#include <filestorm/data_sizes.h>

#include <filesystem>

namespace fs_utils {
  std::filesystem::space_info get_fs_status(const std::filesystem::path& path) { return std::filesystem::space(path); }
  uint64_t file_size(const std::filesystem::path& path) { return std::filesystem::file_size(path); }
}  // namespace fs_utils