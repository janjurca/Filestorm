#include <filestorm/actions/rw_actions.h>

#include <thread>

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
  logger.debug("{}::work: file_path: {}", typeid(*this).name(), get_file_path());
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
  logger.debug("{}::work: file_size: {}", typeid(*this).name(), get_file_size());

  if (is_time_based()) {
    auto start_time = std::chrono::high_resolution_clock::now();
    auto now_time = std::chrono::high_resolution_clock::now();
    logger.debug("{}::work: start_time: {}", typeid(*this).name(), start_time.time_since_epoch().count());
    logger.debug("{}::work: interval: {}", typeid(*this).name(), std::chrono::duration_cast<std::chrono::milliseconds>(get_time_limit()).count());

    auto data = line.get();
    size_t block_size = get_block_size().convert<DataUnit::B>().get_value();

    auto time_limit = std::chrono::duration_cast<std::chrono::milliseconds>(get_time_limit()).count();
    bool ended = false;
    std::thread timerThread([&ended, time_limit]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(time_limit));
      ended = true;
    });
    while (!ended) {
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

void ReadMonitoredAction::prewrite() {
  int fd = open(get_file_path().c_str(), O_RDWR | O_CREAT, S_IRWXU);
  if (fd == -1) {
    throw std::runtime_error(fmt::format("{}::prewrite: error opening file: {}", typeid(*this).name(), strerror(errno)));
  }
  u_int64_t m_written_bytes = 0;
  logger.debug("{}::prewrite: file_size: {}", typeid(*this).name(), get_file_size());
  u_int64_t file_size_bytes = get_file_size().convert<DataUnit::B>().get_value();
  size_t block_size = get_block_size().convert<DataUnit::B>().get_value();
  std::unique_ptr<char[]> line(new char[get_block_size().convert<DataUnit::B>().get_value()]);

  auto data = line.get();

  generate_random_chunk(line.get(), block_size);
  while (m_written_bytes < file_size_bytes) {
    m_written_bytes += write(fd, line.get(), block_size);
  }
  logger.debug("{}::prewrite: written_bytes: {} DONE", typeid(*this).name(), m_written_bytes);
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

WriteMonitoredAction::WriteMonitoredAction(std::chrono::milliseconds monitoring_interval, std::function<void(VirtualMonitoredAction*)> on_log, FileActionAttributes file_attributes)
    //: VirtualMonitoredAction(monitoring_interval, on_log), FileActionAttributes(file_attributes) {}
    : RWAction(monitoring_interval, on_log, file_attributes) {}

inline off_t WriteMonitoredAction::get_offset() {
  static off_t offset = 0;
  static off_t block_size_bytes = get_block_size().convert<DataUnit::B>().get_value();
  static ssize_t file_size_bytes = get_file_size().convert<DataUnit::B>().get_value();

  offset += block_size_bytes;
  if (m_written_bytes % file_size_bytes == 0) {
    logger.debug("{}::get_offset: resetting offset", typeid(*this).name());
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
void WriteMonitoredAction::work() {
  logger.debug("{}::work: file_path: {}", typeid(*this).name(), get_file_path());
  int fd;
  size_t block_size = get_block_size().convert<DataUnit::B>().get_value();

  // Allocate aligned buffer for Direct I/O.
  void* buffer_ptr = nullptr;
  if (posix_memalign(&buffer_ptr, block_size, block_size) != 0) {
    throw std::runtime_error("WriteMonitoredAction::work: posix_memalign failed");
  }
  std::unique_ptr<char, decltype(&free)> line(static_cast<char*>(buffer_ptr), free);

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

  logger.debug("{}::work: file_size: {}", typeid(*this).name(), get_file_size());
  generate_random_chunk(line.get(), block_size);

  if (is_time_based()) {
    auto start_time = std::chrono::high_resolution_clock::now();
    logger.debug("{}::work: start_time: {}", typeid(*this).name(), start_time.time_since_epoch().count());
    auto time_limit = std::chrono::duration_cast<std::chrono::milliseconds>(get_time_limit()).count();
    logger.debug("{}::work: interval: {}", typeid(*this).name(), time_limit);

    bool ended = false;
    std::thread timerThread([&ended, time_limit]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(time_limit));
      ended = true;
    });

    logger.debug("{}::work: m_written_bytes: {}", typeid(*this).name(), m_written_bytes);
    off_t offset = get_offset();
    // Ensure offset is aligned to block_size
    if (offset % block_size != 0) {
      close(fd);
      throw std::runtime_error("WriteMonitoredAction::work: offset is not aligned to block size");
    }
    while (!ended) {
      ssize_t returned_bytes = pwrite(fd, line.get(), block_size, offset);
      if (returned_bytes == -1) {
        logger.error("{}::work: error writing to file: {}", typeid(*this).name(), strerror(errno));
        logger.error("{}::work: fd: {} block_size: {} offset: {}", typeid(*this).name(), fd, block_size, offset);
        break;
      }
      m_written_bytes += returned_bytes;
      offset += returned_bytes;  // update offset for sequential writes
    }
    timerThread.join();
  } else {
    // For non time-based writes, ensure the offset and size are properly handled as well.
    size_t file_size = get_file_size().convert<DataUnit::B>().get_value();
    off_t offset = 0;
    while (m_written_bytes < file_size) {
      ssize_t written = write(fd, line.get(), block_size);
      if (written == -1) {
        logger.error("{}::work: error writing to file: {}", typeid(*this).name(), strerror(errno));
        break;
      }
      m_written_bytes += written;
      offset += written;
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
