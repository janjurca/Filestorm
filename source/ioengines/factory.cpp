
#include <filestorm/ioengines/factory.h>

IOEngineFactory& IOEngineFactory::instance() {
  static IOEngineFactory factory;
  return factory;
}

void IOEngineFactory::registerEngine(const std::string& name, Creator creator) { creators[name] = creator; }

std::unique_ptr<IOEngine> IOEngineFactory::create(const std::string& name) {
  if (creators.count(name)) {
    return creators[name]();
  }
  return nullptr;
}

std::vector<std::string> IOEngineFactory::listEngines() const {
  std::vector<std::string> names;
  for (const auto& [key, _] : creators) {
    names.push_back(key);
  }
  return names;
}
