#include <filestorm/filefrag.h>
#include <fmt/core.h>
struct fiemap_extent_data {
  struct fiemap *fiemap_ptr;
};

std::vector<extents> get_extents(const char *file_path) {
#if defined(__linux__)
  std::vector<extents> extents_list;
  int fd = open(file_path, O_RDONLY);
  if (fd < 0) {
    perror("open");
    throw std::runtime_error(fmt::format("Error opening file [{}], for extents scan.", file_path));
  }

  struct fiemap_extent_data fed;
  memset(&fed, 0, sizeof(fed));

  size_t fiemap_size = sizeof(struct fiemap) + sizeof(struct fiemap_extent) * MAX_EXTENTS;
  fed.fiemap_ptr = (struct fiemap *)malloc(fiemap_size);
  if (fed.fiemap_ptr == NULL) {
    perror("malloc");
    close(fd);
    throw std::runtime_error("Error allocating memory for fiemap.");
  }
  memset(fed.fiemap_ptr, 0, fiemap_size);

  fed.fiemap_ptr->fm_start = 0;
  fed.fiemap_ptr->fm_length = ~0ULL;  // Request all extents from the start
  fed.fiemap_ptr->fm_flags = FIEMAP_FLAG_SYNC;
  fed.fiemap_ptr->fm_extent_count = MAX_EXTENTS;

  if (ioctl(fd, FS_IOC_FIEMAP, fed.fiemap_ptr) == -1) {
    perror("ioctl");
    free(fed.fiemap_ptr);
    close(fd);
    throw std::runtime_error("Error getting file extents.");
  }

  for (unsigned int i = 0; i < fed.fiemap_ptr->fm_mapped_extents; i++) {
    extents_list.push_back({fed.fiemap_ptr->fm_extents[i].fe_logical, fed.fiemap_ptr->fm_extents[i].fe_length, fed.fiemap_ptr->fm_extents[i].fe_flags});
  }

  free(fed.fiemap_ptr);
  close(fd);
  return extents_list;
#else
  static bool warning_printed = false;
  if (!warning_printed) {
    spdlog::debug("This code is for Linux only and the fragmentation monitorning wont be available on other platforms. {}", file_path);
    warning_printed = true;
  }
  return {};
#endif
}
