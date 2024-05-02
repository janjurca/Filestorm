#include <filestorm/filetree.h>
#include <filestorm/utils.h>
#include <fmt/format.h>  // Include the necessary header file

#include <queue>

std::atomic<int> FileTree::directory_count(0);
std::atomic<int> FileTree::file_count(0);

std::atomic<int> FileTree::directory_id(0);
std::atomic<int> FileTree::file_id(0);

FileTree::FileTree(const std::string& rootName, unsigned int max_depth) : _max_depth(max_depth) { root = std::make_shared<Node>(rootName, Type::DIRECTORY, nullptr); }

FileTree::Nodeptr FileTree::addDirectory(Nodeptr parent, const std::string& dirName) {
  if (parent->type == Type::FILE) {
    throw std::runtime_error("Can't add directory to a file node!");
  }
  if (parent->folders.find(dirName) != parent->folders.end()) {
    throw std::runtime_error("Directory already exists!");
  }
  parent->folders[dirName] = std::make_shared<Node>(dirName, Type::DIRECTORY, parent);
  all_directories.push_back(parent->folders[dirName]);
  directory_count++;
  return parent->folders[dirName];
}

FileTree::Nodeptr FileTree::addFile(Nodeptr parent, const std::string& fileName) {
  if (parent->type == Type::FILE) {
    throw std::runtime_error("Can't add file to a file node!");
  }
  if (parent->files.find(fileName) != parent->files.end()) {
    throw std::runtime_error("File already registered!");
  }
  parent->files[fileName] = std::make_shared<Node>(fileName, Type::FILE, parent);
  all_files.push_back(parent->files[fileName]);
  files_for_fallocate.push_back(parent->files[fileName]);
  file_count++;
  return parent->files[fileName];
}

void FileTree::print() const { printRec(root, 0); }

void FileTree::printRec(const Nodeptr node, int depth) const {
  for (int i = 0; i < depth; ++i) std::cout << "--";
  std::cout << node->name;
  if (node->type == Type::DIRECTORY) {
    std::cout << "/";
  }
  std::cout << std::endl;

  for (const auto& child : node->folders) {
    printRec(child.second, depth + 1);
  }
  for (const auto& child : node->files) {
    for (int i = 0; i < depth + 1; ++i) std::cout << "--";
    std::cout << child.second->name << std::endl;
  }
}

void FileTree::remove(Nodeptr node) {
  if (node->parent == nullptr) {
    throw std::runtime_error("Can't remove root node!");
  }
  // recursively remove all children and optionally parent
  while (!node->folders.empty() || !node->files.empty()) {
    if (!node->folders.empty()) remove(node->folders.begin()->second);
    if (!node->files.empty()) remove(node->files.begin()->second);
  }
  if (node->type == Type::DIRECTORY) {
    all_directories.erase(std::remove(all_directories.begin(), all_directories.end(), node), all_directories.end());
    node->parent->folders.erase(node->name);
    directory_count--;
  } else {
    spdlog::debug("Removing file: {} | {} | {}", node->path(), node.use_count(), fmt::ptr(node.get()));
    all_files.erase(std::remove(all_files.begin(), all_files.end(), node), all_files.end());
    spdlog::debug("Removing file: {} | {}", node->path(), node.use_count());
    files_for_fallocate.erase(std::remove(files_for_fallocate.begin(), files_for_fallocate.end(), node), files_for_fallocate.end());
    spdlog::debug("Removing file: {} | {}", node->path(), node.use_count());
    node->parent->files.erase(node->name);
    file_count--;
  }
}

FileTree::Nodeptr FileTree::getRoot() const { return root; }

FileTree::Nodeptr FileTree::mkdir(std::string path, bool recursively) {
  // split path by /
  path = strip(path, '/');
  auto pathParts = split(path, '/');
  if (pathParts.size() == 0) {
    throw std::runtime_error("Path is empty!");
  }
  Nodeptr current = root;
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
      current = current->folders[part];
    }
  }
  return current;
}

