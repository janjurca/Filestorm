#pragma once
#include <filestorm/parameter.h>
#include <filestorm/utils/logger.h>

#pragma once
#include <string>

class IOEngine {
public:
  virtual ~IOEngine() = default;
  virtual std::string name() const = 0;
  virtual void read() = 0;
  virtual void write() = 0;
};
