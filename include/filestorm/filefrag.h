#include <fcntl.h>
#if defined(__linux__)
#  include <linux/fiemap.h>
#  include <linux/fs.h>
#else
#  warning "The file fragments code is for Linux only and the fragmentation monitorning wont be available on other platforms."
#endif
#include <spdlog/spdlog.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <iostream>
#include <vector>

#define MAX_EXTENTS 32

struct extents {
  uint64_t start;
  uint64_t length;
  uint64_t flags;
};

std::vector<extents> get_extents(const char *file_path);