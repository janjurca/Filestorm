#include <filestorm/ioengines/ioengine.h>
#include <filestorm/ioengines/register.h>

#include <iostream>

class PosixIOEngine : public IOEngine {
public:
  std::string name() const override { return "posix"; }
  void read() override { std::cout << "[POSIX] Reading\n"; }
  void write() override { std::cout << "[POSIX] Writing\n"; }
};

REGISTER_IO_ENGINE(PosixIOEngine);
