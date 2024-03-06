#include <fcntl.h>
#include <linux/fiemap.h>
#include <linux/fs.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <iostream>

#define MAX_EXTENTS 32

struct fiemap_extent_data {
  struct fiemap *fiemap_ptr;
};

// Function to print the extents of a file
void print_file_extents(const char *file_path) {
  int fd = open(file_path, O_RDONLY);
  if (fd < 0) {
    perror("open");
    return;
  }

  struct fiemap_extent_data fed;
  memset(&fed, 0, sizeof(fed));

  size_t fiemap_size = sizeof(struct fiemap) + sizeof(struct fiemap_extent) * MAX_EXTENTS;
  fed.fiemap_ptr = (struct fiemap *)malloc(fiemap_size);
  if (fed.fiemap_ptr == NULL) {
    perror("malloc");
    close(fd);
    return;
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
    return;
  }

  std::cout << "Number of extents: " << fed.fiemap_ptr->fm_mapped_extents << std::endl;
  for (unsigned int i = 0; i < fed.fiemap_ptr->fm_mapped_extents; i++) {
    std::cout << "Extent " << i << ": Start=" << fed.fiemap_ptr->fm_extents[i].fe_logical << ", Length=" << fed.fiemap_ptr->fm_extents[i].fe_length
              << ", Flags=" << fed.fiemap_ptr->fm_extents[i].fe_flags << std::endl;
  }

  free(fed.fiemap_ptr);
  close(fd);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <file_path>" << std::endl;
    return 1;
  }

  print_file_extents(argv[1]);
  return 0;
}
