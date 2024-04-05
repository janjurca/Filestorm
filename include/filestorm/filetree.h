#pragma once

#include <filestorm/filefrag.h>
#include <filestorm/utils.h>

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
    long size;
    std::map<std::string, std::unique_ptr<Node>> folders;
    std::map<std::string, std::unique_ptr<Node>> files;
    std::vector<extents> _extents;

    Node(const std::string& n, Type t, Node* p, long size = 0) : name(n), type(t), parent(p), size(size) {}
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
  };

  std::vector<Node*> all_files;
  std::vector<Node*> all_directories;

  int64_t total_extents_count = 0;

private:
  std::unique_ptr<Node> root;

public:
  FileTree(const std::string& rootName, unsigned int max_depth = 0);
  Node* addDirectory(Node* parent, const std::string& dirName);
  void remove(Node* node);
  FileTree::Node* addFile(Node* parent, const std::string& fileName, long size = 0);

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