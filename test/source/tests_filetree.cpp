#include <doctest/doctest.h>
#include <filestorm/filetree.h>

#include <iostream>
#include <sstream>
#include <string>

TEST_CASE("Testing FileTree") {
  FileTree tree("root");

  SUBCASE("Adding directories and files") {
    auto dir1 = tree.addDirectory(tree.getRoot(), "dir1");
    CHECK(dir1 != nullptr);
    CHECK(dir1->name == "dir1");

    auto file1 = tree.addFile(dir1, "file1.txt");
    CHECK(file1 != nullptr);
    CHECK(file1->name == "file1.txt");

    auto subdir = tree.addDirectory(dir1, "subdir");
    CHECK(subdir != nullptr);
    CHECK(subdir->name == "subdir");

    auto file2 = tree.addFile(subdir, "file2.txt");
    CHECK(file2 != nullptr);
    CHECK(file2->name == "file2.txt");

    auto dir2 = tree.addDirectory(tree.getRoot(), "dir2");
    CHECK(dir2 != nullptr);
    CHECK(dir2->name == "dir2");
  }

  SUBCASE("Removing nodes") {
    auto dir1 = tree.addDirectory(tree.getRoot(), "dir1");
    tree.remove(dir1);
    CHECK(tree.getRoot()->files.empty());
    CHECK(tree.getRoot()->folders.empty());
  }
}

TEST_CASE("Testing FileTree with multiple levels") {
  FileTree tree("root");

  SUBCASE("Adding directories and files at multiple levels") {
    auto dir1 = tree.addDirectory(tree.getRoot(), "dir1");
    CHECK(dir1 != nullptr);
    CHECK(dir1->name == "dir1");

    auto file1 = tree.addFile(dir1, "file1.txt");
    CHECK(file1 != nullptr);
    CHECK(file1->name == "file1.txt");

    auto subdir1 = tree.addDirectory(dir1, "subdir1");
    CHECK(subdir1 != nullptr);
    CHECK(subdir1->name == "subdir1");

    auto file2 = tree.addFile(subdir1, "file2.txt");
    CHECK(file2 != nullptr);
    CHECK(file2->name == "file2.txt");

    auto subdir2 = tree.addDirectory(subdir1, "subdir2");
    CHECK(subdir2 != nullptr);
    CHECK(subdir2->name == "subdir2");

    auto file3 = tree.addFile(subdir2, "file3.txt");
    CHECK(file3 != nullptr);
    CHECK(file3->name == "file3.txt");
  }

  SUBCASE("Removing nodes at multiple levels") {
    auto dir1 = tree.addDirectory(tree.getRoot(), "dir1");
    auto subdir1 = tree.addDirectory(dir1, "subdir1");
    tree.remove(subdir1);
    CHECK(dir1->files.empty());
    CHECK(dir1->folders.empty());
  }
}

TEST_CASE("FileTree mkdir") {
  FileTree fileTree("root");

  SUBCASE("Create directory non-recursively") {
    auto createdDir = fileTree.mkdir("newDir", false);
    REQUIRE(createdDir != nullptr);
    CHECK(createdDir->name == "newDir");
    CHECK(createdDir->type == FileTree::Type::DIRECTORY);

    // Check that the parent directory has the child
    CHECK(fileTree.getNode("/")->folders.count("newDir") == 1);
    CHECK(fileTree.getNode("/")->files.count("newDir") == 0);
  }

  SUBCASE("Create directory recursively") {
    auto createdDir = fileTree.mkdir("newDir2/anotherDir", true);
    REQUIRE(createdDir != nullptr);
    CHECK(createdDir->name == "anotherDir");
    CHECK(createdDir->type == FileTree::Type::DIRECTORY);

    // Check that all parent directories have the child
    CHECK(fileTree.getNode("/")->folders.count("newDir2") == 1);
    CHECK(fileTree.getNode("newDir2")->folders.count("anotherDir") == 1);
  }

  SUBCASE("Create directory with existing path") {
    auto createdDir = fileTree.mkdir("existingDir", false);
    REQUIRE(createdDir != nullptr);

    CHECK_THROWS_WITH_AS(fileTree.mkdir("existingDir", false), "Directory already registered!", std::runtime_error);

    auto existingDir = fileTree.getNode("existingDir");
    CHECK(existingDir != nullptr);
    CHECK(existingDir->name == "existingDir");
    CHECK(existingDir->type == FileTree::Type::DIRECTORY);
  }
}

