#include <errno.h>
#include <fcntl.h>
#include <filestorm/actions/actions.h>
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#if __linux__ || __unix__ || defined(_POSIX_VERSION) || __APPLE__
#  include <unistd.h>
#else
#  error "Unknown system"
#endif

#include <fcntl.h>     // for open
#include <sys/stat.h>  // for S_IRWXU
#include <unistd.h>    // for close

#include <algorithm>
#include <cerrno>  // for errno
#include <chrono>
#include <cstring>  // for strerror
#include <fstream>
#include <iostream>  // for std::cerr
#include <memory>    // for std::unique_ptr
#include <random>
#include <string>
#include <vector>
// ABOUT DIRECT IO https://github.com/facebook/rocksdb/wiki/Direct-IO

class ReadAction : public VirtualMonitoredAction, public FileActionAttributes {
protected:
  u_int64_t m_read_bytes = 0;

  inline off_t get_offset();

public:
  ReadAction(std::chrono::milliseconds monitoring_interval,
             std::function<void(VirtualMonitoredAction*)> on_log,
             FileActionAttributes file_attributes);

  void work() override;
  void log_values() override;
};
class RandomReadAction : public ReadAction {
private:
  std::vector<off_t> m_offsets;

  inline off_t get_offset();
};

class WriteAction : public VirtualMonitoredAction, public FileActionAttributes {
protected:
  u_int64_t m_written_bytes = 0;

  inline off_t get_offset();
  inline void generate_random_chunk(char* chunk, size_t size);

public:
  WriteAction(std::chrono::milliseconds monitoring_interval,
              std::function<void(VirtualMonitoredAction*)> on_log,
              FileActionAttributes file_attributes);

  void work() override;

  void log_values() override;
};

class RandomWriteAction : public WriteAction {
private:
  std::vector<off_t> m_offsets;

  inline off_t get_offset();
};