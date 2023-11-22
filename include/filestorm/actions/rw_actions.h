#include <errno.h>
#include <fcntl.h>
#include <filestorm/actions/actions.h>
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <memory>  // for std::unique_ptr
#include <random>
#include <string>
#include <vector>
// ABOUT DIRECT IO https://github.com/facebook/rocksdb/wiki/Direct-IO

class ReadAction : public VirtualMonitoredAction, public FileActionAttributes {
private:
  long long m_read_bytes = 0;

public:
  ReadAction(std::chrono::milliseconds monitoring_interval,
             std::function<void(VirtualMonitoredAction*)> on_log,
             FileActionAttributes file_attributes)
      : VirtualMonitoredAction(monitoring_interval, on_log),
        FileActionAttributes(file_attributes) {}

  void work() override {
    std::ifstream file(get_file_path());
    // allocate array if bytes on heap c++ way
    // char line[get_block_size()];
    std::unique_ptr<char[]> line(new char[get_block_size()]);

    // read file by blocks in size of block size
    if (!file.is_open()) {
      throw std::runtime_error("ReadAction::work: error opening file");
    }
    if (is_time_based()) {
      auto start_time = std::chrono::high_resolution_clock::now();
      auto now_time = std::chrono::high_resolution_clock::now();

      while (std::chrono::duration_cast<std::chrono::milliseconds>(now_time - start_time).count()
             <= get_interval().count()) {
        if (file.eof()) {
          file.clear();
          file.seekg(0, std::ios::beg);
        }
        file.read(line.get(), get_block_size());
        m_read_bytes += get_block_size();
        now_time = std::chrono::high_resolution_clock::now();
      }
    } else {
      while (file.read(line.get(), get_block_size())) {
        m_read_bytes += get_block_size();
      }
    }
  }

  void log_values() override { addMonitoredData("read_bytes", m_read_bytes); }
};

class WriteAction : public VirtualMonitoredAction, public FileActionAttributes {
private:
  long long m_written_bytes = 0;

public:
  WriteAction(std::chrono::milliseconds monitoring_interval,
              std::function<void(VirtualMonitoredAction*)> on_log,
              FileActionAttributes file_attributes)
      : VirtualMonitoredAction(monitoring_interval, on_log),
        FileActionAttributes(file_attributes) {}

  inline void generate_random_chunk(char* chunk, size_t size) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 255);

    std::generate_n(chunk, size, []() { return dis(gen); });
  }

  void work() override {
    spdlog::debug("WriteAction::work: file_path: {}", get_file_path());
    int fd;
    std::unique_ptr<char[]> line(new char[get_block_size()]);

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  error "Windows is not supported"
#elif __APPLE__
    fd = open(get_file_path().c_str(), O_RDWR | O_CREAT, S_IRWXU);
    if (fcntl(fd, F_NOCACHE, 1) == -1 && fd != -1) {
      close(fd);
      throw std::runtime_error("WriteAction::work: error on fcntl F_NOCACHE");
    }
#elif __linux__ || __unix__ || defined(_POSIX_VERSION)
    fd = open(get_file_path().c_str(), O_DIRECT | O_RDWR | O_CREAT, S_IRWXU);
#else
#  warning "Unknown system"
    fd = open(get_file_path().c_str(), O_DIRECT | O_RDWR | O_CREAT, S_IRWXU);
#endif

    if (fd == -1) {
      throw std::runtime_error("WriteAction::work: error opening file");
    }
    size_t offset = 0;
    spdlog::debug("WriteAction::work: file_size: {}", get_file_size());
    generate_random_chunk(line.get(), get_block_size());
    if (is_time_based()) {
      auto start_time = std::chrono::high_resolution_clock::now();
      auto now_time = std::chrono::high_resolution_clock::now();
      spdlog::debug("WriteAction::work: start_time: {}", start_time.time_since_epoch().count());
      spdlog::debug("WriteAction::work: now_time: {}", now_time.time_since_epoch().count());
      spdlog::debug(
          "WriteAction::work: interval: {}",
          std::chrono::duration_cast<std::chrono::milliseconds>(get_time_limit()).count());

      auto data = line.get();
      auto block_size = get_block_size();

      while (std::chrono::duration_cast<std::chrono::milliseconds>(now_time - start_time).count()
             <= get_time_limit().count()) {
        // write(fd, line.get(), get_block_size());
        m_written_bytes += write(fd, data, block_size);
        // now_time = std::chrono::high_resolution_clock::now();
        if (m_written_bytes % get_file_size() == 0) {
          if (lseek(fd, offset, SEEK_SET) != 0) {
            throw std::runtime_error("WriteAction::work: error on fseek");
          }
        }
      }
    } else {
      while (m_written_bytes < get_file_size()) {
        generate_random_chunk(line.get(), get_block_size());
        write(fd, line.get(), get_block_size());
        m_written_bytes += get_block_size();
      }
    }
    close(fd);
  }

  void log_values() override { addMonitoredData("write_bytes", m_written_bytes); }
};