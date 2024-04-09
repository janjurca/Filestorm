#include <filestorm/actions/rw_actions.h>

inline off_t ReadMonitoredAction::get_offset() {
  static off_t offset = 0;
  static off_t block_size_bytes = get_block_size().convert<DataUnit::B>().get_value();
  static ssize_t file_size_bytes = get_file_size().convert<DataUnit::B>().get_value();

  offset += block_size_bytes;
  if (m_read_bytes % file_size_bytes == 0) {
    offset = 0;
  }
  return offset;
}

ReadMonitoredAction::ReadMonitoredAction(std::chrono::milliseconds monitoring_interval, std::function<void(VirtualMonitoredAction*)> on_log, FileActionAttributes file_attributes)
    //: VirtualMonitoredAction(monitoring_interval, on_log), FileActionAttributes(file_attributes) {}
    : RWAction(monitoring_interval, on_log, file_attributes) {
  prewrite();
}

void ReadMonitoredAction::work() {
  spdlog::debug("{}::work: file_path: {}", typeid(*this).name(), get_file_path());
  int fd;
  std::unique_ptr<char[]> line(new char[get_block_size().convert<DataUnit::B>().get_value()]);

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  error "Windows is not supported"
#elif __APPLE__
  fd = open(get_file_path().c_str(), O_RDWR | O_CREAT, S_IRWXU);
  if (fcntl(fd, F_NOCACHE, 1) == -1 && fd != -1) {
    close(fd);
    throw std::runtime_error(fmt::format("{}::work: error on fcntl F_NOCACHE: {}", typeid(*this).name(), strerror(errno)));
  }
#elif __linux__ || __unix__ || defined(_POSIX_VERSION)
  fd = open(get_file_path().c_str(), O_DIRECT | O_RDWR | O_CREAT, S_IRWXU);
#else
#  warning "Unknown system"
  fd = open(get_file_path().c_str(), O_DIRECT | O_RDWR | O_CREAT, S_IRWXU);
#endif

  if (fd == -1) {
    throw std::runtime_error(fmt::format("{}::work: error opening file: {}", typeid(*this).name(), strerror(errno)));
  }
  spdlog::debug("{}::work: file_size: {}", typeid(*this).name(), get_file_size());

  if (is_time_based()) {
    auto start_time = std::chrono::high_resolution_clock::now();
    auto now_time = std::chrono::high_resolution_clock::now();
    spdlog::debug("{}::work: start_time: {}", typeid(*this).name(), start_time.time_since_epoch().count());
    spdlog::debug("{}::work: interval: {}", typeid(*this).name(), std::chrono::duration_cast<std::chrono::milliseconds>(get_time_limit()).count());

    auto data = line.get();
    size_t block_size = get_block_size().convert<DataUnit::B>().get_value();

    while (std::chrono::duration_cast<std::chrono::milliseconds>(now_time - start_time).count() <= get_time_limit().count()) {
      m_read_bytes += pread(fd, data, block_size, get_offset());
      now_time = std::chrono::high_resolution_clock::now();
    }
  } else {
    size_t block_size = get_block_size().convert<DataUnit::B>().get_value();
    while (m_read_bytes < get_file_size().convert<DataUnit::B>().get_value()) {
      m_read_bytes += pread(fd, line.get(), block_size, get_offset());
    }
  }
  close(fd);
}

void ReadMonitoredAction::prewrite() {
  int fd = open(get_file_path().c_str(), O_RDWR | O_CREAT, S_IRWXU);
  if (fd == -1) {
    throw std::runtime_error(fmt::format("{}::prewrite: error opening file: {}", typeid(*this).name(), strerror(errno)));
  }
  u_int64_t m_written_bytes = 0;
  spdlog::debug("{}::prewrite: file_size: {}", typeid(*this).name(), get_file_size());
  u_int64_t file_size_bytes = get_file_size().convert<DataUnit::B>().get_value();
  size_t block_size = get_block_size().convert<DataUnit::B>().get_value();
  std::unique_ptr<char[]> line(new char[get_block_size().convert<DataUnit::B>().get_value()]);

  auto data = line.get();

  generate_random_chunk(line.get(), block_size);
  while (m_written_bytes < file_size_bytes) {
    m_written_bytes += write(fd, line.get(), block_size);
  }
  spdlog::debug("{}::prewrite: written_bytes: {} DONE", typeid(*this).name(), m_written_bytes);
  close(fd);
}

