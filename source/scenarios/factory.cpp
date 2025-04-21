
#include <filestorm/scenarios/factory.h>

ScenarioFactory& ScenarioFactory::instance() {
  static ScenarioFactory factory;
  return factory;
}

void ScenarioFactory::registerScenario(const std::string& name, Creator creator) { creators[name] = creator; }

std::unique_ptr<Scenario> ScenarioFactory::create(const std::string& name) {
  if (creators.count(name)) {
    return creators[name]();
  }
  return nullptr;
}

std::vector<std::string> ScenarioFactory::listScenarios() const {
  std::vector<std::string> names;
  for (const auto& [key, _] : creators) {
    names.push_back(key);
  }
  return names;
}
