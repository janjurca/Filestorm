#include <filestorm/ioengines/ioengine.h>
#include <filestorm/ioengines/register.h>

#include <iostream>

class SyncIOEngine : public IOEngine {
public:
  SyncIOEngine();
  std::string name() const override { return "sync"; }
  std::string description() const override { return "POSIX synchronous I/O engine"; }
  ssize_t read(int fd, void* buf, size_t count) override { return ::read(fd, buf, count); }
  ssize_t write(int fd, void* buf, size_t count) override { return ::write(fd, buf, count); }
};
REGISTER_IO_ENGINE(SyncIOEngine);

SyncIOEngine::SyncIOEngine() { addParameter(Parameter("h", "help", "Show help", "false", false)); }