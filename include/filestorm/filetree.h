#pragma once

#include <filestorm/filefrag.h>
#include <filestorm/utils.h>
#include <filestorm/utils/fs.h>

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

  class Node {
  public:
    std::string name;
    Type type;
    Node* parent;
    int fallocated_count = 0;
    std::map<std::string, std::unique_ptr<Node>> folders;
    std::map<std::string, std::unique_ptr<Node>> files;
    std::vector<extents> _extents;

    Node(const std::string& n, Type t, Node* p) : name(n), type(t), parent(p) {}
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

    int getExtentsCount(bool update = true) {
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
      return _extents.size();
    }

    std::uintmax_t size() { return fs_utils::file_size(path(true)); }

    std::tuple<size_t, size_t> getHoleAdress(size_t blocksize, bool increment) {
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
        getHoleAdress(blocksize, false);
        return true;
      } catch (const std::exception& e) {
        return false;
      }
    }

    void truncate(std::uintmax_t new_size) {
      // There is the need to adjust the fallocation number according to new truncated size
      std::uintmax_t old_size = size();
      if (new_size > old_size) {
        return;
      }
      int index = 1;
      std::uintmax_t hole_size = old_size / 2;
      std::uintmax_t start = 0, end = hole_size;
      while (end < new_size) {
        start = end;
        hole_size = hole_size / 2;
        end = start + hole_size;
        index++;
      }
      fallocated_count = index;
    }
  };

  std::vector<Node*> all_files;
  std::vector<Node*> all_directories;
  std::vector<Node*> files_for_fallocate;

  int64_t total_extents_count = 0;

private:
  std::unique_ptr<Node> root;

public:
  FileTree(const std::string& rootName, unsigned int max_depth = 0);
  Node* addDirectory(Node* parent, const std::string& dirName);
  void remove(Node* node);
  FileTree::Node* addFile(Node* parent, const std::string& fileName);

  Node* mkdir(std::string path, bool recursively = false);
  Node* mkfile(std::string path);
  void rm(std::string path, bool recursively = false);

  Node* getNode(std::string path);

  void print() const;

  Node* getRoot() const;

  int getDirectoryCount() const { return directory_count; }
  int getFileCount() const { return file_count; }

  std::string newDirectoryPath();
  std::string newFilePath();

  Node* randomFile();
  Node* randomDirectory();
  Node* randomPunchableFile(size_t blocksize, bool commit);
  bool hasPunchableFiles();
  void checkFallocatability(Node* file, size_t blocksize);

  void leafDirWalk(std::function<void(Node*)> f);
  void bottomUpDirWalk(Node* node, std::function<void(Node*)> f);

private:
  void printRec(const Node* node, int depth) const;
};

/* File tree example:
FileTree tree("root");

  auto dir1 = tree.addDirectory(tree.root.get(), "dir1");
  tree.addFile(dir1, "file1.txt");
  auto subdir = tree.addDirectory(dir1, "subdir");
  tree.addFile(subdir, "file2.txt");
  tree.addDirectory(tree.root.get(), "dir2");

  tree.print();
*/