#include <fcntl.h>
#include <filestorm/ioengines/ioengine.h>
#include <filestorm/ioengines/register.h>
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

  ssize_t read(int fd, void* buf, size_t count) override;
  ssize_t write(int fd, void* buf, size_t count) override;

private:
  io_context_t ctx_ = 0;
  unsigned iodepth_ = 1;
};

LibAIOEngine::LibAIOEngine() {
  // register CLI parameters
  addParameter(Parameter("h", "help", "Show help", "false", false));
  addParameter(Parameter("", "iodepth", "I/O depth for libaio", "1"));

  // parse iodepth and initialize AIO context
  std::string depth = getParameter("iodepth").get_string();
  if (depth.empty()) {
    throw std::invalid_argument("I/O depth must be a positive integer");
  }
  iodepth_ = static_cast<unsigned>(std::stoul(depth));
  if (io_setup(iodepth_, &ctx_) < 0) {
    throw std::runtime_error(std::string("io_setup failed: ") + std::strerror(errno));
  }
}

LibAIOEngine::~LibAIOEngine() {
  if (ctx_) io_destroy(ctx_);
}

ssize_t LibAIOEngine::read(int fd, void* buf, size_t count) {
  // determine current file offset
  off_t offset = lseek(fd, 0, SEEK_CUR);
  if (offset == (off_t)-1) throw std::runtime_error(std::string("lseek failed: ") + std::strerror(errno));

  // prepare AIO control block
  struct iocb cb;
  io_prep_pread(&cb, fd, buf, count, offset);
  struct iocb* cbs[1] = {&cb};

  // submit the read request
  int ret = io_submit(ctx_, 1, cbs);
  if (ret < 0) throw std::runtime_error(std::string("io_submit read failed: ") + std::strerror(-ret));

  // wait for completion
  struct io_event events[1];
  ret = io_getevents(ctx_, 1, 1, events, nullptr);
  if (ret < 0) throw std::runtime_error(std::string("io_getevents read failed: ") + std::strerror(-ret));

  // advance file offset
  off_t bytes = events[0].res;
  if (lseek(fd, bytes, SEEK_CUR) == (off_t)-1) throw std::runtime_error(std::string("lseek advance failed: ") + std::strerror(errno));

  return bytes;
}

ssize_t LibAIOEngine::write(int fd, void* buf, size_t count) {
  off_t offset = lseek(fd, 0, SEEK_CUR);
  if (offset == (off_t)-1) throw std::runtime_error(std::string("lseek failed: ") + std::strerror(errno));

  struct iocb cb;
  io_prep_pwrite(&cb, fd, buf, count, offset);
  struct iocb* cbs[1] = {&cb};

  int ret = io_submit(ctx_, 1, cbs);
  if (ret < 0) throw std::runtime_error(std::string("io_submit write failed: ") + std::strerror(-ret));

  struct io_event events[1];
  ret = io_getevents(ctx_, 1, 1, events, nullptr);
  if (ret < 0) throw std::runtime_error(std::string("io_getevents write failed: ") + std::strerror(-ret));

  off_t bytes = events[0].res;
  if (lseek(fd, bytes, SEEK_CUR) == (off_t)-1) throw std::runtime_error(std::string("lseek advance failed: ") + std::strerror(errno));

  return bytes;
}

REGISTER_IO_ENGINE(LibAIOEngine);
