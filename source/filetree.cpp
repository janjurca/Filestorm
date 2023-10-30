#include <filestorm/filetree.h>

FileTree::FileTree(const std::string& rootName) {
  root = std::make_unique<Node>(rootName, Type::DIRECTORY);
}

FileTree::Node* FileTree::addDirectory(Node* parent, const std::string& dirName) {
  if (parent->type == Type::FILE) {
    throw std::runtime_error("Can't add directory to a file node!");
  }
  parent->children.emplace_back(std::make_unique<Node>(dirName, Type::DIRECTORY));
  return parent->children.back().get();
}

void FileTree::addFile(Node* parent, const std::string& fileName, long size) {
  if (parent->type == Type::FILE) {
    throw std::runtime_error("Can't add file to a file node!");
  }
  parent->children.emplace_back(std::make_unique<Node>(fileName, Type::FILE, size));
}

void FileTree::print() const { printRec(root.get(), 0); }

void FileTree::printRec(const Node* node, int depth) const {
  for (int i = 0; i < depth; ++i) std::cout << "--";
  std::cout << node->name;
  if (node->type == Type::DIRECTORY) {
    std::cout << "/";
  }
  std::cout << std::endl;

  for (const auto& child : node->children) {
    printRec(child.get(), depth + 1);
  }
}