#include <fcntl.h>
#include <filestorm/ioengines/ioengine.h>
#include <filestorm/ioengines/register.h>
#include <filestorm/utils/logger.h>
#include <unistd.h>
#include <aio.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <vector>

class POSIXAIOEngine : public IOEngine {
public:
  POSIXAIOEngine();
  ~POSIXAIOEngine() override;

  std::string name() const override { return "posix_aio"; }
  std::string description() const override { return "POSIX AIO I/O engine"; }

  ssize_t write(int fd, void* buf, size_t count, off_t offset) override;
  ssize_t read(int fd, void* buf, size_t count, off_t offset) override;
  ssize_t complete();

  std::string setup(int argc, char** argv) override;

private:
  ssize_t wait_for_some_completions();

  unsigned iodepth_ = 1;
  unsigned submitted_cbs = 0;
  std::vector<struct aiocb*> io_requests_;
};

POSIXAIOEngine::POSIXAIOEngine() {
  // register CLI parameters
  addParameter(Parameter("h", "help", "Show help", "false", false));
  addParameter(Parameter("", "iodepth", "I/O depth for POSIX AIO", "1"));
}

std::string POSIXAIOEngine::setup(int argc, char** argv) {
  auto ret = IOEngine::setup(argc, argv);
  // parse iodepth and initialize AIO context
  std::string depth = getParameter("iodepth").get_string();
  if (depth.empty()) {
    throw std::invalid_argument("I/O depth must be a positive integer.");
  }
  iodepth_ = static_cast<unsigned>(std::stoul(depth));
  return ret;
}

POSIXAIOEngine::~POSIXAIOEngine() {
  // Cleanup the submitted io requests
  for (auto req : io_requests_) {
    delete req;
  }
}

ssize_t POSIXAIOEngine::read(int fd, void* buf, size_t count, off_t offset) {
  ssize_t processed_bytes = 0;
  if (submitted_cbs >= iodepth_) {
    processed_bytes += wait_for_some_completions();  // Return bytes from one completed op
  }

  struct aiocb* cb = new struct aiocb();
  memset(cb, 0, sizeof(struct aiocb));
  cb->aio_fildes = fd;
  cb->aio_buf = buf;
  cb->aio_nbytes = count;
  cb->aio_offset = offset;

  int ret = aio_read(cb);
  if (ret < 0) {
    delete cb;
    throw std::runtime_error(std::string("aio_read failed: ") + std::strerror(errno));
  }

  io_requests_.push_back(cb);
  submitted_cbs++;
  return processed_bytes;
}

ssize_t POSIXAIOEngine::write(int fd, void* buf, size_t count, off_t offset) {
  ssize_t processed_bytes = 0;
  if (submitted_cbs >= iodepth_) {
    processed_bytes += wait_for_some_completions();  // Return bytes from one completed op
  }

  struct aiocb* cb = new struct aiocb();
  memset(cb, 0, sizeof(struct aiocb));
  cb->aio_fildes = fd;
  cb->aio_buf = buf;
  cb->aio_nbytes = count;
  cb->aio_offset = offset;

  int ret = aio_write(cb);
  if (ret < 0) {
    delete cb;
    throw std::runtime_error(std::string("aio_write failed: ") + std::strerror(errno));
  }

  io_requests_.push_back(cb);
  submitted_cbs++;
  return processed_bytes;
}

ssize_t POSIXAIOEngine::complete() {
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

ssize_t POSIXAIOEngine::wait_for_some_completions() {
  ssize_t total_bytes = 0;

  for (size_t i = 0; i < io_requests_.size(); ++i) {
    struct aiocb* cb = io_requests_[i];

    if (aio_error(cb) != EINPROGRESS) {
      int ret = aio_return(cb);
      if (ret < 0) {
        throw std::runtime_error(std::string("AIO operation failed: ") + std::strerror(aio_error(cb)));
      }
      delete cb;
      io_requests_.erase(io_requests_.begin() + i);
      submitted_cbs--;
      total_bytes += ret;
      --i;  // Adjust index due to removal
    }
  }

  return total_bytes;
}

REGISTER_IO_ENGINE(POSIXAIOEngine);
