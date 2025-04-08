#include <filestorm/filefrag.h>
#include <filestorm/utils/logger.h>
#include <fmt/core.h>

struct fiemap_extent_data {
  struct fiemap *fiemap_ptr;
};

std::vector<extents> get_extents(const char *file_path) {
  // Check for null file path
  if (file_path == nullptr) {
    throw std::runtime_error("File path is null.");
  }

  // Check if the file exists
  if (access(file_path, F_OK) == -1) {
    throw std::runtime_error(fmt::format("File [{}] does not exist.", file_path));
  }

#if defined(__linux__)
  std::vector<extents> extents_list;

  // Open the file read-only to query its extents
  int fd = open(file_path, O_RDONLY);
  if (fd < 0) {
    perror("open");
    throw std::runtime_error(fmt::format("Error opening file [{}], for extents scan.", file_path));
  }

  // Allocate enough memory for a fiemap struct plus MAX_EXTENTS fiemap_extents
  const size_t fiemap_size = sizeof(struct fiemap) + sizeof(struct fiemap_extent) * MAX_EXTENTS;
  struct fiemap *fiemap_ptr = (struct fiemap *)malloc(fiemap_size);
  if (fiemap_ptr == NULL) {
    perror("malloc");
    close(fd);
    throw std::runtime_error("Error allocating memory for fiemap.");
  }

  uint64_t start = 0;  // Start offset for fiemap scan
  bool done = false;   // Loop control flag

  // Loop until we've retrieved all extents
  while (!done) {
    // Clear fiemap memory before each ioctl call
    memset(fiemap_ptr, 0, fiemap_size);

    // Set up fiemap request parameters
    fiemap_ptr->fm_start = start;               // Start from the current offset
    fiemap_ptr->fm_length = ~0ULL;              // Request all extents from 'start' onward
    fiemap_ptr->fm_flags = FIEMAP_FLAG_SYNC;    // Make sure file is synced to disk before scanning
    fiemap_ptr->fm_extent_count = MAX_EXTENTS;  // Maximum number of extents we can receive

    // Perform the ioctl call to get the file extents
    if (ioctl(fd, FS_IOC_FIEMAP, fiemap_ptr) == -1) {
      perror("ioctl");
      free(fiemap_ptr);
      close(fd);
      throw std::runtime_error("Error getting file extents.");
    }

    // If no extents were mapped, we're done
    if (fiemap_ptr->fm_mapped_extents == 0) {
      break;
    }

    // Loop through the mapped extents and store them
    for (unsigned int i = 0; i < fiemap_ptr->fm_mapped_extents; i++) {
      auto &fe = fiemap_ptr->fm_extents[i];
      extents_list.push_back({
          fe.fe_logical,  // Logical offset within file
          fe.fe_length,   // Length of the extent
          fe.fe_flags     // Flags describing the extent (e.g., if it's shared, last, etc.)
      });
    }

    // Check if the last returned extent marks the end of the file
    auto &last_extent = fiemap_ptr->fm_extents[fiemap_ptr->fm_mapped_extents - 1];
    if (last_extent.fe_flags & FIEMAP_EXTENT_LAST) {
      done = true;  // We're done
    } else {
      // Update start offset to scan from the next byte after the last extent
      start = last_extent.fe_logical + last_extent.fe_length;
    }
  }

  // Free allocated memory and close the file descriptor
  free(fiemap_ptr);
  close(fd);

  // Return the collected extents
  return extents_list;

#else
  // If not on Linux, log a warning once and return an empty list
  static bool warning_printed = false;
  if (!warning_printed) {
    logger.warn("This code is for Linux only and the fragmentation monitoring won't be available on other platforms. {}", file_path);
    warning_printed = true;
  }
  return {};
#endif
}