TEST_CASE("FileTree mkfile") {
  FileTree fileTree("root");

  SUBCASE("Create file") {
    auto createdFile = fileTree.mkfile("newFile.txt");
    REQUIRE(createdFile != nullptr);
    CHECK(createdFile->name == "newFile.txt");
    CHECK(createdFile->type == FileTree::Type::FILE);

    // Check that the parent directory has the file
    CHECK(fileTree.getNode("/")->files.count("newFile.txt") == 1);
    CHECK(fileTree.getNode("/")->folders.count("newFile.txt") == 0);
  }

  SUBCASE("Create file with existing path") {
    auto createdFile = fileTree.mkfile("existingFile.txt");
    REQUIRE(createdFile != nullptr);

    CHECK_THROWS_WITH_AS(fileTree.mkfile("existingFile.txt"), "File already registered!", std::runtime_error);

    auto existingFile = fileTree.getNode("existingFile.txt");
    CHECK(existingFile != nullptr);
    CHECK(existingFile->name == "existingFile.txt");
    CHECK(existingFile->type == FileTree::Type::FILE);
  }

  SUBCASE("Create file and get its path") {
    auto createdFile = fileTree.mkfile("newFile.txt");
    REQUIRE(createdFile != nullptr);

    CHECK(createdFile->path(true) == "root/newFile.txt");
    CHECK(createdFile->path() == "/newFile.txt");
    CHECK(createdFile->path(false) == "/newFile.txt");
  }
}

TEST_CASE("FileTree rm") {
  FileTree fileTree("root");

  SUBCASE("Remove file") {
    auto createdFile = fileTree.mkfile("toRemove.txt");
    REQUIRE(createdFile != nullptr);

    fileTree.rm("toRemove.txt");
    CHECK_THROWS_WITH_AS(fileTree.getNode("toRemove.txt"), "Node doesn't exist! (Folder)", std::runtime_error);
  }

  SUBCASE("Remove directory recursively") {
    auto createdDir = fileTree.mkdir("toRemoveDir", true);
    REQUIRE(createdDir != nullptr);

    fileTree.mkfile("toRemoveDir/file1.txt");
    fileTree.mkfile("toRemoveDir/file2.txt");

    fileTree.rm("toRemoveDir", true);
    CHECK_THROWS_WITH_AS(fileTree.getNode("toRemoveDir"), "Node doesn't exist! (Folder)", std::runtime_error);
  }

  SUBCASE("Remove non-empty directory non-recursively") {
    auto createdDir = fileTree.mkdir("nonEmptyDir", true);
    REQUIRE(createdDir != nullptr);

    fileTree.mkfile("nonEmptyDir/file1.txt");

    CHECK_THROWS_WITH_AS(fileTree.rm("nonEmptyDir", false), "Directory isn't empty!", std::runtime_error);

    auto removedDir = fileTree.getNode("nonEmptyDir");
    CHECK(removedDir != nullptr);  // Directory should not be removed non-recursively
  }

  SUBCASE("Remove non-existing path") {
    fileTree.rm("nonExistingPath", true);
    fileTree.rm("nonExistingPath/file.txt", true);

    // No crashes or unexpected behavior should occur
  }
}

TEST_CASE("getExtentsCount for folder node") {
  // Setup: Create a folder node (folders should not have extents)
  FileTree::Node folderNode("folder", FileTree::Type::DIRECTORY, nullptr);
  SUBCASE("update is true") {
    // Action
    int count = folderNode.getExtentsCount(true);
    // Assert
    CHECK(count == 0);
  }

  SUBCASE("update is false") {
    // Action
    int count = folderNode.getExtentsCount(false);

    // Assert
    CHECK(count == 0);
  }
}

