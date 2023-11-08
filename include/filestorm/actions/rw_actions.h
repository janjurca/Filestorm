#include <errno.h>
#include <fcntl.h>
#include <filestorm/actions/actions.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#include <string>

class ReadAction : public VirtualMonitoredAction, public FileActionStrategy {
private:
  std::string m_file_path;
  size_t m_block_size;
  long long m_read_bytes = 0;

public:
  ReadAction(std::string file_path, size_t block_size,
             std::chrono::milliseconds monitoring_interval,
             std::function<void(VirtualMonitoredAction*)> on_log, bool time_based)
      : m_file_path(file_path),
        m_block_size(block_size),
        VirtualMonitoredAction(monitoring_interval, on_log),
        FileActionStrategy(time_based) {}

  void work() override {
    std::ifstream file(m_file_path);
    std::string line;
    // read file by blocks in size of block size
    while (file.read(&line, m_block_size)) {
      m_read_bytes += m_block_size;
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

class WriteAction : public VirtualMonitoredAction, public FileActionStrategy {
private:
  std::string m_file_path;
  size_t m_block_size;
  size_t m_file_size;
  long long m_written_bytes = 0;

public:
  WriteAction(std::string file_path, size_t block_size, size_t file_size,
              std::chrono::milliseconds monitoring_interval,
              std::function<void(VirtualMonitoredAction*)> on_log, bool time_based)
      : m_file_path(file_path),
        m_block_size(block_size),
        m_file_size(file_size),
        VirtualMonitoredAction(monitoring_interval, on_log),
        FileActionStrategy(time_based) {}

  void work() override {
    int fd;
    if ((fd = open(m_file_path.c_str(), O_DIRECT | O_RDWR | O_CREAT, S_IRWXU)) == -1) {
      throw std::runtime_error("WriteAction::work: error opening file");
    }

    write(fd, message, 64);
    close(fd);
    while (file.write(&line, m_block_size)) {
      m_written_bytes += m_block_size;
    }
  }

  void log_values() override {
    std::lock_guard<std::mutex> lock(m_monitoredDataMutex);
    if (m_monitoredData.find("written_bytes") == m_monitoredData.end()) {
      m_monitoredData["written_bytes"] = std::vector<float>();
    }
    m_monitoredData["written_bytes"].push_back(m_written_bytes);
  }
};