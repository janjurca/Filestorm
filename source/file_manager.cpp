#include <filestorm/file_manager.h>

FileStrategy::FileStrategy(int file_count, int directory_count_per_level, int directory_depth,
                           int file_size, int file_size_variance, enum FileContent file_content)
    : _file_count(file_count),
      _directory_count_per_level(directory_count_per_level),
      _directory_depth(directory_depth),
      _file_size(file_size),
      _file_size_variance(file_size_variance),
      _file_content(file_content) {}

FileManager::FileManager(FileStrategy file_strategy) {}
