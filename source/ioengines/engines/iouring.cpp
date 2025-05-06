#include <fcntl.h>
#include <filestorm/ioengines/ioengine.h>
#include <filestorm/ioengines/register.h>
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
  std::string description() const override { return "IO uring I/O engine"; }

  ssize_t read(int fd, void* buf, size_t count) override;
  ssize_t write(int fd, void* buf, size_t count) override;
  void sync(int fd) override;

private:
  struct io_uring ring_;
  unsigned queue_depth_ = 0;
};

IOUringEngine::IOUringEngine() {
  // register CLI parameters
  addParameter(Parameter("h", "help", "Show help", "false", false));
  addParameter(Parameter("q", "queue-depth", "Submission queue depth for io_uring", "128"));

  // parse queue depth
  queue_depth_ = static_cast<unsigned>(std::stoul(getParameter("queue-depth").get_string()));
  // initialize io_uring
  if (io_uring_queue_init(queue_depth_, &ring_, 0) < 0) {
    throw std::runtime_error(std::string("io_uring_queue_init failed: ") + std::strerror(errno));
  }
}

IOUringEngine::~IOUringEngine() { io_uring_queue_exit(&ring_); }

ssize_t IOUringEngine::read(int fd, void* buf, size_t count) {
  // get current offset
  off_t offset = lseek(fd, 0, SEEK_CUR);
  if (offset == (off_t)-1) throw std::runtime_error(std::string("lseek failed: ") + std::strerror(errno));

  // prepare submission
  struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
  if (!sqe) throw std::runtime_error("io_uring_get_sqe returned nullptr");
  io_uring_prep_read(sqe, fd, buf, count, offset);

  // submit and wait
  if (io_uring_submit(&ring_) < 0) throw std::runtime_error(std::string("io_uring_submit failed: ") + std::strerror(errno));

  struct io_uring_cqe* cqe;
  if (io_uring_wait_cqe(&ring_, &cqe) < 0) throw std::runtime_error(std::string("io_uring_wait_cqe failed: ") + std::strerror(errno));

  ssize_t res = cqe->res;
  io_uring_cqe_seen(&ring_, cqe);

  if (res < 0) throw std::runtime_error(std::string("io_uring read error: ") + std::strerror(-res));

  // advance offset
  if (lseek(fd, res, SEEK_CUR) == (off_t)-1) throw std::runtime_error(std::string("lseek advance failed: ") + std::strerror(errno));

  return res;
}

ssize_t IOUringEngine::write(int fd, void* buf, size_t count) {
  off_t offset = lseek(fd, 0, SEEK_CUR);
  if (offset == (off_t)-1) throw std::runtime_error(std::string("lseek failed: ") + std::strerror(errno));

  struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
  if (!sqe) throw std::runtime_error("io_uring_get_sqe returned nullptr");
  io_uring_prep_write(sqe, fd, buf, count, offset);

  if (io_uring_submit(&ring_) < 0) throw std::runtime_error(std::string("io_uring_submit failed: ") + std::strerror(errno));

  struct io_uring_cqe* cqe;
  if (io_uring_wait_cqe(&ring_, &cqe) < 0) throw std::runtime_error(std::string("io_uring_wait_cqe failed: ") + std::strerror(errno));

  ssize_t res = cqe->res;
  io_uring_cqe_seen(&ring_, cqe);

  if (res < 0) throw std::runtime_error(std::string("io_uring write error: ") + std::strerror(-res));

  if (lseek(fd, res, SEEK_CUR) == (off_t)-1) throw std::runtime_error(std::string("lseek advance failed: ") + std::strerror(errno));

  return res;
}

void IOUringEngine::sync(int fd) {
  struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
  if (!sqe) throw std::runtime_error("io_uring_get_sqe for fsync failed");
  io_uring_prep_fsync(sqe, fd, IORING_FSYNC_DATASYNC);

  if (io_uring_submit(&ring_) < 0) throw std::runtime_error(std::string("io_uring_submit fsync failed: ") + std::strerror(errno));

  struct io_uring_cqe* cqe;
  if (io_uring_wait_cqe(&ring_, &cqe) < 0) throw std::runtime_error(std::string("io_uring_wait_cqe fsync failed: ") + std::strerror(errno));

  int res = cqe->res;
  io_uring_cqe_seen(&ring_, cqe);
  if (res < 0) throw std::runtime_error(std::string("io_uring fsync error: ") + std::strerror(-res));
}

REGISTER_IO_ENGINE(IOUringEngine);