void ReadMonitoredAction::log_values() { addMonitoredData("read_bytes", m_read_bytes); }

inline off_t RandomReadMonitoredAction::get_offset() {
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

inline off_t WriteMonitoredAction::get_offset() {
  static off_t offset = 0;
  static off_t block_size_bytes = get_block_size().convert<DataUnit::B>().get_value();
  static ssize_t file_size_bytes = get_file_size().convert<DataUnit::B>().get_value();

  offset += block_size_bytes;
  if (m_written_bytes % file_size_bytes == 0) {
    spdlog::debug("{}::get_offset: resetting offset", typeid(*this).name());
    offset = 0;
  }
  return offset;
}
inline void WriteMonitoredAction::generate_random_chunk(char* chunk, size_t size) {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis(0, 255);

  std::generate_n(chunk, size, []() { return dis(gen); });
}

WriteMonitoredAction::WriteMonitoredAction(std::chrono::milliseconds monitoring_interval, std::function<void(VirtualMonitoredAction*)> on_log, FileActionAttributes file_attributes)
    //: VirtualMonitoredAction(monitoring_interval, on_log), FileActionAttributes(file_attributes) {}
    : RWAction(monitoring_interval, on_log, file_attributes) {}

void WriteMonitoredAction::work() {
  spdlog::debug("{}::work: file_path: {}", typeid(*this).name(), get_file_path());
  int fd;
  std::unique_ptr<char[]> line(new char[get_block_size().convert<DataUnit::B>().get_value()]);

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  error "Windows is not supported"
#elif __APPLE__
  fd = open(get_file_path().c_str(), O_RDWR | O_CREAT, S_IRWXU);
  if (fcntl(fd, F_NOCACHE, 1) == -1 && fd != -1) {
    close(fd);
    throw std::runtime_error("WriteMonitoredAction::work: error on fcntl F_NOCACHE");
  }
#elif __linux__ || __unix__ || defined(_POSIX_VERSION)
  fd = open(get_file_path().c_str(), O_DIRECT | O_RDWR | O_CREAT, S_IRWXU);
#else
#  warning "Unknown system"
  fd = open(get_file_path().c_str(), O_DIRECT | O_RDWR | O_CREAT, S_IRWXU);
#endif
  if (fd == -1) {
    std::cerr << "Error opening file: " << strerror(errno) << std::endl;
    if (errno == EINVAL) {
      throw std::runtime_error("WriteMonitoredAction::work: error Direct IO not supported");
    }
    throw std::runtime_error("WriteMonitoredAction::work: error opening file");
  }

  spdlog::debug("{}::work: file_size: {}", typeid(*this).name(), get_file_size());
  generate_random_chunk(line.get(), get_block_size().convert<DataUnit::B>().get_value());
  if (is_time_based()) {
    auto start_time = std::chrono::high_resolution_clock::now();
    auto now_time = std::chrono::high_resolution_clock::now();
    spdlog::debug("{}::work: start_time: {}", typeid(*this).name(), start_time.time_since_epoch().count());
    spdlog::debug("{}::work: interval: {}", typeid(*this).name(), std::chrono::duration_cast<std::chrono::milliseconds>(get_time_limit()).count());

    auto data = line.get();
    size_t block_size = get_block_size().convert<DataUnit::B>().get_value();

    while (std::chrono::duration_cast<std::chrono::milliseconds>(now_time - start_time).count() <= std::chrono::duration_cast<std::chrono::milliseconds>(get_time_limit()).count()) {
      m_written_bytes += pwrite(fd, data, block_size, get_offset());
      now_time = std::chrono::high_resolution_clock::now();
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

void WriteMonitoredAction::log_values() { addMonitoredData("write_bytes", m_written_bytes); }

inline off_t RandomWriteMonitoredAction::get_offset() {
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
