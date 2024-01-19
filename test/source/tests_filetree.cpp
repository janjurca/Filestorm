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
}

TEST_CASE("FileTree rm") {
  FileTree fileTree("root");

  SUBCASE("Remove file") {
    auto createdFile = fileTree.mkfile("toRemove.txt");
    REQUIRE(createdFile != nullptr);

    fileTree.rm("toRemove.txt");
    CHECK_THROWS_WITH_AS(fileTree.getNode("toRemove.txt"), "Node doesn't exist!", std::runtime_error);
  }

  SUBCASE("Remove directory recursively") {
    auto createdDir = fileTree.mkdir("toRemoveDir", true);
    REQUIRE(createdDir != nullptr);

    fileTree.mkfile("toRemoveDir/file1.txt");
    fileTree.mkfile("toRemoveDir/file2.txt");

    fileTree.rm("toRemoveDir", true);
    CHECK_THROWS_WITH_AS(fileTree.getNode("toRemoveDir"), "Node doesn't exist!", std::runtime_error);
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
