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
    CHECK(tree.getRoot()->children.empty());
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
    CHECK(dir1->children.empty());
  }
}