FileTree::Nodeptr FileTree::mkfile(std::string path) {
  // split path by /
  path = strip(path, '/');
  auto pathParts = split(path, '/');
  if (pathParts.size() == 0) {
    throw std::runtime_error("Path is empty!");
  }
  Nodeptr current = root;
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
      current = current->folders[part];
    }
  }
  throw std::runtime_error("mkfile error: File probably already registered!");
}

FileTree::Nodeptr FileTree::getNode(std::string path) {
  // split path by /
  if (path == "/") {
    return root;
  }
  path = strip(path, '/');
  auto pathParts = split(path, '/');
  if (pathParts.size() == 0) {
    throw std::runtime_error("Path is empty!");
  }
  Nodeptr current = root;
  for (auto& part : pathParts) {
    if (part == "") {
      throw std::runtime_error("Path contains empty part!");
    }
    if (current->files.find(part) != current->files.end()) {
      if (part == pathParts.back()) {
        return current->files[part];
      } else {
        throw std::runtime_error("Node doesn't exist! (File)");
      }
    }
    if (current->folders.find(part) != current->folders.end()) {
      current = current->folders[part];
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
  Nodeptr current = nullptr;
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

  Nodeptr current_root = root;

  while (depth < rand_selected_depth) {
    auto current_dir_count = current_root->folders.size();
    if (current_dir_count == 0) {
      break;
    }
    auto selected_dir_index = rand() % current_dir_count;
    auto selected_dir = current_root->folders.begin();
    std::advance(selected_dir, selected_dir_index);
    current_root = selected_dir->second;
    depth++;
  }

  return fmt::format("{}/{}", current_root->path(), new_dir_name);
}

std::string FileTree::newFilePath() {
  std::string new_file_name = fmt::format("file_{}", file_id++);
  unsigned int depth = 0;
  unsigned int rand_selected_depth = rand() % _max_depth;

  Nodeptr current_root = root;

  while (depth < rand_selected_depth) {
    auto current_dir_count = current_root->folders.size();
    if (current_dir_count == 0) {
      break;
    }
    auto selected_dir_index = rand() % current_dir_count;
    auto selected_dir = current_root->folders.begin();
    std::advance(selected_dir, selected_dir_index);
    current_root = selected_dir->second;
    depth++;
  }

  return fmt::format("{}/{}", current_root->path(), new_file_name);
}

FileTree::Nodeptr FileTree::randomFile() {
  if (all_files.size() == 0) {
    throw std::runtime_error("No files in the tree!");
  }

  auto random_file = all_files.begin();
  std::advance(random_file, rand() % all_files.size());
  return *random_file;
}

FileTree::Nodeptr FileTree::randomDirectory() {
  if (all_directories.size() == 0) {
    throw std::runtime_error("No directories in the tree!");
  }
  auto random_dir = all_directories.begin();
  std::advance(random_dir, rand() % all_directories.size());
  return *random_dir;
}

void FileTree::leafDirWalk(std::function<void(Nodeptr)> f) {
  std::queue<Nodeptr> q;
  q.push(root);

  while (!q.empty()) {
    auto current = q.front();
    q.pop();
    if (current->folders.size() == 0) {
      f(current);
    } else {
      for (const auto& child : current->folders) {
        q.push(child.second);
      }
    }
  }
}

void FileTree::bottomUpDirWalk(Nodeptr node, std::function<void(Nodeptr)> f) {
  if (node->folders.size() == 0) {
    if (node != root) {
      f(node);
    }
  } else {
    for (const auto& child : node->folders) {
      bottomUpDirWalk(child.second, f);
    }
    if (node != root) {
      f(node);
    }
  }
}

FileTree::Nodeptr FileTree::randomPunchableFile() {
  // return random item from punchable files list
  if (files_for_fallocate.empty()) {
    throw std::runtime_error("No punchable files in the tree!");
  }
  return files_for_fallocate.at(0);
  auto random_file = files_for_fallocate.begin();
  std::advance(random_file, rand() % files_for_fallocate.size());
  return *random_file;
}

bool FileTree::hasPunchableFiles() { return files_for_fallocate.size() > 0; }

void FileTree::removeFromPunchableFiles(Nodeptr file) { files_for_fallocate.erase(std::remove(files_for_fallocate.begin(), files_for_fallocate.end(), file), files_for_fallocate.end()); }