// Note: Depending on your build, you may need to include additional headers.
// Make sure the paths to the filestorm headers are correct.

TEST_CASE("Basic FileTree operations") {
  FileTree tree("root");

  SUBCASE("Adding directories and files") {
    auto dir1 = tree.addDirectory(tree.getRoot(), "dir1");
    CHECK(dir1 != nullptr);
    CHECK(dir1->name == "dir1");

    auto file1 = tree.addFile(dir1, "file1.txt");
    CHECK(file1 != nullptr);
    CHECK(file1->name == "file1.txt");

    auto subdir = tree.addDirectory(dir1, "subdir");
    CHECK(subdir != nullptr);
    CHECK(subdir->name == "subdir");

    auto file2 = tree.addFile(subdir, "file2.txt");
    CHECK(file2 != nullptr);
    CHECK(file2->name == "file2.txt");

    auto dir2 = tree.addDirectory(tree.getRoot(), "dir2");
    CHECK(dir2 != nullptr);
    CHECK(dir2->name == "dir2");
  }

  SUBCASE("Removing nodes from the tree") {
    auto dir1 = tree.addDirectory(tree.getRoot(), "dir1");
    auto file1 = tree.addFile(dir1, "file1.txt");
    auto subdir = tree.addDirectory(dir1, "subdir");
    auto file2 = tree.addFile(subdir, "file2.txt");

    // Remove leaf nodes first
    tree.remove(file2);
    CHECK(dir1->folders.count("subdir") == 1);  // subdir still exists
    // Now remove subdir which is empty now
    tree.remove(subdir);
    CHECK(dir1->folders.find("subdir") == dir1->folders.end());

    // Removing an entire directory should clear it from the parent's map
    tree.remove(dir1);
    CHECK(tree.getRoot()->folders.find("dir1") == tree.getRoot()->folders.end());
  }
}

TEST_CASE("FileTree operations with multiple levels") {
  FileTree tree("root");

  SUBCASE("Adding directories and files at multiple levels") {
    auto dir1 = tree.addDirectory(tree.getRoot(), "dir1");
    CHECK(dir1 != nullptr);
    CHECK(dir1->name == "dir1");

    auto file1 = tree.addFile(dir1, "file1.txt");
    CHECK(file1 != nullptr);
    CHECK(file1->name == "file1.txt");

    auto subdir1 = tree.addDirectory(dir1, "subdir1");
    CHECK(subdir1 != nullptr);
    CHECK(subdir1->name == "subdir1");

    auto file2 = tree.addFile(subdir1, "file2.txt");
    CHECK(file2 != nullptr);
    CHECK(file2->name == "file2.txt");

    auto subdir2 = tree.addDirectory(subdir1, "subdir2");
    CHECK(subdir2 != nullptr);
    CHECK(subdir2->name == "subdir2");

    auto file3 = tree.addFile(subdir2, "file3.txt");
    CHECK(file3 != nullptr);
    CHECK(file3->name == "file3.txt");
  }

  SUBCASE("Removing nodes at multiple levels") {
    auto dir1 = tree.addDirectory(tree.getRoot(), "dir1");
    auto subdir1 = tree.addDirectory(dir1, "subdir1");
    // After removal of a subdirectory, its parent should not retain it.
    tree.remove(subdir1);
    CHECK(dir1->folders.find("subdir1") == dir1->folders.end());
  }
}

