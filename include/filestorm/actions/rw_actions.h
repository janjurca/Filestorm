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

#include <algorithm>
#include <chrono>
#include <fstream>
#include <memory>  // for std::unique_ptr
#include <random>
#include <string>
#include <vector>
// ABOUT DIRECT IO https://github.com/facebook/rocksdb/wiki/Direct-IO

class ReadAction : public VirtualMonitoredAction, public FileActionAttributes {
protected:
  long long m_read_bytes = 0;

  inline off_t get_offset() {
    static off_t offset = 0;
    static off_t block_size_bytes = get_block_size().convert<DataUnit::B>().get_value();
    static ssize_t file_size_bytes = get_file_size().convert<DataUnit::B>().get_value();

    offset += block_size_bytes;
    if (m_read_bytes % file_size_bytes == 0) {
      offset = 0;
    }
    return offset;
  }

public:
  ReadAction(std::chrono::milliseconds monitoring_interval,
             std::function<void(VirtualMonitoredAction*)> on_log,
             FileActionAttributes file_attributes)
      : VirtualMonitoredAction(monitoring_interval, on_log),
        FileActionAttributes(file_attributes) {}

  void work() override {
    spdlog::debug("{}::work: file_path: {}", typeid(*this).name(), get_file_path());
    int fd;
    std::unique_ptr<char[]> line(new char[get_block_size().convert<DataUnit::B>().get_value()]);

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  error "Windows is not supported"
#elif __APPLE__
    fd = open(get_file_path().c_str(), O_RDWR | O_CREAT, S_IRWXU);
    if (fcntl(fd, F_NOCACHE, 1) == -1 && fd != -1) {
      close(fd);
      throw std::runtime_error(fmt::format("{}::work: error on fcntl F_NOCACHE: {}",
                                           typeid(*this).name(), strerror(errno)));
    }
#elif __linux__ || __unix__ || defined(_POSIX_VERSION)
    fd = open(get_file_path().c_str(), O_DIRECT | O_RDWR | O_CREAT, S_IRWXU);
#else
#  warning "Unknown system"
    fd = open(get_file_path().c_str(), O_DIRECT | O_RDWR | O_CREAT, S_IRWXU);
#endif

    if (fd == -1) {
      throw std::runtime_error(
          fmt::format("{}::work: error opening file: {}", typeid(*this).name(), strerror(errno)));
    }
    size_t offset = 0;
    spdlog::debug("{}::work: file_size: {}", typeid(*this).name(), get_file_size());

    if (is_time_based()) {
      auto start_time = std::chrono::high_resolution_clock::now();
      auto now_time = std::chrono::high_resolution_clock::now();
      spdlog::debug("{}::work: start_time: {}", typeid(*this).name(),
                    start_time.time_since_epoch().count());
      spdlog::debug(
          "{}::work: interval: {}", typeid(*this).name(),
          std::chrono::duration_cast<std::chrono::milliseconds>(get_time_limit()).count());

      auto data = line.get();
      size_t block_size = get_block_size().convert<DataUnit::B>().get_value();

      while (std::chrono::duration_cast<std::chrono::milliseconds>(now_time - start_time).count()
             <= get_time_limit().count()) {
        m_read_bytes += pread(fd, data, block_size, get_offset());
      }
    } else {
      size_t block_size = get_block_size().convert<DataUnit::B>().get_value();
      while (m_read_bytes < get_file_size().convert<DataUnit::B>().get_value()) {
        m_read_bytes += pread(fd, line.get(), block_size, get_offset());
      }
    }
    close(fd);
  }

  void log_values() override { addMonitoredData("read_bytes", m_read_bytes); }
};

class RandomReadAction : public ReadAction {
private:
  std::vector<off_t> m_offsets;

