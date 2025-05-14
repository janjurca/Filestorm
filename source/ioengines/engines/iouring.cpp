#include <fcntl.h>
#include <filestorm/ioengines/ioengine.h>
#include <filestorm/ioengines/register.h>
#include <filestorm/utils/logger.h>
#include <liburing.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>

class IOUringEngine : public IOEngine {
public:
  IOUringEngine();
  ~IOUringEngine() override;

  std::string name() const override { return "iouring"; }
  std::string description() const override { return "io_uring I/O engine"; }

  ssize_t write(int fd, void* buf, size_t count, off_t offset) override;
  ssize_t read(int fd, void* buf, size_t count, off_t offset) override;
  ssize_t complete();

  std::string setup(int argc, char** argv) override;

private:
  ssize_t wait_for_some_completions();

  struct io_uring ring_;
  unsigned iodepth_ = 1;
  unsigned submitted_cbs = 0;
};

IOUringEngine::IOUringEngine() {
  // Register CLI parameters
  addParameter(Parameter("h", "help", "Show help", "false", false));
  addParameter(Parameter("", "iodepth", "I/O depth for io_uring", "1"));
}

std::string IOUringEngine::setup(int argc, char** argv) {
  auto ret = IOEngine::setup(argc, argv);
  // Parse iodepth and initialize io_uring context
  std::string depth = getParameter("iodepth").get_string();
  if (depth.empty()) {
    throw std::invalid_argument("I/O depth must be a positive integer.");
  }
  iodepth_ = static_cast<unsigned>(std::stoul(depth));

  // Setup io_uring context
  if (io_uring_queue_init(iodepth_, &ring_, 0) < 0) {
    throw std::runtime_error(std::string("io_uring_queue_init failed: ") + std::strerror(errno));
  }
  return ret;
}

IOUringEngine::~IOUringEngine() {
  if (submitted_cbs > 0) {
    complete();  // Ensure all submitted operations are completed before cleanup
  }
  io_uring_queue_exit(&ring_);
}

ssize_t IOUringEngine::read(int fd, void* buf, size_t count, off_t offset) {
  ssize_t processed_bytes = 0;
  if (submitted_cbs >= iodepth_) {
    processed_bytes += wait_for_some_completions();  // Return bytes from one completed op
  }

  struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
  if (!sqe) {
    throw std::runtime_error("Failed to get SQE from io_uring.");
  }

  io_uring_prep_read(sqe, fd, buf, count, offset);
  io_uring_sqe_set_data(sqe, sqe);

  int ret = io_uring_submit(&ring_);
  if (ret < 0) {
    throw std::runtime_error(std::string("io_uring_submit read failed: ") + std::strerror(-ret));
  }
  submitted_cbs++;
  return processed_bytes;
}

ssize_t IOUringEngine::write(int fd, void* buf, size_t count, off_t offset) {
  ssize_t processed_bytes = 0;
  if (submitted_cbs >= iodepth_) {
    processed_bytes += wait_for_some_completions();  // Return bytes from one completed op
  }

  struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
  if (!sqe) {
    throw std::runtime_error("Failed to get SQE from io_uring.");
  }

  io_uring_prep_write(sqe, fd, buf, count, offset);
  io_uring_sqe_set_data(sqe, sqe);

  int ret = io_uring_submit(&ring_);
  if (ret < 0) {
    throw std::runtime_error(std::string("io_uring_submit write failed: ") + std::strerror(-ret));
  }
  submitted_cbs++;
  return processed_bytes;
}

ssize_t IOUringEngine::complete() {
  ssize_t total_bytes = 0;
  while (submitted_cbs > 0) {
    ssize_t bytes = wait_for_some_completions();
    if (bytes < 0) {
      throw std::runtime_error(std::string("wait_for_some_completions failed: ") + std::strerror(-bytes));
    }
    total_bytes += bytes;
  }
  return total_bytes;
}

ssize_t IOUringEngine::wait_for_some_completions() {
  struct io_uring_cqe* cqe;
  int ret = io_uring_wait_cqe(&ring_, &cqe);
  if (ret < 0) {
    throw std::runtime_error(std::string("io_uring_wait_cqe failed: ") + std::strerror(-ret));
  }

  ssize_t bytes = cqe->res;
  if (bytes < 0) {
    throw std::runtime_error(std::string("I/O operation failed: ") + std::strerror(-bytes));
  }

  // Optional: You can check for more information in the cqe->user_data, if needed.
  io_uring_cqe_seen(&ring_, cqe);
  submitted_cbs--;
  return bytes;
}

REGISTER_IO_ENGINE(IOUringEngine);