TEST_CASE("FileTree mkdir functionality") {
  FileTree fileTree("root");

  SUBCASE("Create directory non-recursively") {
    auto createdDir = fileTree.mkdir("newDir", false);
    REQUIRE(createdDir != nullptr);
    CHECK(createdDir->name == "newDir");
    CHECK(createdDir->type == FileTree::Type::DIRECTORY);

    // Check that the root now has the child directory
    auto root = fileTree.getNode("/");
    CHECK(root->folders.count("newDir") == 1);
    CHECK(root->files.count("newDir") == 0);
  }

  SUBCASE("Create directory recursively") {
    auto createdDir = fileTree.mkdir("newDir2/anotherDir", true);
    REQUIRE(createdDir != nullptr);
    CHECK(createdDir->name == "anotherDir");
    CHECK(createdDir->type == FileTree::Type::DIRECTORY);

    // Check that the parent directories exist
    CHECK(fileTree.getNode("/")->folders.count("newDir2") == 1);
    CHECK(fileTree.getNode("newDir2")->folders.count("anotherDir") == 1);
  }

  SUBCASE("Create directory with an existing path") {
    auto createdDir = fileTree.mkdir("existingDir", false);
    REQUIRE(createdDir != nullptr);

    CHECK_THROWS_WITH_AS(fileTree.mkdir("existingDir", false), "Directory already registered!", std::runtime_error);

    auto existingDir = fileTree.getNode("existingDir");
    CHECK(existingDir != nullptr);
    CHECK(existingDir->name == "existingDir");
    CHECK(existingDir->type == FileTree::Type::DIRECTORY);
  }
}

TEST_CASE("FileTree mkfile functionality") {
  FileTree fileTree("root");

  SUBCASE("Create file") {
    auto createdFile = fileTree.mkfile("newFile.txt");
    REQUIRE(createdFile != nullptr);
    CHECK(createdFile->name == "newFile.txt");
    CHECK(createdFile->type == FileTree::Type::FILE);

    // Check that the file is registered in the root directory
    auto root = fileTree.getNode("/");
    CHECK(root->files.count("newFile.txt") == 1);
    CHECK(root->folders.count("newFile.txt") == 0);
  }

  SUBCASE("Create file with an already registered path") {
    auto createdFile = fileTree.mkfile("existingFile.txt");
    REQUIRE(createdFile != nullptr);

    CHECK_THROWS_WITH_AS(fileTree.mkfile("existingFile.txt"), "File already registered!", std::runtime_error);

    auto existingFile = fileTree.getNode("existingFile.txt");
    CHECK(existingFile != nullptr);
    CHECK(existingFile->name == "existingFile.txt");
    CHECK(existingFile->type == FileTree::Type::FILE);
  }

  SUBCASE("File path generation consistency") {
    auto createdFile = fileTree.mkfile("newFile.txt");
    REQUIRE(createdFile != nullptr);
    // Assuming path(true) returns a full path starting with "root" and
    // path() or path(false) returns a relative or UNIX style path starting with "/"
    CHECK(createdFile->path(true) == "root/newFile.txt");
    CHECK(createdFile->path() == "/newFile.txt");
    CHECK(createdFile->path(false) == "/newFile.txt");
  }
}

TEST_CASE("FileTree rm (remove) functionality") {
  FileTree fileTree("root");

  SUBCASE("Remove file node") {
    auto createdFile = fileTree.mkfile("toRemove.txt");
    REQUIRE(createdFile != nullptr);

    fileTree.rm("toRemove.txt");
    CHECK_THROWS_WITH_AS(fileTree.getNode("toRemove.txt"), "Node doesn't exist! (Folder)", std::runtime_error);
  }

  SUBCASE("Remove directory recursively") {
    auto createdDir = fileTree.mkdir("toRemoveDir", true);
    REQUIRE(createdDir != nullptr);

    fileTree.mkfile("toRemoveDir/file1.txt");
    fileTree.mkfile("toRemoveDir/file2.txt");

    fileTree.rm("toRemoveDir", true);
    CHECK_THROWS_WITH_AS(fileTree.getNode("toRemoveDir"), "Node doesn't exist! (Folder)", std::runtime_error);
  }

  SUBCASE("Fail to remove non-empty directory non-recursively") {
    auto createdDir = fileTree.mkdir("nonEmptyDir", true);
    REQUIRE(createdDir != nullptr);

    fileTree.mkfile("nonEmptyDir/file1.txt");

    CHECK_THROWS_WITH_AS(fileTree.rm("nonEmptyDir", false), "Directory isn't empty!", std::runtime_error);

    // Verify directory still exists
    auto nonEmptyDir = fileTree.getNode("nonEmptyDir");
    CHECK(nonEmptyDir != nullptr);
  }

  SUBCASE("Remove non-existing path should not crash") {
    // Removal of paths not found should not throw when recursive removal is specified.
    CHECK_NOTHROW(fileTree.rm("nonExistingPath", true));
    CHECK_NOTHROW(fileTree.rm("nonExistingPath/file.txt", true));
  }
}