  inline off_t get_offset() {
    static off_t offset = 0;
    static off_t block_size_bytes = get_block_size().convert<DataUnit::B>().get_value();
    static ssize_t file_size_bytes = get_file_size().convert<DataUnit::B>().get_value();

    // Calculate the total number of blocks in the file
    ssize_t num_blocks = file_size_bytes / block_size_bytes;

    // Use a random number generator to select a block index
    std::random_device rd;   // Seed
    std::mt19937 gen(rd());  // Standard mersenne_twister_engine
    std::uniform_int_distribution<> distrib(0, num_blocks - 1);

    // Generate a random block index
    ssize_t block_index = distrib(gen);

    // Calculate and return the offset
    return block_index * block_size_bytes;
  }
};

class WriteAction : public VirtualMonitoredAction, public FileActionAttributes {
protected:
  long long m_written_bytes = 0;

  inline off_t get_offset() {
    static off_t offset = 0;
    static off_t block_size_bytes = get_block_size().convert<DataUnit::B>().get_value();
    static ssize_t file_size_bytes = get_file_size().convert<DataUnit::B>().get_value();

    offset += block_size_bytes;
    if (m_written_bytes % file_size_bytes == 0) {
      offset = 0;
    }
    return offset;
  }
  inline void generate_random_chunk(char* chunk, size_t size) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 255);

    std::generate_n(chunk, size, []() { return dis(gen); });
  }

public:
  WriteAction(std::chrono::milliseconds monitoring_interval,
              std::function<void(VirtualMonitoredAction*)> on_log,
              FileActionAttributes file_attributes)
      : VirtualMonitoredAction(monitoring_interval, on_log),
        FileActionAttributes(file_attributes) {}

  inline void work() override {
    spdlog::debug("{}::work: file_path: {}", typeid(*this).name(), get_file_path());
    int fd;
    std::unique_ptr<char[]> line(new char[get_block_size().convert<DataUnit::B>().get_value()]);

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
    spdlog::debug("{}::work: file_size: {}", typeid(*this).name(), get_file_size());
    generate_random_chunk(line.get(), get_block_size().convert<DataUnit::B>().get_value());
    if (is_time_based()) {
      auto start_time = std::chrono::high_resolution_clock::now();
      auto now_time = std::chrono::high_resolution_clock::now();
      spdlog::debug("{}::work: start_time: {}", typeid(*this).name(),
                    start_time.time_since_epoch().count());
      spdlog::debug(
          "{}::work: interval: {}", typeid(*this).name(),
          std::chrono::duration_cast<std::chrono::milliseconds>(get_time_limit()).count());

      auto data = line.get();
      size_t block_size = get_block_size().convert<DataUnit::B>().get_value();

      while (std::chrono::duration_cast<std::chrono::milliseconds>(now_time - start_time).count()
             <= get_time_limit().count()) {
        m_written_bytes += pwrite(fd, data, block_size, get_offset());
      }
    } else {
      size_t block_size = get_block_size().convert<DataUnit::B>().get_value();

      while (m_written_bytes < get_file_size().convert<DataUnit::B>().get_value()) {
        generate_random_chunk(line.get(), block_size);
        m_written_bytes += write(fd, line.get(), block_size);
      }
    }
    close(fd);
  }

  void log_values() override { addMonitoredData("write_bytes", m_written_bytes); }
};

class RandomWriteAction : public WriteAction {
private:
  std::vector<off_t> m_offsets;

  inline off_t get_offset() {
    static off_t offset = 0;
    static off_t block_size_bytes = get_block_size().convert<DataUnit::B>().get_value();
    static ssize_t file_size_bytes = get_file_size().convert<DataUnit::B>().get_value();

    // Calculate the total number of blocks in the file
    ssize_t num_blocks = file_size_bytes / block_size_bytes;

    // Use a random number generator to select a block index
    std::random_device rd;   // Seed
    std::mt19937 gen(rd());  // Standard mersenne_twister_engine
    std::uniform_int_distribution<> distrib(0, num_blocks - 1);

    // Generate a random block index
    ssize_t block_index = distrib(gen);

    // Calculate and return the offset
    return block_index * block_size_bytes;
  }
};