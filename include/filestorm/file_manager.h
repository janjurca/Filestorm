#pragma once

class FileStrategy {
private:
  enum FileContent { RANDOM, ZERO, ONE };
  int _file_count;
  int _directory_count_per_level;
  int _directory_depth;
  int _file_size;
  int _file_size_variance;
  enum FileContent _file_content;
  /* data */
public:
  FileStrategy(int file_count, int directory_count_per_level, int directory_depth, int file_size,
               int file_size_variance, enum FileContent file_content);
  ~FileStrategy();
  int file_count() const { return _file_count; }
  int directory_count_per_level() const { return _directory_count_per_level; }
  int directory_depth() const { return _directory_depth; }
  int file_size() const { return _file_size; }
  int file_size_variance() const { return _file_size_variance; }
  enum FileContent file_content() const { return _file_content; }
};

class FileManager {
private:
  /* data */
public:
  FileManager(FileStrategy file_strategy);
};
