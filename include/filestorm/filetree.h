#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

class FileTree {
public:
  enum class Type { FILE, DIRECTORY };

  struct Node {
    std::string name;
    Type type;
    long size;
    std::vector<std::unique_ptr<Node>> children;

    Node(const std::string& n, Type t, long size = 0) : name(n), type(t), size(size) {}
  };

private:
  std::unique_ptr<Node> root;

public:
  FileTree(const std::string& rootName);

  Node* addDirectory(Node* parent, const std::string& dirName);

  void addFile(Node* parent, const std::string& fileName, long size = 0);

  void print() const;

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