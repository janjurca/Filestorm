#include <filestorm/ioengines/ioengine.h>
#include <filestorm/ioengines/register.h>

#include <iostream>

class SyncIOEngine : public IOEngine {
public:
  SyncIOEngine();
  std::string name() const override { return "sync"; }
  std::string description() const override { return "POSIX synchronous I/O engine"; }
  void read() override { std::cout << "[POSIX] Reading\n"; }
  void write() override { std::cout << "[POSIX] Writing\n"; }
};
REGISTER_IO_ENGINE(SyncIOEngine);

SyncIOEngine::SyncIOEngine() { addParameter(Parameter("h", "help", "Show help", "false", false)); }