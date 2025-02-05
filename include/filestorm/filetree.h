#pragma once

#include <filestorm/filefrag.h>
#include <filestorm/utils.h>
#include <filestorm/utils/fs.h>
#include <filestorm/utils/logger.h>

#include <atomic>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

class FileTree {
public:
  enum class Type { FILE, DIRECTORY };
  static std::atomic<int> directory_count;
  static std::atomic<int> file_count;

  static std::atomic<int> directory_id;
  static std::atomic<int> file_id;

  unsigned int _max_depth;

  class Node;
  // using Nodeptr = std::shared_ptr<Node>;
  using Nodeptr = std::shared_ptr<Node>;

  class Node {
  public:
    std::string name;
    Type type;
    Nodeptr parent;
    int fallocated_count = 0;
    std::map<std::string, Nodeptr> folders;
    std::map<std::string, Nodeptr> files;
    std::vector<extents> _extents;

    Node(const std::string& n, Type t, Nodeptr p) : name(n), type(t), parent(p) {}
    std::string path(bool include_root = false) const {
      if (parent == nullptr) {
        if (include_root) {
          return name;
        } else {
          return "";
        }
      }
      return parent->path(include_root) + "/" + name;
    }

    int getFallocationCount() { return fallocated_count; }

    std::vector<extents> getExtents(bool update = true) {
      if (update) {
        if (type == Type::FILE) {
          _extents.clear();
          // extents_count = get_extents(path(true).c_str()).size();
          for (auto extent : get_extents(path(true).c_str())) {
            _extents.push_back(extent);
          }
        } else {
          _extents.clear();
        }
      }
      return _extents;
    }

    std::uintmax_t size() { return fs_utils::file_size(path(true)); }

    std::tuple<size_t, size_t> getHoleAddress(size_t blocksize, bool increment) {
      size_t start = 0, end = size() / 2;
      for (int i = 0; i < fallocated_count; i++) {
        size_t length = end - start;
        start = end;
        end += length / 2;
      }
      start = start - (start % blocksize);
      end = end - (end % blocksize);
      if (end - start < 3 * blocksize) {
        throw std::runtime_error(fmt::format("Not enough space for hole in file: {}, fallocated count {}", path(true), fallocated_count));
      }
      start += blocksize;
      end -= blocksize;
      if (increment) {
        fallocated_count++;
      }
      return std::make_tuple(start, end);
    }

    bool isPunchable(size_t blocksize) {
      try {
        getHoleAddress(blocksize, false);
        return true;
      } catch (const std::exception& e) {
        return false;
      }
    }

    void truncate(std::uintmax_t blocksize, std::uintmax_t new_size) {
      // There is the need to adjust the fallocation number according to new truncated size
      std::uintmax_t old_size = size();
      if (new_size >= old_size) {
        return;
      }
      int index = 1;
      std::uintmax_t hole_size = old_size / 2;
      std::uintmax_t start = 0, end = hole_size;
      while (end < new_size && hole_size > blocksize * 3) {
        if (index < 300) {
          //  logger.debug("start: {}, end: {}, hole_size: {}, new_size: {}, index: {}, old_size: {}", start, end, hole_size, new_size, index, old_size);
        } else {
          throw std::runtime_error("Too many iterations");
        }

        start = end;
        hole_size = hole_size / 2;
        end = start + hole_size;
        index++;
      }
      fallocated_count = index;
    }
  };

  std::vector<Nodeptr> all_files;
  std::vector<Nodeptr> all_directories;
  std::vector<Nodeptr> files_for_fallocate;

  int64_t total_extents_count = 0;

private:
  Nodeptr root;

public:
  FileTree(const std::string& rootName, unsigned int max_depth = 0);
  Nodeptr addDirectory(Nodeptr parent, const std::string& dirName);
  void remove(Nodeptr node);
  FileTree::Nodeptr addFile(Nodeptr parent, const std::string& fileName);

  Nodeptr mkdir(std::string path, bool recursively = false);
  Nodeptr mkfile(std::string path);
  void rm(std::string path, bool recursively = false);

  Nodeptr getNode(std::string path);

  void print() const;

  Nodeptr getRoot() const;

  int getDirectoryCount() const { return directory_count; }
  int getFileCount() const { return file_count; }

  std::string newDirectoryPath();
  std::string newFilePath();

  Nodeptr randomFile();
  Nodeptr randomDirectory();
  Nodeptr randomPunchableFile();
  bool hasPunchableFiles();
  void removeFromPunchableFiles(Nodeptr file);

  void leafDirWalk(std::function<void(Nodeptr)> f);
  void bottomUpDirWalk(Nodeptr node, std::function<void(Nodeptr)> f);

  bool findNullPointer() {
    // find null pointer in files_for fallocate
    for (auto& file : files_for_fallocate) {
      if (file == nullptr) {
        return true;
      }
    }
    return false;
  }

private:
  void printRec(const Nodeptr node, int depth) const;
};
