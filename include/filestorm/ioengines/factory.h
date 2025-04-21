#pragma once

#include <filestorm/ioengines/ioengine.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class IOEngineFactory {
public:
  using Creator = std::function<std::unique_ptr<IOEngine>()>;

  static IOEngineFactory& instance();

  void registerEngine(const std::string& name, Creator creator);
  std::unique_ptr<IOEngine> create(const std::string& name);
  std::vector<std::string> listEngines() const;

private:
  std::unordered_map<std::string, Creator> creators;
};
