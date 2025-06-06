#include <filestorm/ioengines/ioengine.h>
#include <filestorm/ioengines/register.h>

#include <iostream>

class SyncIOEngine : public IOEngine {
public:
  SyncIOEngine();
  std::string name() const override { return "sync"; }
  std::string description() const override { return "POSIX synchronous I/O engine"; }
  ssize_t read(int fd, void* buf, size_t count, off_t offset) override { return ::pread(fd, buf, count, offset); }
  ssize_t write(int fd, void* buf, size_t count, off_t offset) override { return ::pwrite(fd, buf, count, offset); }
  ssize_t complete() override { return 0; }  // No async completion for sync I/O
};
REGISTER_IO_ENGINE(SyncIOEngine);

SyncIOEngine::SyncIOEngine() { addParameter(Parameter("h", "help", "Show help", "false", false)); }