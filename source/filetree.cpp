#include <filestorm/filetree.h>

std::atomic<int> FileTree::directory_count(0);
std::atomic<int> FileTree::file_count(0);

FileTree::FileTree(const std::string& rootName) { root = std::make_unique<Node>(rootName, Type::DIRECTORY, nullptr); }

FileTree::Node* FileTree::addDirectory(Node* parent, const std::string& dirName) {
  if (parent->type == Type::FILE) {
    throw std::runtime_error("Can't add directory to a file node!");
  }
  if (parent->children.find(dirName) != parent->children.end()) {
    throw std::runtime_error("Directory already exists!");
  }
  parent->children[dirName] = std::make_unique<Node>(dirName, Type::DIRECTORY, parent);
  directory_count++;
  return parent->children[dirName].get();
}

FileTree::Node* FileTree::addFile(Node* parent, const std::string& fileName, long size) {
  if (parent->type == Type::FILE) {
    throw std::runtime_error("Can't add file to a file node!");
  }
  if (parent->children.find(fileName) != parent->children.end()) {
    throw std::runtime_error("File already exists!");
  }
  parent->children[fileName] = std::make_unique<Node>(fileName, Type::FILE, parent, size);
  file_count++;
  return parent->children[fileName].get();
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
    printRec(child.second.get(), depth + 1);
  }
}

void FileTree::remove(Node* node) {
  // recursively remove all children and optionaly parent
  for (auto& child : node->children) {
    remove(child.second.get());
  }
  if (node->children.size() > 0) {
    throw std::runtime_error("Node has children! Even though they should have been removed!");
  }

  if (node->type == Type::DIRECTORY) {
    directory_count--;
  } else {
    file_count--;
  }
  node->parent->children.erase(node->name);
}

FileTree::Node* FileTree::getRoot() const { return root.get(); }