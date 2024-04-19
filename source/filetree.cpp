#include <filestorm/filetree.h>
#include <filestorm/utils.h>
#include <fmt/format.h>  // Include the necessary header file

#include <queue>

std::atomic<int> FileTree::directory_count(0);
std::atomic<int> FileTree::file_count(0);

std::atomic<int> FileTree::directory_id(0);
std::atomic<int> FileTree::file_id(0);

FileTree::FileTree(const std::string& rootName, unsigned int max_depth) : _max_depth(max_depth) { root = std::make_unique<Node>(rootName, Type::DIRECTORY, nullptr); }

FileTree::Node* FileTree::addDirectory(Node* parent, const std::string& dirName) {
  if (parent->type == Type::FILE) {
    throw std::runtime_error("Can't add directory to a file node!");
  }
  if (parent->folders.find(dirName) != parent->folders.end()) {
    throw std::runtime_error("Directory already exists!");
  }
  parent->folders[dirName] = std::make_unique<Node>(dirName, Type::DIRECTORY, parent);
  all_directories.push_back(parent->folders[dirName].get());
  directory_count++;
  return parent->folders[dirName].get();
}

FileTree::Node* FileTree::addFile(Node* parent, const std::string& fileName) {
  if (parent->type == Type::FILE) {
    throw std::runtime_error("Can't add file to a file node!");
  }
  if (parent->files.find(fileName) != parent->files.end()) {
    throw std::runtime_error("File already registered!");
  }
  parent->files[fileName] = std::make_unique<Node>(fileName, Type::FILE, parent);
  all_files.push_back(parent->files[fileName].get());
  files_for_fallocate.push_back(parent->files[fileName].get());
  file_count++;
  return parent->files[fileName].get();
}

void FileTree::print() const { printRec(root.get(), 0); }

void FileTree::printRec(const Node* node, int depth) const {
  for (int i = 0; i < depth; ++i) std::cout << "--";
  std::cout << node->name;
  if (node->type == Type::DIRECTORY) {
    std::cout << "/";
  }
  std::cout << std::endl;

  for (const auto& child : node->folders) {
    printRec(child.second.get(), depth + 1);
  }
  for (const auto& child : node->files) {
    for (int i = 0; i < depth + 1; ++i) std::cout << "--";
    std::cout << child.second->name << std::endl;
  }
}

void FileTree::remove(Node* node) {
  if (node->parent == nullptr) {
    throw std::runtime_error("Can't remove root node!");
  }
  // recursively remove all children and optionaly parent
  while (node->folders.size() > 0 and node->files.size() > 0) {
    remove(node->folders.begin()->second.get());
    remove(node->files.begin()->second.get());
  }
  if (node->type == Type::DIRECTORY) {
    // all_directories.erase(std::remove(all_directories.begin(), all_directories.end(), node), all_directories.end());
    std::remove(all_directories.begin(), all_directories.end(), node);
    node->parent->folders.erase(node->name);
    directory_count--;
  } else {
    // all_files.erase(std::remove(all_files.begin(), all_files.end(), node), all_files.end());
    std::remove(all_files.begin(), all_files.end(), node);
    std::remove(files_for_fallocate.begin(), files_for_fallocate.end(), node);
    node->parent->files.erase(node->name);
    file_count--;
  }
}

FileTree::Node* FileTree::getRoot() const { return root.get(); }

FileTree::Node* FileTree::mkdir(std::string path, bool recursively) {
  // split path by /
  path = strip(path, '/');
  auto pathParts = split(path, '/');
  if (pathParts.size() == 0) {
    throw std::runtime_error("Path is empty!");
  }
  Node* current = root.get();
  for (auto& part : pathParts) {
    if (part == "") {
      throw std::runtime_error("Path contains empty part!");
    }
    if (current->folders.find(part) == current->folders.end() and current->files.find(part) == current->files.end()) {
      if (recursively or part == pathParts.back()) {
        current = addDirectory(current, part);
      } else {
        throw std::runtime_error("Directory doesn't exist!");
      }
    } else {
      if (part == pathParts.back()) {
        throw std::runtime_error("Directory already registered!");
      }
      current = current->folders[part].get();
    }
  }
  return current;
}

FileTree::Node* FileTree::mkfile(std::string path) {
  // split path by /
  path = strip(path, '/');
  auto pathParts = split(path, '/');
  if (pathParts.size() == 0) {
    throw std::runtime_error("Path is empty!");
  }
  Node* current = root.get();
  for (auto& part : pathParts) {
    if (part == "") {
      throw std::runtime_error("Path contains empty part!");
    }
    if (current->folders.find(part) == current->folders.end()) {
      if (part == pathParts.back()) {
        current = addFile(current, part);
        return current;
      } else {
        throw std::runtime_error("Directory isn't registered!");
      }
    } else {
      current = current->folders[part].get();
    }
  }
  throw std::runtime_error("mkfile error: File probably already registered!");
}

