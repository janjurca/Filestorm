#pragma once

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

  struct Node {
    std::string name;
    Type type;
    Node* parent;
    long size;
    std::map<std::string, std::unique_ptr<Node>> children;

    Node(const std::string& n, Type t, Node* p, long size = 0) : name(n), type(t), parent(p), size(size) {}
  };

private:
  std::unique_ptr<Node> root;

public:
  FileTree(const std::string& rootName);
  Node* addDirectory(Node* parent, const std::string& dirName);
  void remove(Node* node);
  FileTree::Node* addFile(Node* parent, const std::string& fileName, long size = 0);

  Node* mkdir(std::string path, bool recursively = false);
  Node* mkfile(std::string path);
  void rm(std::string path, bool recursively = false);

  Node* getNode(std::string path);

  void print() const;

  Node* getRoot() const;

  const int getDirectoryCount() const { return directory_count; }
  const int getFileCount() const { return file_count; }

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