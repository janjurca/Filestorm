#include <fcntl.h>     // for open
#include <sys/stat.h>  // for S_IRWXU
#include <unistd.h>    // for close

#include <algorithm>
#include <cerrno>  // for errno
#include <chrono>
#include <cstring>  // for strerror
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iostream>  // for std::cerr
#include <memory>    // for std::unique_ptr
#include <random>
#include <string_view>

void write(const char* filename, int size) {
  int flags = O_RDWR | O_CREAT | O_DIRECT;
  int fd = open(filename, flags, S_IRWXU);
  char* buffer_64k = new char[64 * 1024];
  std::chrono::time_point<std::chrono::system_clock> start, end;
  start = std::chrono::system_clock::now();
  ssize_t ret = 0;
  for (int i = 0; i < size * 1024 * 1024 / 64; i++) {
    ret += write(fd, buffer_64k, 64 * 1024);
  }
  end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = end - start;
  std::cout << "Time taken to write " << ret / 1024 / 1024 << "MB of data: " << elapsed_seconds.count() << "s | speed in Mb/s: " << (ret / 1024 / 1024) / elapsed_seconds.count() << std::endl;
  close(fd);
}
void punch_hole(const char* filename, long offset_gb, long len_gb) {
  int fd = open(filename, O_RDWR);
  std::chrono::time_point<std::chrono::system_clock> start, end;
  start = std::chrono::system_clock::now();
  // ofest of hole is 1GB and size of hole is 6GB
  off_t offset = offset_gb * 1024 * 1024 * 1024;
  off_t len = len_gb * 1024 * 1024 * 1024;
  fallocate(fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE, offset, len);
  end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = end - start;
  std::cout << "Time taken to punch hole of 2GB in file: " << elapsed_seconds.count() << "s" << std::endl;
  close(fd);
}

int main(int argc, const char** argv) {
  int start_size_gb = 50;

  std::string base_dir(argv[1]);
  for (int i = 0; i < 10; i++) {
    std::string filename = base_dir + "/file_" + std::to_string(i);
    write(filename.c_str(), start_size_gb);
    punch_hole(filename.c_str(), 1, start_size_gb - 1);
    start_size_gb -= 2;
  }
  return 0;
}