TEST_CASE("Testing getExtentsCount for folder node") {
  // Assuming the Node class has a method getExtentsCount(bool) and extents for a folder is 0.
  FileTree::Node folderNode("folder", FileTree::Type::DIRECTORY, nullptr);
  SUBCASE("update flag true") {
    int count = folderNode.getExtentsCount(true);
    CHECK(count == 0);
  }
  SUBCASE("update flag false") {
    int count = folderNode.getExtentsCount(false);
    CHECK(count == 0);
  }
}

TEST_CASE("Testing random file and directory functionality") {
  FileTree tree("root");

  //  SUBCASE("Random file throws exception if no files exist") { CHECK_THROWS_WITH_AS(tree.randomFile(), "No files in the tree!", std::runtime_error); }

  // TODO those test should be fixed in future.
  // SUBCASE("Random directory throws exception if no directories exist") {
  //   // The tree always has a root directory so this might not throw.
  //   auto rootDir = tree.randomDirectory();
  //   CHECK(rootDir != nullptr);
  //   CHECK(rootDir->name == "root");
  // }

  SUBCASE("Adding files allows random retrieval") {
    tree.mkfile("fileA.txt");
    auto randomFile = tree.randomFile();
    CHECK(randomFile != nullptr);
    // The file name must be either the one added or following naming conventions from mkfile.
    CHECK((randomFile->name == "fileA.txt" || randomFile->name.substr(0, 5) == "file_"));
  }
}

TEST_CASE("Testing directory walk functions") {
  FileTree tree("root");

  // Create a simple tree structure:
  auto dirA = tree.mkdir("dirA", false);
  auto dirB = tree.mkdir("dirB", false);
  tree.mkfile("dirA/file1.txt");
  tree.mkfile("dirB/file2.txt");
  tree.mkdir("dirA/subdirA", false);
  tree.mkfile("dirA/subdirA/file3.txt");

  SUBCASE("Leaf directory walk should invoke function for every leaf") {
    std::stringstream ss;
    tree.leafDirWalk([&ss](FileTree::Nodeptr node) { ss << node->name << " "; });
    std::string result = ss.str();
    // Check that expected leaf nodes are processed (they do not have subdirectories)
    CHECK(result.find("dirB") != std::string::npos);
    CHECK(result.find("subdirA") != std::string::npos);
  }

  SUBCASE("Bottom-up directory walk should process nodes in subtrees before parents") {
    std::vector<std::string> walkNames;
    tree.bottomUpDirWalk(tree.getRoot(), [&walkNames](FileTree::Nodeptr node) { walkNames.push_back(node->name); });
    // Since root should not be processed and leaf nodes are processed first,
    // check that subdirectories appear in the walk.
    CHECK(std::find(walkNames.begin(), walkNames.end(), "subdirA") != walkNames.end());
    CHECK(std::find(walkNames.begin(), walkNames.end(), "dirA") != walkNames.end());
  }
}

TEST_CASE("Testing punchable file functions") {
  FileTree tree("root");

  SUBCASE("Random punchable file throws exception if none exist") { CHECK_THROWS_WITH_AS(tree.randomPunchableFile(), "No punchable files in the tree!", std::runtime_error); }

  SUBCASE("Adding files populates punchable files and removal works") {
    auto fileNode = tree.mkfile("punchFile.txt");
    CHECK(tree.hasPunchableFiles());
    auto randomPunch = tree.randomPunchableFile();
    CHECK(randomPunch != nullptr);
    // Remove the file from punchable list and verify it is removed.
    tree.removeFromPunchableFiles(fileNode);
    CHECK_FALSE(tree.hasPunchableFiles());
  }
}
