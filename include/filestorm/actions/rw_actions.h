#include <errno.h>
#include <fcntl.h>
#include <filestorm/actions/actions.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <random>
#include <string>
#include <vector>
// ABOUT DIRECT IO https://github.com/facebook/rocksdb/wiki/Direct-IO

class ReadAction : public VirtualMonitoredAction, public FileActionAttributes {
private:
  std::string m_file_path;
  size_t m_block_size;
  long long m_read_bytes = 0;

public:
  ReadAction(std::chrono::milliseconds monitoring_interval,
             std::function<void(VirtualMonitoredAction*)> on_log,
             FileActionAttributes file_strategy)
      : m_file_path(file_path),
        m_block_size(block_size),
        VirtualMonitoredAction(monitoring_interval, on_log),
        FileActionAttributes(file_strategy) {}

  void work() override {
    std::ifstream file(m_file_path);
    char line[m_block_size];
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
        file.read(line, m_block_size);
        m_read_bytes += m_block_size;
        now_time = std::chrono::high_resolution_clock::now();
      }
    } else {
      while (file.read(line, m_block_size)) {
        m_read_bytes += m_block_size;
      }
    }
  }

  void log_values() override {
    std::lock_guard<std::mutex> lock(m_monitoredDataMutex);
    if (m_monitoredData.find("read_bytes") == m_monitoredData.end()) {
      m_monitoredData["read_bytes"] = std::vector<float>();
    }
    m_monitoredData["read_bytes"].push_back(m_read_bytes);
  }
};

class WriteAction : public VirtualMonitoredAction, public FileActionAttributes {
private:
  std::string m_file_path;
  size_t m_block_size;
  size_t m_file_size;
  long long m_written_bytes = 0;

public:
  WriteAction(std::chrono::milliseconds monitoring_interval,
              std::function<void(VirtualMonitoredAction*)> on_log,
              FileActionAttributes file_strategy)
      : m_file_path(file_path),
        m_block_size(block_size),
        m_file_size(file_size),
        VirtualMonitoredAction(monitoring_interval, on_log),
        FileActionAttributes(file_strategy) {}

  inline void generate_random_chunk(char* chunk, size_t size) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 255);

    std::generate_n(chunk, size, []() { return dis(gen); });
  }

  void work() override {
    int fd;
    char line[m_block_size];

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  error "Windows is not supported"
#elif __APPLE__
    fd = open(m_file_path.c_str(), O_RDWR | O_CREAT, S_IRWXU);
    if (fcntl(fd, F_NOCACHE, 1) == -1 && fd != -1) {
      close(fd);
      throw std::runtime_error("WriteAction::work: error on fcntl F_NOCACHE");
    }
#elif __linux__ || __unix__ || defined(_POSIX_VERSION)
    fd = open(m_file_path.c_str(), O_DIRECT | O_RDWR | O_CREAT, S_IRWXU);
#else
#  warning "Unknown system"
    fd = open(m_file_path.c_str(), O_DIRECT | O_RDWR | O_CREAT, S_IRWXU);
#endif

    if (fd == -1) {
      throw std::runtime_error("WriteAction::work: error opening file");
    }

    if (is_time_based()) {
      auto start_time = std::chrono::high_resolution_clock::now();
      auto now_time = std::chrono::high_resolution_clock::now();

      while (std::chrono::duration_cast<std::chrono::milliseconds>(now_time - start_time).count()
             <= get_interval().count()) {
        generate_random_chunk(line, m_block_size);
        write(fd, line, m_block_size);
        m_written_bytes += m_block_size;
        now_time = std::chrono::high_resolution_clock::now();
      }
    } else {
      while (m_written_bytes < m_file_size) {
        generate_random_chunk(line, m_block_size);
        write(fd, line, m_block_size);
        m_written_bytes += m_block_size;
      }
    }
    close(fd);
  }

  void log_values() override {
    std::lock_guard<std::mutex> lock(m_monitoredDataMutex);
    if (m_monitoredData.find("written_bytes") == m_monitoredData.end()) {
      m_monitoredData["written_bytes"] = std::vector<float>();
    }
    m_monitoredData["written_bytes"].push_back(m_written_bytes);
  }
};