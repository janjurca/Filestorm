#include <fcntl.h>
#include <filestorm/ioengines/ioengine.h>
#include <filestorm/ioengines/register.h>
#include <filestorm/utils/logger.h>
#include <libaio.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>

class LibAIOEngine : public IOEngine {
public:
  LibAIOEngine();
  ~LibAIOEngine() override;

  std::string name() const override { return "libaio"; }
  std::string description() const override { return "Libaio I/O engine"; }

  ssize_t write(int fd, void* buf, size_t count, off_t offset) override;
  ssize_t read(int fd, void* buf, size_t count, off_t offset) override;
  ssize_t complete();

  std::string setup(int argc, char** argv) override;

private:
  ssize_t wait_for_some_completions();

  io_context_t ctx_ = 0;
  unsigned iodepth_ = 1;
  unsigned submitted_cbs = 0;
};

LibAIOEngine::LibAIOEngine() {
  // register CLI parameters
  addParameter(Parameter("h", "help", "Show help", "false", false));
  addParameter(Parameter("", "iodepth", "I/O depth for libaio", "1"));
}

std::string LibAIOEngine::setup(int argc, char** argv) {
  auto ret = IOEngine::setup(argc, argv);
  // parse iodepth and initialize AIO context
  std::string depth = getParameter("iodepth").get_string();
  if (depth.empty()) {
    throw std::invalid_argument("I/O depth must be a positive integer.");
  }
  iodepth_ = static_cast<unsigned>(std::stoul(depth));
  if (io_setup(iodepth_, &ctx_) < 0) {
    ctx_ = 0;
    throw std::runtime_error(std::string("io_setup failed: ") + std::strerror(errno));
  }
  return ret;
}

LibAIOEngine::~LibAIOEngine() {
  if (ctx_) {
    complete();
    io_destroy(ctx_);
  };
}

ssize_t LibAIOEngine::read(int fd, void* buf, size_t count, off_t offset) {
  ssize_t processed_bytes = 0;
  if (submitted_cbs >= iodepth_) {
    processed_bytes += wait_for_some_completions();  // Return bytes from one completed op
  }

  auto cb = new iocb();
  memset(cb, 0, sizeof(cb));
  io_prep_pread(cb, fd, buf, count, offset);
  cb->data = cb;

  struct iocb* cbs[1] = {cb};

  int ret = io_submit(ctx_, 1, cbs);
  if (ret < 0) {
    throw std::runtime_error(std::string("io_submit read failed: ") + std::strerror(-ret));
  }
  submitted_cbs++;
  return processed_bytes;
}

ssize_t LibAIOEngine::write(int fd, void* buf, size_t count, off_t offset) {
  ssize_t processed_bytes = 0;
  if (submitted_cbs >= iodepth_) {
    processed_bytes += wait_for_some_completions();  // Return bytes from one completed op
  }

  auto cb = new iocb();
  memset(cb, 0, sizeof(cb));
  io_prep_pwrite(cb, fd, buf, count, offset);
  cb->data = cb;

  struct iocb* cbs[1] = {cb};

  int ret = io_submit(ctx_, 1, cbs);
  if (ret < 0) {
    throw std::runtime_error(std::string("io_submit write failed: ") + std::strerror(-ret));
  }
  submitted_cbs++;
  return processed_bytes;
}

ssize_t LibAIOEngine::complete() {
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

ssize_t LibAIOEngine::wait_for_some_completions() {
  std::vector<io_event> events(iodepth_);
  int ret = io_getevents(ctx_, 1, iodepth_, events.data(), nullptr);
  if (ret < 0) {
    throw std::runtime_error(std::string("io_getevents failed: ") + std::strerror(-ret));
  }

  ssize_t total_bytes = 0;

  for (int i = 0; i < ret; ++i) {
    struct iocb* completed_cb = static_cast<struct iocb*>(events[i].obj);
    ssize_t bytes = events[i].res;
    if (events[i].res < 0) {
      throw std::runtime_error(std::string("AIO operation failed: ") + std::strerror(-events[i].res));
    }
    if (events[i].res2 != 0) {
      throw std::runtime_error(std::string("AIO operation failed with unexpected result: ") + std::strerror(-events[i].res2));
    }
    if (completed_cb->data != completed_cb) {
      throw std::runtime_error("AIO operation completed with unexpected data");
    }
    delete static_cast<iocb*>(events[i].obj);
    submitted_cbs--;
    total_bytes += bytes;
  }

  return total_bytes;
}

REGISTER_IO_ENGINE(LibAIOEngine);
