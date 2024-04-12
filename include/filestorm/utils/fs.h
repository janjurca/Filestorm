#pragma once

#include <filesystem>

namespace fs_utils {
  std::filesystem::space_info get_fs_status(const std::filesystem::path& path);
  std::uintmax_t file_size(const std::filesystem::path& path);
}  // namespace fs_utils