FileTree::Node* FileTree::getNode(std::string path) {
  // split path by /
  if (path == "/") {
    return root.get();
  }
  path = strip(path, '/');
  auto pathParts = split(path, '/');
  if (pathParts.size() == 0) {
    throw std::runtime_error("Path is empty!");
  }
  Node* current = root.get();
  for (auto& part : pathParts) {
    if (part == "") {
      throw std::runtime_error("Path contains empty part!");
    }
    if (current->files.find(part) != current->files.end()) {
      if (part == pathParts.back()) {
        return current->files[part].get();
      } else {
        throw std::runtime_error("Node doesn't exist! (File)");
      }
    }
    if (current->folders.find(part) != current->folders.end()) {
      current = current->folders[part].get();
    } else {
      throw std::runtime_error("Node doesn't exist! (Folder)");
    }
  }
  return current;
}

void FileTree::rm(std::string path, bool recursively) {
  // split path by /
  path = strip(path, '/');
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
      if (current->files.size() == 0 and current->folders.size() == 0) {
        remove(current);
        return;
      } else {
        throw std::runtime_error("Directory isn't empty!");
      }
    }
  }
}

std::string FileTree::newDirectoryPath() {
  std::string new_dir_name = fmt::format("dir_{}", directory_id++);
  unsigned int depth = 0;
  unsigned int rand_selected_depth = rand() % _max_depth;

  auto current_root = root.get();

  while (depth < rand_selected_depth) {
    auto current_dir_count = current_root->folders.size();
    if (current_dir_count == 0) {
      break;
    }
    auto selected_dir_index = rand() % current_dir_count;
    auto selected_dir = current_root->folders.begin();
    std::advance(selected_dir, selected_dir_index);
    current_root = selected_dir->second.get();
    depth++;
  }

  return fmt::format("{}/{}", current_root->path(), new_dir_name);
}

std::string FileTree::newFilePath() {
  std::string new_file_name = fmt::format("file_{}", file_id++);
  unsigned int depth = 0;
  unsigned int rand_selected_depth = rand() % _max_depth;

  auto current_root = root.get();

  while (depth < rand_selected_depth) {
    auto current_dir_count = current_root->folders.size();
    if (current_dir_count == 0) {
      break;
    }
    auto selected_dir_index = rand() % current_dir_count;
    auto selected_dir = current_root->folders.begin();
    std::advance(selected_dir, selected_dir_index);
    current_root = selected_dir->second.get();
    depth++;
  }

  return fmt::format("{}/{}", current_root->path(), new_file_name);
}

FileTree::Node* FileTree::randomFile() {
  if (all_files.size() == 0) {
    throw std::runtime_error("No files in the tree!");
  }

  auto random_file = all_files.begin();
  std::advance(random_file, rand() % all_files.size());
  return *random_file;
}

FileTree::Node* FileTree::randomDirectory() {
  if (all_directories.size() == 0) {
    throw std::runtime_error("No directories in the tree!");
  }
  auto random_dir = all_directories.begin();
  std::advance(random_dir, rand() % all_directories.size());
  return *random_dir;
}

void FileTree::leafDirWalk(std::function<void(Node*)> f) {
  std::queue<Node*> q;
  q.push(root.get());
  while (!q.empty()) {
    auto current = q.front();
    q.pop();
    if (current->folders.size() == 0) {
      f(current);
    } else {
      for (const auto& child : current->folders) {
        q.push(child.second.get());
      }
    }
  }
}

void FileTree::bottomUpDirWalk(Node* node, std::function<void(Node*)> f) {
  if (node->folders.size() == 0) {
    if (node != root.get()) {
      f(node);
    }
    // f(node);
  } else {
    for (const auto& child : node->folders) {
      bottomUpDirWalk(child.second.get(), f);
    }
    if (node != root.get()) {
      f(node);
    }
    // f(node);
  }
}

FileTree::Node* FileTree::randomPunchableFile(size_t blocksize) {
  if (files_for_fallocate.size() == 0) {
    throw std::runtime_error("No punchable files in the tree!");
  }
  auto random_file = files_for_fallocate.begin();
  return *random_file;
}

bool FileTree::hasPunchableFiles() { return files_for_fallocate.size() > 0; }

void FileTree::checkFallocatability(Node* file, size_t blocksize) {
  if (!file->isPunchable(blocksize)) {
    // remove node from files_for_fallocate
    auto it = std::find(files_for_fallocate.begin(), files_for_fallocate.end(), file);
    if (it != files_for_fallocate.end()) {
      files_for_fallocate.erase(it);
    }
  }
}

void FileTree::removeFromPunchableFiles(Node* file) {
  auto it = std::find(files_for_fallocate.begin(), files_for_fallocate.end(), file);
  if (it != files_for_fallocate.end()) {
    files_for_fallocate.erase(it);
  }
}