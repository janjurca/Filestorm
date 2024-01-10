#include <filestorm/filetree.h>
#include <filestorm/utils.h>

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
  while (node->children.size() > 0) {
    remove(node->children.begin()->second.get());
  }
  if (node->type == Type::DIRECTORY) {
    directory_count--;
  } else {
    file_count--;
  }

  if (node->parent == nullptr) {
    throw std::runtime_error("Can't remove root node!");
  }
  node->parent->children.erase(node->name);
}

FileTree::Node* FileTree::getRoot() const { return root.get(); }

FileTree::Node* FileTree::mkdir(std::string path, bool recursively) {
  // split path by /
  auto pathParts = split(path, '/');
  if (pathParts.size() == 0) {
    throw std::runtime_error("Path is empty!");
  }
  Node* current = root.get();
  for (auto& part : pathParts) {
    if (part == "") {
      throw std::runtime_error("Path contains empty part!");
    }
    if (current->children.find(part) == current->children.end()) {
      if (recursively or part == pathParts.back()) {
        current = addDirectory(current, part);
      } else {
        throw std::runtime_error("Directory doesn't exist!");
      }
    } else {
      if (part == pathParts.back()) {
        throw std::runtime_error("Directory already registered!");
      }
      current = current->children[part].get();
    }
  }
  return current;
}

FileTree::Node* FileTree::mkfile(std::string path) {
  // split path by /
  auto pathParts = split(path, '/');
  if (pathParts.size() == 0) {
    throw std::runtime_error("Path is empty!");
  }
  Node* current = root.get();
  for (auto& part : pathParts) {
    if (part == "") {
      throw std::runtime_error("Path contains empty part!");
    }
    if (current->children.find(part) == current->children.end()) {
      if (part == pathParts.back()) {
        current = addFile(current, part);
        return current;
      } else {
        throw std::runtime_error("Directory isn't registered!");
      }
    } else if (current->children[part]->type == Type::FILE) {
      throw std::runtime_error("File already registered!");
    } else {
      current = current->children[part].get();
    }
  }
  throw std::runtime_error("mkfile error: File probably already registered!");
}

FileTree::Node* FileTree::getNode(std::string path) {
  // split path by /
  if (path == "/") {
    return root.get();
  }
  auto pathParts = split(path, '/');
  if (pathParts.size() == 0) {
    throw std::runtime_error("Path is empty!");
  }
  Node* current = root.get();
  for (auto& part : pathParts) {
    if (part == "") {
      throw std::runtime_error("Path contains empty part!");
    }
    if (current->children.find(part) == current->children.end()) {
      throw std::runtime_error("Node doesn't exist!");
    } else {
      current = current->children[part].get();
    }
  }
  return current;
}

void FileTree::rm(std::string path, bool recursively) {
  // split path by /
  auto pathParts = split(path, '/');
  if (pathParts.size() == 0) {
    throw std::runtime_error("Path is empty!");
  }
  Node* current = nullptr;
  if (recursively) {
    try {
      current = getNode(path);
    } catch (const std::exception& e) {
      return;
    }
    remove(current);
    return;
  } else {
    current = getNode(path);
    if (current->type == Type::FILE) {
      remove(current);
      return;
    } else {
      if (current->children.size() == 0) {
        remove(current);
        return;
      } else {
        throw std::runtime_error("Directory isn't empty!");
      }
    }
  }
}